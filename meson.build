project(
  'wqparallel',
  ['cpp'],
  version: '0.2.0',
  license: 'MIT',
  default_options: ['cpp_std=c++23', 'warning_level=3'],
)

pkgconfig = import('pkgconfig')
wqparallel_inc = include_directories('headers')
docs_opt = get_option('docs')
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
    wqparallel_tests = executable(
      'wqparallel_tests',
      'tests/wqparallel_tests.cc',
      include_directories: wqparallel_inc,
      dependencies: gtest_dep,
    )
    test('wqparallel', wqparallel_tests, protocol: 'gtest', suite: 'unit')
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

doxygen = find_program('doxygen', required: docs_opt)
if doxygen.found()
  docs_conf = configuration_data()
  docs_conf.set('PROJECT_NAME', meson.project_name())
  docs_conf.set('PROJECT_VERSION', meson.project_version())
  docs_conf.set_quoted('PROJECT_DOC_OUTPUT', meson.project_build_root() / 'docs' / 'doxygen')
  docs_conf.set_quoted('PROJECT_MAINPAGE', meson.project_source_root() / 'docs' / 'mainpage.md')
  docs_conf.set(
    'PROJECT_INPUTS',
    '"@0@" "@1@"'.format(
      meson.project_source_root() / 'headers',
      meson.project_source_root() / 'docs' / 'mainpage.md',
    ),
  )
  docs_conf.set_quoted('PROJECT_STRIP_FROM_PATH', meson.project_source_root())

  doxyfile = configure_file(
    input: 'docs/Doxyfile.in',
    output: 'Doxyfile',
    configuration: docs_conf,
  )

  run_target(
    'docs',
    command: [doxygen, doxyfile],
  )
endif
