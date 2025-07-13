-- set minimum xmake version
set_xmakever("2.8.2")

-- includes
includes("lib/commonlibsse-ng")

-- set project
set_project("modex")
set_version("1.2.3")
set_license("GPL-3.0")

-- set defaults
set_languages("c++23")
set_warnings("allextra")

-- set policies
set_policy("package.requires_lock", true)

-- add rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode/"})

-- require packagess
add_requires("freetype", "nlohmann_json v3.12.0", "simpleini", "fmt")
add_requires("imgui v1.91.5", {configs = {dx11 = true, win32 = true}})

-- targets
target("modex")
    -- add dependencies to target
    add_deps("commonlibsse-ng")

    -- add packages to target
    add_packages("freetype", "nlohmann_json", "simpleini", "fmt")
    add_packages("imgui", {configs = {dx11 = true, win32 = true}})

    -- add commonlibsse-ng plugin
    add_rules("commonlibsse-ng.plugin", {
        name = "modex",
        author = "patchuli",
        description = "SKSE64 plugin template using CommonLibSSE-NG"
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")