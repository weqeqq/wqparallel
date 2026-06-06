export module weqeqq.parallel:execution;

namespace weqeqq::parallel {

/**
 * \brief Selects whether an algorithm should run sequentially or in parallel.
 */
export enum class Execution {
  /// Prefer the parallel implementation.
  kParallel,
  /// Force the sequential implementation.
  kSequential,
};

}  // namespace weqeqq::parallel
