#include <gtest/gtest.h>
#include <weqeqq/parallel.h>

#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

TEST(ThreadPoolSubmitTest, PropagatesExceptionsThroughFuture) {
  weqeqq::parallel::ThreadPool pool(2);

  auto future =
      pool.Submit([]() -> int { throw std::runtime_error("submit boom"); });

  EXPECT_THROW(
      {
        try {
          static_cast<void>(future.get());
        } catch (const std::runtime_error& ex) {
          EXPECT_EQ(std::string_view(ex.what()), "submit boom");
          throw;
        }
      },
      std::runtime_error);
}

TEST(ThreadPoolParallelForTest, RepeatedCallsDoNotDeadlock) {
  weqeqq::parallel::ThreadPool pool(4);
  std::vector<int> values(1024, 0);

  for (int iteration = 0; iteration < 256; ++iteration) {
    pool.ParallelFor(
        0, static_cast<std::ptrdiff_t>(values.size()),
        [&](std::ptrdiff_t i) {
          values[static_cast<std::size_t>(i)] = static_cast<int>(i) + iteration;
        });

    for (std::size_t i = 0; i < values.size(); ++i) {
      EXPECT_EQ(values[i], static_cast<int>(i) + iteration);
    }
  }
}

TEST(ThreadPoolParallelForTest, PropagatesExceptionsAndRemainsReusable) {
  weqeqq::parallel::ThreadPool pool(4);
  std::vector<int> values(16, 0);

  for (int iteration = 0; iteration < 128; ++iteration) {
    std::atomic<int> visited{0};

    EXPECT_THROW(
        {
          try {
            pool.ParallelFor(0, 1024, [&](std::ptrdiff_t i) {
              visited.fetch_add(1, std::memory_order_relaxed);
              if (i == 17) {
                throw std::runtime_error("parallel boom");
              }
            });
          } catch (const std::runtime_error& ex) {
            EXPECT_EQ(std::string_view(ex.what()), "parallel boom");
            throw;
          }
        },
        std::runtime_error);

    EXPECT_GT(visited.load(std::memory_order_relaxed), 0);

    pool.ParallelFor(
        0, static_cast<std::ptrdiff_t>(values.size()),
        [&](std::ptrdiff_t i) {
          values[static_cast<std::size_t>(i)] = iteration + 1;
        });

    for (int value : values) {
      EXPECT_EQ(value, iteration + 1);
    }
  }
}

}  // namespace
