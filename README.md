# wqparallel

`wqparallel` is a header-only C++23 library with a small thread pool and
parallel `for_each`/`transform` helpers.

## Features

- Header-only distribution.
- `ThreadPool` with `Submit` and `ParallelFor`.
- Exception propagation from `Submit` and `ParallelFor`.
- `ForEachIndex`, `ForEach`, and `Transform` helpers.
- Automatic sequential fallback for non-random-access iterators.

## Build And Test

```sh
meson setup build
meson compile -C build
meson test -C build --print-errorlogs
```

The test suite uses the system `gtest` package. Install it with your platform
package manager before configuring the build, or disable tests explicitly:

```sh
meson setup build -Dtests=disabled
```

`-Dtests=enabled` requires system GoogleTest to be present. `-Dtests=auto`
builds the tests only when the dependency is available. The test sources live in
`tests/sources`, and `tests/meson.build` registers them as separate gtest
targets.

## Install

```sh
meson install -C build
```

This installs the public headers and a `pkg-config` file named
`wqparallel.pc`.

## Usage

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

With Meson as a subproject:

```meson
wqparallel_dep = dependency('wqparallel', fallback: ['wqparallel', 'wqparallel_dep'])
```

With `pkg-config` after installation:

```sh
pkg-config --cflags wqparallel
```

## Notes

- Parallel iterator algorithms use indexed access when available.
- Non-random-access iterators transparently fall back to the sequential
  standard algorithms.
