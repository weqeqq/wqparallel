/**
 * \file for_each.h
 * \brief Parallel-friendly algorithms built on top of ThreadPool.
 */

#pragma once

#include <weqeqq/parallel/execution.h>
#include <weqeqq/parallel/thread_pool.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <thread>
#include <utility>
#include <variant>

namespace weqeqq::parallel {

namespace detail {

inline constexpr std::ptrdiff_t kExecutionParallelThreshold = 256;
inline constexpr std::size_t kDefaultExecutionThreads       = 4;

inline std::size_t DefaultExecutionThreadCount() {
  const auto hardware_threads = std::thread::hardware_concurrency();
  if (hardware_threads == 0) return 1;

  return std::min<std::size_t>(hardware_threads, kDefaultExecutionThreads);
}

}  // namespace detail

/**
 * \brief Iterates over an index range sequentially.
 * \tparam Function Callable type receiving the current index.
 * \param start Inclusive start index.
 * \param end Exclusive end index.
 * \param function Function called for each index.
 */
template <typename Function>
inline void ForEachIndex(std::ptrdiff_t start, std::ptrdiff_t end,
                         Function&& function) {
  for (auto index = start; index < end; ++index) {
    std::invoke(function, index);
  }
}

/**
 * \brief Iterates over an index range using the requested execution mode.
 * \tparam Function Callable type receiving the current index.
 * \param execution Requested execution mode.
 * \param start Inclusive start index.
 * \param end Exclusive end index.
 * \param function Function called for each index.
 */
template <typename Function>
inline void ForEachIndex(Execution execution, std::ptrdiff_t start,
                         std::ptrdiff_t end, Function&& function) {
  if (execution == Execution::kSequential || start >= end) {
    return ForEachIndex(start, end, std::forward<Function>(function));
  }

  const auto total = end - start;
  if (total <= 1) {
    return ForEachIndex(start, end, std::forward<Function>(function));
  }

  if (total < detail::kExecutionParallelThreshold) {
    return ForEachIndex(start, end, std::forward<Function>(function));
  }

  ThreadPool pool(detail::DefaultExecutionThreadCount());
  pool.ParallelFor(start, end, std::forward<Function>(function));
}

/**
 * \brief Iterates over an index range using a specific thread pool.
 * \tparam Function Callable type receiving the current index.
 * \param pool Pool that executes the parallel work.
 * \param start Inclusive start index.
 * \param end Exclusive end index.
 * \param function Function called for each index.
 */
template <typename Function>
inline void ForEachIndex(ThreadPool& pool, std::ptrdiff_t start,
                         std::ptrdiff_t end, Function&& function) {
  if (start >= end) return;
  pool.ParallelFor(start, end, std::forward<Function>(function));
}

/**
 * \brief Runtime execution policy for the high-level algorithms.
 *
 * The policy can either hold an `Execution` enum or a reference to a
 * user-supplied ThreadPool.
 */
using ExecutionPolicy =
    std::variant<std::reference_wrapper<ThreadPool>, Execution>;

/**
 * \brief Iterates over an index range using a runtime policy.
 * \tparam Function Callable type receiving the current index.
 * \param policy Execution enum or thread pool reference.
 * \param start Inclusive start index.
 * \param end Exclusive end index.
 * \param function Function called for each index.
 */
template <typename Function>
inline void ForEachIndex(ExecutionPolicy policy, std::ptrdiff_t start,
                         std::ptrdiff_t end, Function&& function) {
  if (std::holds_alternative<std::reference_wrapper<ThreadPool>>(policy)) {
    ForEachIndex(std::get<std::reference_wrapper<ThreadPool>>(policy).get(),
                 start, end, std::forward<Function>(function));
  } else {
    ForEachIndex(std::get<Execution>(policy), start, end,
                 std::forward<Function>(function));
  }
}

/**
 * \brief Convenience wrapper for running an indexed loop in parallel.
 * \tparam Function Callable type receiving the current index.
 * \param start Inclusive start index.
 * \param end Exclusive end index.
 * \param function Function called for each index.
 */
template <typename Function>
inline void ForEachIndexParallel(std::ptrdiff_t start, std::ptrdiff_t end,
                                 Function&& function) {
  ForEachIndex(Execution::kParallel, start, end,
               std::forward<Function>(function));
}

/**
 * \brief Transforms a range into an output range.
 * \tparam InputIterator Input iterator type.
 * \tparam OutputIterator Output iterator type.
 * \tparam Function Unary transform function.
 * \param execution Requested execution mode.
 * \param first Beginning of the input range.
 * \param last End of the input range.
 * \param output Beginning of the output range.
 * \param function Unary transform function.
 *
 * Random-access iterators use the indexed parallel path. Other iterator
 * categories transparently fall back to `std::transform`.
 */
template <typename InputIterator, typename OutputIterator, typename Function>
inline void Transform(Execution execution, InputIterator first,
                      InputIterator last, OutputIterator output,
                      Function&& function) {
  if (execution == Execution::kSequential) {
    std::transform(first, last, output, std::forward<Function>(function));
    return;
  }

  if constexpr (!std::random_access_iterator<InputIterator> ||
                !std::random_access_iterator<OutputIterator>) {
    std::transform(first, last, output, std::forward<Function>(function));
  } else {
    const auto size = static_cast<std::ptrdiff_t>(last - first);
    ForEachIndex(execution, 0, size, [&](std::ptrdiff_t i) {
      output[i] = std::invoke(function, first[i]);
    });
  }
}

/**
 * \brief Transforms a range using a specific thread pool.
 * \tparam InputIterator Input iterator type.
 * \tparam OutputIterator Output iterator type.
 * \tparam Function Unary transform function.
 * \param pool Pool that executes the parallel work.
 * \param first Beginning of the input range.
 * \param last End of the input range.
 * \param output Beginning of the output range.
 * \param function Unary transform function.
 */
template <typename InputIterator, typename OutputIterator, typename Function>
inline void Transform(ThreadPool& pool, InputIterator first, InputIterator last,
                      OutputIterator output, Function&& function) {
  if constexpr (!std::random_access_iterator<InputIterator> ||
                !std::random_access_iterator<OutputIterator>) {
    std::transform(first, last, output, std::forward<Function>(function));
  } else {
    const auto size = static_cast<std::ptrdiff_t>(last - first);
    ForEachIndex(pool, 0, size, [&](std::ptrdiff_t i) {
      output[i] = std::invoke(function, first[i]);
    });
  }
}

/**
 * \brief Transforms a range using a runtime policy.
 * \tparam InputIterator Input iterator type.
 * \tparam OutputIterator Output iterator type.
 * \tparam Function Unary transform function.
 * \param policy Execution enum or thread pool reference.
 * \param first Beginning of the input range.
 * \param last End of the input range.
 * \param output Beginning of the output range.
 * \param function Unary transform function.
 */
template <typename InputIterator, typename OutputIterator, typename Function>
inline void Transform(ExecutionPolicy policy, InputIterator first,
                      InputIterator last, OutputIterator output,
                      Function&& function) {
  if (std::holds_alternative<std::reference_wrapper<ThreadPool>>(policy)) {
    Transform(std::get<std::reference_wrapper<ThreadPool>>(policy).get(), first,
              last, output, std::forward<Function>(function));
  } else {
    Transform(std::get<Execution>(policy), first, last, output,
              std::forward<Function>(function));
  }
}

/**
 * \brief Applies a function to each element of a range.
 * \tparam Iterator Iterator type.
 * \tparam Function Unary function type.
 * \param execution Requested execution mode.
 * \param first Beginning of the range.
 * \param last End of the range.
 * \param function Function applied to each element.
 *
 * Random-access iterators use the indexed parallel path. Other iterator
 * categories transparently fall back to `std::for_each`.
 */
template <typename Iterator, typename Function>
inline void ForEach(Execution execution, Iterator first, Iterator last,
                    Function&& function) {
  if (execution == Execution::kSequential) {
    std::for_each(first, last, std::forward<Function>(function));
    return;
  }

  if constexpr (!std::random_access_iterator<Iterator>) {
    std::for_each(first, last, std::forward<Function>(function));
  } else {
    const auto size = static_cast<std::ptrdiff_t>(last - first);
    ForEachIndex(execution, 0, size, [&](std::ptrdiff_t i) {
      std::invoke(function, *(first + i));
    });
  }
}

/**
 * \brief Applies a function to each element using a specific thread pool.
 * \tparam Iterator Iterator type.
 * \tparam Function Unary function type.
 * \param pool Pool that executes the parallel work.
 * \param first Beginning of the range.
 * \param last End of the range.
 * \param function Function applied to each element.
 */
template <typename Iterator, typename Function>
inline void ForEach(ThreadPool& pool, Iterator first, Iterator last,
                    Function&& function) {
  if constexpr (!std::random_access_iterator<Iterator>) {
    std::for_each(first, last, std::forward<Function>(function));
  } else {
    const auto size = static_cast<std::ptrdiff_t>(last - first);
    ForEachIndex(pool, 0, size, [&](std::ptrdiff_t i) {
      std::invoke(function, *(first + i));
    });
  }
}

/**
 * \brief Applies a function to each element using a runtime policy.
 * \tparam Iterator Iterator type.
 * \tparam Function Unary function type.
 * \param policy Execution enum or thread pool reference.
 * \param first Beginning of the range.
 * \param last End of the range.
 * \param function Function applied to each element.
 */
template <typename Iterator, typename Function>
inline void ForEach(ExecutionPolicy policy, Iterator first, Iterator last,
                    Function&& function) {
  if (std::holds_alternative<std::reference_wrapper<ThreadPool>>(policy)) {
    ForEach(std::get<std::reference_wrapper<ThreadPool>>(policy).get(), first,
            last, std::forward<Function>(function));
  } else {
    ForEach(std::get<Execution>(policy), first, last,
            std::forward<Function>(function));
  }
}

}  // namespace weqeqq::parallel
