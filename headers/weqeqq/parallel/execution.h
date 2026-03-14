/**
 * \file execution.h
 * \brief Execution policy declarations.
 */

#pragma once

namespace weqeqq::parallel {

/**
 * \brief Selects whether an algorithm should run sequentially or in parallel.
 */
enum class Execution {
  /// Prefer the parallel implementation.
  kParallel,
  /// Force the sequential implementation.
  kSequential,
};

}  // namespace weqeqq::parallel
