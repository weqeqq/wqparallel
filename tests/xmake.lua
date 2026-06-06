add_requires("gtest")

target("tests")
    set_kind("binary")
    add_packages("gtest")
    add_deps("weqeqq.parallel")
    add_files("sources/*.cc")
