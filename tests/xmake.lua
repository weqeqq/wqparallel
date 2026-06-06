add_requires("gtest")

target("tests")
    set_kind("binary")
    set_default(false)
    add_packages("gtest")
    add_deps("weqeqq.parallel")
    add_files("sources/*.cc")
