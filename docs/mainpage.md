# wqparallel

`wqparallel` is a header-only C++23 library that exposes a small thread pool
and a compact set of parallel-friendly algorithms.

## Modules

- `weqeqq::parallel::ThreadPool` schedules task objects and indexed loops.
- `weqeqq::parallel::Execution` chooses between sequential and parallel modes.
- `weqeqq::parallel::ExecutionPolicy` allows selecting either an execution mode
  or a specific thread pool instance at runtime.
- `ForEachIndex`, `ForEach`, and `Transform` provide a minimal algorithm layer
  on top of the pool.

## Design Notes

- `ParallelFor()` dynamically distributes work in chunks and rethrows the first
  exception after all scheduled workers finish.
- `ForEach()` and `Transform()` use an indexed parallel path only for
  random-access iterators.
- Non-random-access iterators automatically fall back to the corresponding
  standard algorithms.

## Example

```cpp
#include <vector>
#include <weqeqq/parallel.h>

int main() {
  std::vector<int> values(1024, 0);

  weqeqq::parallel::ForEachIndex(
      weqeqq::parallel::Execution::kParallel, 0,
      static_cast<std::ptrdiff_t>(values.size()),
      [&](std::ptrdiff_t i) {
        values[static_cast<std::size_t>(i)] = static_cast<int>(i);
      });
}
```
