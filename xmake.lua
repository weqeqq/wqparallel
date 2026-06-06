set_project("weqeqq.parallel")
set_version("0.2.2")

add_rules("plugin.compile_commands.autoupdate")

set_policy("build.c++.modules.std", false)

set_languages("c++23")

option("tests")
    set_default(false)
    set_showmenu(true)
    set_description("Build tests")
option_end()

target("weqeqq.parallel")
    set_kind("static")
    add_files("sources/weqeqq/**.cppm", { public = true })

if has_config("tests") then
    includes("tests")
end
