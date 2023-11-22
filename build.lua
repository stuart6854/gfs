-- premake5.lua
workspace "gfs"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "GFS"

    -- Workspace-wide build options for MSVC
    filter "system:windows"
        buildoptions { "" }

    OutputDir = "%{cfg.system}/%{cfg.buildcfg}"

    include "build_gfs.lua"

    group "Tests"
        include "testbed/build_testbed.lua"

    group "Benchmarks"