#include <gtest/gtest.h>
#include <weqeqq/parallel.h>

#include <list>
#include <vector>

namespace {

TEST(ForEachTest, FallsBackForBidirectionalIterators) {
  std::list<int> values{1, 2, 3, 4};

  weqeqq::parallel::ForEach(weqeqq::parallel::Execution::kParallel,
                            values.begin(), values.end(),
                            [](int& value) { value *= 2; });

  EXPECT_EQ(std::vector<int>(values.begin(), values.end()),
            std::vector<int>({2, 4, 6, 8}));
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

}  // namespace
