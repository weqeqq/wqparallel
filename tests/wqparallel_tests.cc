#include <gtest/gtest.h>
#include <weqeqq/parallel.h>

#include <atomic>
#include <cstddef>
#include <list>
#include <numeric>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

TEST(ForEachIndexTest, SequentialFillsRange) {
  std::vector<int> values(8, 0);

  weqeqq::parallel::ForEachIndex(
      0, static_cast<std::ptrdiff_t>(values.size()), [&](std::ptrdiff_t i) {
        values[static_cast<std::size_t>(i)] = static_cast<int>(i * 3);
      });

  for (std::size_t i = 0; i < values.size(); ++i) {
    EXPECT_EQ(values[i], static_cast<int>(i * 3));
  }
}

TEST(ForEachIndexTest, ParallelExecutionPolicyUsesPool) {
  weqeqq::parallel::ThreadPool pool(4);
  std::vector<int> values(256, 0);

  weqeqq::parallel::ForEachIndex(
      weqeqq::parallel::ExecutionPolicy{std::ref(pool)}, 0,
      static_cast<std::ptrdiff_t>(values.size()), [&](std::ptrdiff_t i) {
        values[static_cast<std::size_t>(i)] = static_cast<int>(i);
      });

  for (std::size_t i = 0; i < values.size(); ++i) {
    EXPECT_EQ(values[i], static_cast<int>(i));
  }
}

TEST(ForEachTest, FallsBackForBidirectionalIterators) {
  std::list<int> values{1, 2, 3, 4};

  weqeqq::parallel::ForEach(weqeqq::parallel::Execution::kParallel,
                            values.begin(), values.end(),
                            [](int& value) { value *= 2; });

  EXPECT_EQ(std::vector<int>(values.begin(), values.end()),
            std::vector<int>({2, 4, 6, 8}));
}

TEST(TransformTest, ParallelRandomAccessPathProducesExpectedValues) {
  std::vector<int> input(128);
  std::iota(input.begin(), input.end(), 0);
  std::vector<int> output(input.size(), 0);

  weqeqq::parallel::Transform(weqeqq::parallel::Execution::kParallel,
                              input.begin(), input.end(), output.begin(),
                              [](int value) { return value * value; });

  for (std::size_t i = 0; i < output.size(); ++i) {
    EXPECT_EQ(output[i], static_cast<int>(i * i));
  }
}

TEST(TransformTest, ParallelExecutionCanBeRepeatedSafely) {
  std::vector<int> input(4096);
  std::iota(input.begin(), input.end(), 0);
  std::vector<int> output(input.size(), 0);

  for (int iteration = 0; iteration < 64; ++iteration) {
    weqeqq::parallel::Transform(
        weqeqq::parallel::Execution::kParallel, input.begin(), input.end(),
        output.begin(), [iteration](int value) { return value + iteration; });

    for (std::size_t i = 0; i < output.size(); ++i) {
      EXPECT_EQ(output[i], static_cast<int>(i) + iteration);
    }
  }
}

TEST(TransformTest, FallsBackForBidirectionalIterators) {
  std::list<int> input{3, 4, 5};
  std::list<int> output(input.size(), 0);

  weqeqq::parallel::Transform(weqeqq::parallel::Execution::kParallel,
                              input.begin(), input.end(), output.begin(),
                              [](int value) { return value + 7; });

  EXPECT_EQ(std::vector<int>(output.begin(), output.end()),
            std::vector<int>({10, 11, 12}));
}

TEST(ForEachTest, ParallelRandomAccessPathMutatesVector) {
  std::vector<int> values(2048, 1);

  weqeqq::parallel::ForEach(weqeqq::parallel::Execution::kParallel,
                            values.begin(), values.end(),
                            [](int& value) { value *= 3; });

  for (int value : values) {
    EXPECT_EQ(value, 3);
  }
}

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

TEST(PolicyOverloadTest, SupportsParallelAlgorithms) {
  weqeqq::parallel::ThreadPool pool(3);
  std::vector<int> input{1, 2, 3, 4};
  std::vector<int> output(input.size(), 0);

  weqeqq::parallel::Transform(weqeqq::parallel::ExecutionPolicy{std::ref(pool)},
                              input.begin(), input.end(), output.begin(),
                              [](int value) { return value + 1; });
  EXPECT_EQ(output, std::vector<int>({2, 3, 4, 5}));

  weqeqq::parallel::ForEach(
      weqeqq::parallel::ExecutionPolicy{weqeqq::parallel::Execution::kParallel},
      output.begin(), output.end(), [](int& value) { value *= 2; });
  EXPECT_EQ(output, std::vector<int>({4, 6, 8, 10}));
}

TEST(ForEachIndexTest, ParallelExecutionHandlesLargeRanges) {
  std::vector<int> values(4096, 0);

  weqeqq::parallel::ForEachIndex(
      weqeqq::parallel::Execution::kParallel, 0,
      static_cast<std::ptrdiff_t>(values.size()), [&](std::ptrdiff_t i) {
        values[static_cast<std::size_t>(i)] = static_cast<int>(i % 11);
      });

  for (std::size_t i = 0; i < values.size(); ++i) {
    EXPECT_EQ(values[i], static_cast<int>(i % 11));
  }
}

}  // namespace
