project "Testbed"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    files { "**.hpp", "**.cpp" }

    includedirs 
    { 
        "src", 
        "include",
        -- Include GFS
        "../include" 
    }
    libdirs
    {
        "../external/**/lib"
    }    

    links 
    {
        "GFS",
        -- LZ4
        "lz4"
    }

    targetdir("..bin/" .. OutputDir .. "/%{prj.name}")
    objdir("../bin/objs/" .. OutputDir .. "/%{prj.name}")

    filter "system:windows"
        systemversion "latest"
        defines {}

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        symbols "Off"
