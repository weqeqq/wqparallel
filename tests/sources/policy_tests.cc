#include <gtest/gtest.h>
#include <weqeqq/parallel.h>

#include <vector>

namespace {

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

}  // namespace
