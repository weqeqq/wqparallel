test_sources = {
  'for_each_index': files('sources/for_each_index_tests.cc'),
  'for_each': files('sources/for_each_tests.cc'),
  'transform': files('sources/transform_tests.cc'),
  'thread_pool': files('sources/thread_pool_tests.cc'),
  'policy': files('sources/policy_tests.cc'),
}

foreach test_name, test_source : test_sources
  test_exe = executable(
    test_name,
    test_source,
    include_directories: wqparallel_inc,
    dependencies: gtest_dep,
  )

  test(
    test_name,
    test_exe,
    is_parallel: false,
    protocol: 'gtest',
    suite: 'unit',
  )
endforeach
