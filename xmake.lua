-- set minimum xmake version
set_xmakever("2.8.2")

-- includes
includes("lib/commonlibsse-ng")

-- set project
set_project("Modex - A Mod Explorer Menu")
set_version("2.0.0")
set_license("GPL-3.0")

-- set defaults
set_languages("c++23")
set_warnings("allextra")

-- set policies
set_policy("package.requires_lock", true)

-- add rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")
add_rules("plugin.compile_commands.autoupdate")

add_requires("freetype")
add_requires("nlohmann_json v3.12.0")
add_requires("simpleini")
add_requires("imm32")

-- imgui package @ 21d329 (post 1.92.5 update).
add_requires("imgui 21d3299e588b5c702dcca0f448b4f937af369b4a", {configs = {win32 = true, dx11 = true}})

-- explicitly define macros
add_defines("ENABLE_SKYRIM_SE=1")
add_defines("ENABLE_SKYRIM_AE=1")
add_undefines("ENABLE_SKYRIM_VR=1")

-- targets
target("Modex - A Mod Explorer Menu")
    add_deps("commonlibsse-ng")

    -- additional packages
    add_packages("freetype")
    add_packages("nlohmann_json")
    add_packages("simpleini")
    add_packages("imgui")
    add_packages("imm32")
    
    -- add commonlibsse-ng plugin
    add_rules("commonlibsse-ng.plugin", {
        name = "Modex - A Mod Explorer Menu",
        author = "Patchuli",
        description = "SKSE64 plugin template using CommonLibSSE-NG"
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")

    -- distribute
    after_build(function (target)
        local project_name = target:name()
        local mods_path = os.getenv("MO2_MODS_FOLDER")
        local dist_path = os.projectdir() .. "/dist/"

        -- create plugin directory structure
        os.mkdir(path.join(dist_path, "SKSE", "Plugins"))
        os.mkdir(path.join(dist_path, "Interface", "Modex", "language"))
        os.mkdir(path.join(dist_path, "Interface", "Modex", "user"))

        -- copy plugin to project distributable folder
        os.cp(target:targetfile(), path.join(dist_path, "SKSE", "Plugins"))

        -- copy pdb to project distributable folder
        os.cp(target:targetfile():gsub("%.dll$", ".pdb"), path.join(dist_path, "SKSE", "Plugins"))

        -- copy folders and files from dist to MO2 mods folder if it exists
        os.cp(dist_path .. "/*", path.join(mods_path, project_name))
    end)
