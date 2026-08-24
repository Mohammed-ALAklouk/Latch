workspace "Latch"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "Latch"

project "Latch"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    targetdir "bin/%{cfg.buildcfg}"
    objdir "bin/obj/%{cfg.buildcfg}"

    files {
        "src/**.h",
        "src/**.cpp",
        "vendor/imgui/*.h",
        "vendor/imgui/*.cpp",
    }

    includedirs {
        "src",
        "vendor/raylib/include",
        "vendor/imgui",
    }

    libdirs {
        "vendor/raylib/lib",
    }

    links {
        "raylib",
        "winmm",
        "gdi32",
        "opengl32",
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "on"
