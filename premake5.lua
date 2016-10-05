workspace "minecraft-unicode-font-generator"
	configurations { "Debug", "Release" }

project "minecraft-unicode-font-generator"
	kind "ConsoleApp"
	language "C++"
	targetdir "bin/%{cfg.buildcfg}"
	includedirs { "/usr/local/include/freetype2/", "/usr/local/include/tclap/" }
	buildoptions "-std=c++14"

	links { "png",  "freetype" }

	files { "**.hh", "**.cpp" }

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Speed"
