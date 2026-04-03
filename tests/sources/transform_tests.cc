#include <gtest/gtest.h>
#include <weqeqq/parallel.h>

#include <cstddef>
#include <list>
#include <numeric>
#include <vector>

namespace {

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

}  // namespace
