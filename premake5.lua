workspace "cpu_emu"
    configurations {"Debug", "Release"}

project "cpuemu"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin"
    cppdialect "C++17"
    toolset "gcc"

    linkoptions { '-static-libstdc++', '-static-libgcc' }
    files { "./src/**.h", "./src/**.cpp" }
    defines { "BOT_VERSION=\"2.0.1\"" }
    includedirs { 
        "/usr/local/include/",
        "/usr/include/",
        "./src/"
    }
    links {
    }

    filter "configurations:Debug"
        buildoptions { "-rdynamic" }
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        optimize "On"