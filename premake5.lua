workspace "cpu_emu"
    configurations {"Debug", "Release"}

project "cpuemu"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin"
    cppdialect "C++17"
    toolset "gcc"
	architecture "x86_64"

    files { "./src/**.h", "./src/**.cpp" }
    includedirs { 
        "/usr/local/include/",
        "/usr/include/"
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        optimize "On"