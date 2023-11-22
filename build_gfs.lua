project "gfs"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    files { "src/**.hpp", "src/**.cpp", "include/**.hpp" }

    includedirs 
    { 
        "src", 
        "include",
        -- LZ4
        "external/lz4/include"
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
