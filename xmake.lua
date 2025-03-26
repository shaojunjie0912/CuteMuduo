set_project("CuteMuduo")
set_xmakever("2.9.8")

set_languages("c++20")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")

set_warnings("allextra")
set_defaultmode("debug")

includes("CuteMuduo")
includes("tests")
