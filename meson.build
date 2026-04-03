project(
  'wqparallel',
  ['cpp'],
  version: '0.2.2',
  license: 'MIT',
  default_options: ['cpp_std=c++23,c++latest', 'warning_level=3'],
)

pkgconfig = import('pkgconfig')
wqparallel_inc = include_directories('headers')
tests_opt = get_option('tests')

wqparallel_dep = declare_dependency(
  include_directories: wqparallel_inc,
)
meson.override_dependency('wqparallel', wqparallel_dep)

install_headers('headers/weqeqq/parallel.h', subdir: 'weqeqq')
install_headers(
  'headers/weqeqq/parallel/execution.h',
  'headers/weqeqq/parallel/for_each.h',
  'headers/weqeqq/parallel/thread_pool.h',
  subdir: 'weqeqq/parallel',
)

pkgconfig.generate(
  name: 'wqparallel',
  filebase: 'wqparallel',
  description: 'Header-only parallel helpers for C++23',
  version: meson.project_version(),
  subdirs: '.',
)

if not tests_opt.disabled()
  gtest_dep = dependency('gtest', main: true, required: false)
  if not gtest_dep.found()
    gtest_dep = dependency('gtest_main', required: tests_opt)
  endif

  if gtest_dep.found()
    subdir('tests')
  elif tests_opt.enabled()
    error(
      'System GoogleTest was not found. Install gtest/gtest_main via your system package manager or configure with -Dtests=disabled.',
    )
  else
    message(
      'System GoogleTest not found; skipping test target because -Dtests=auto.',
    )
  endif
endif
