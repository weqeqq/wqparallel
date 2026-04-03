#include <gtest/gtest.h>
#include <weqeqq/parallel.h>

#include <cstddef>
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
