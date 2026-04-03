/**
 * \file thread_pool.h
 * \brief Thread pool and parallel loop primitives.
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace weqeqq::parallel {

/**
 * \brief Fixed-size thread pool for running small tasks and indexed loops.
 *
 * `ThreadPool` owns a set of worker threads and can either execute arbitrary
 * callables through Submit() or distribute index-based work with ParallelFor().
 * Exceptions thrown by submitted work are propagated through the corresponding
 * `std::future` or rethrown by ParallelFor().
 */
class ThreadPool {
 public:
  /**
   * \brief Creates a pool with the requested worker count.
   * \param num_threads Requested number of worker threads.
   *
   * When `num_threads` is zero the pool falls back to one worker thread.
   */
  explicit ThreadPool(
      std::size_t num_threads = std::thread::hardware_concurrency()) {
    if (num_threads == 0) num_threads = 1;

    workers_.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
      workers_.emplace_back([this] { WorkerLoop(); });
    }
  }

  ~ThreadPool() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      stop_ = true;
    }
    cv_.notify_all();
    for (auto& worker : workers_) {
      worker.join();
    }
  }

  ThreadPool(const ThreadPool&)            = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&)                 = delete;
  ThreadPool& operator=(ThreadPool&&)      = delete;

  /**
   * \brief Queues a callable and returns a future for its result.
   * \tparam F Callable type.
   * \param func Callable object to execute on the pool.
   * \return `std::future` associated with the queued work item.
   * \throws std::runtime_error If the pool is already stopping.
   */
  template <typename F>
  auto Submit(F&& func) -> std::future<std::invoke_result_t<std::decay_t<F>&>> {
    using ReturnType = std::invoke_result_t<std::decay_t<F>&>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::forward<F>(func));
    auto future = task->get_future();

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (stop_) {
        throw std::runtime_error("cannot submit work to a stopped ThreadPool");
      }
      tasks_.emplace([task = std::move(task)]() mutable { (*task)(); });
    }
    cv_.notify_one();

    return future;
  }

  /**
   * \brief Executes an indexed loop over `[start, end)`.
   * \tparam Function Callable type receiving the current index.
   * \param start Inclusive start index.
   * \param end Exclusive end index.
   * \param function Function invoked for each index.
   *
   * Work is chunked dynamically across the calling thread and worker threads.
   * The first exception raised by `function` cancels further work and is
   * rethrown after all scheduled workers finish.
   *
   * \throws std::runtime_error If the pool is already stopping.
   * \throws Any exception thrown by \p function.
   */
  template <typename Function>
  void ParallelFor(std::ptrdiff_t start, std::ptrdiff_t end,
                   Function&& function) {
    if (start >= end) return;

    const auto total = end - start;
    const auto task_count =
        std::min(static_cast<std::ptrdiff_t>(workers_.size() + 1), total);

    if (task_count <= 1) {
      for (auto i = start; i < end; ++i) {
        std::invoke(function, i);
      }
      return;
    }

    struct SharedState {
      std::atomic<bool> cancelled{false};
      std::ptrdiff_t completed_workers = 0;
      std::mutex done_mutex;
      std::condition_variable done_cv;
      std::mutex exception_mutex;
      std::exception_ptr exception;

      void CaptureException(std::exception_ptr ex) {
        cancelled.store(true, std::memory_order_release);
        std::lock_guard<std::mutex> lock(exception_mutex);
        if (!exception) {
          exception = ex;
        }
      }

      void MarkWorkerDone(std::ptrdiff_t expected_workers) {
        std::lock_guard<std::mutex> lock(done_mutex);
        ++completed_workers;
        if (completed_workers == expected_workers) {
          done_cv.notify_one();
        }
      }
    };

    const auto block_size =
        std::max<std::ptrdiff_t>(1, total / (task_count * 4));
    std::atomic<std::ptrdiff_t> next{start};
    auto state = std::make_shared<SharedState>();

    auto run_blocks = [&function, &next, end, block_size, state]() {
      while (!state->cancelled.load(std::memory_order_acquire)) {
        const auto chunk_start =
            next.fetch_add(block_size, std::memory_order_relaxed);
        if (chunk_start >= end) {
          break;
        }

        const auto chunk_end = std::min(end, chunk_start + block_size);
        for (auto i = chunk_start; i < chunk_end; ++i) {
          if (state->cancelled.load(std::memory_order_acquire)) {
            return;
          }

          try {
            std::invoke(function, i);
          } catch (...) {
            state->CaptureException(std::current_exception());
            return;
          }
        }
      }
    };

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (stop_) {
        throw std::runtime_error("cannot submit work to a stopped ThreadPool");
      }

      for (std::ptrdiff_t t = 0; t < task_count - 1; ++t) {
        tasks_.emplace([run_blocks, state, task_count]() mutable {
          run_blocks();
          state->MarkWorkerDone(task_count - 1);
        });
      }
    }
    cv_.notify_all();

    run_blocks();

    std::unique_lock<std::mutex> lock(state->done_mutex);
    state->done_cv.wait(lock, [&] {
      return state->completed_workers == task_count - 1;
    });
    lock.unlock();

    std::exception_ptr exception;
    {
      std::lock_guard<std::mutex> exception_lock(state->exception_mutex);
      exception = state->exception;
    }
    if (exception) {
      std::rethrow_exception(exception);
    }
  }

  /**
   * \brief Returns the number of owned worker threads.
   */
  std::size_t NumThreads() const { return workers_.size(); }

  /**
   * \brief Returns the lazily created process-wide pool instance.
   */
  static ThreadPool& Global() {
    static ThreadPool instance;
    return instance;
  }

 private:
  void WorkerLoop() {
    while (true) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
        if (stop_ && tasks_.empty()) return;
        task = std::move(tasks_.front());
        tasks_.pop();
      }
      task();
    }
  }

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool stop_ = false;
};

}  // namespace weqeqq::parallel
