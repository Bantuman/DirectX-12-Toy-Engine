

project "Engine"
    location "%{wks.location}/src"
    language "C++"
    cppdialect "C++20"

    targetdir("../lib/")
    targetname("%{prj.name}_%{cfg.buildcfg}")
    objdir("../temp/%{prj.name}_%{cfg.buildcfg}")

    pchheader "enginepch.h"
    pchsource "enginepch.cpp"

    defines {
    }

    files {
        "enginepch.h",
        "enginepch.cpp",

        "Engine/**.h",
        "Engine/**.hpp",
        "Engine/**.cpp",
        "Shaders/**.hlsl",
        "Shaders/**.hlsli",
    }
    disablewarnings { "4702" }
    vpaths {
        ["Precompiled Headers"] = {"**pch.h", "**pch.cpp"}
    }

    includedirs {
       ".",
       "Engine/",
       "Engine/Core",
       "$(WindowsSDK_IncludePath)",
        -- External includes
        table.unpack(shared_includes)
    }

    libdirs {
        "../lib/",
        "$(WindowsSDK_LibraryPath_x64)"

    }
    
    links {
        "d3d12",
        "dxgi",
        "dxguid",
        "D3DCompiler",
        "shlwapi",
        "External"
    }

    filter "configurations:Debug"
        defines {"_DEBUG", "%{wks.name}_DEBUG"}
        runtime "Debug"
        symbols "on"
        links {
        }
    filter "configurations:Release"
        defines {"_RELEASE", "%{wks.name}_RELEASE"}
        runtime "Release"
        optimize "on"
        links {
        }
    filter "configurations:Retail"
        defines {"_RETAIL", "%{wks.name}_RETAIL"}
        runtime "Release"
        optimize "on"
        links {
        }

    filter "system:windows"
        kind "StaticLib"
        staticruntime "off"
        symbols "on"
        systemversion "latest"
       -- warnings "Extra"
        sdlchecks "true"
        flags { 
		--	"FatalCompileWarnings",
			"MultiProcessorCompile"
		}
		links {
		--	"DXGI"
		}

		defines {
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS", 
			"_LIB", 
			"_WIN32_WINNT=0x0601",
			"SYSTEM_WINDOWS"
		}
    
    filter "files:**.hlsl"
        shadermodel "6.0"
        shaderobjectfileoutput "../bin/Shaders/%{file.basename}.cso"
    filter "files:**PS.hlsl"
        shadertype "Pixel"
    filter "files:**VS.hlsl"
        shadertype "Vertex"
    filter "files:**GS.hlsl"
        shadertype "Geometry"
    filter "files:**CS.hlsl"
        shadertype "Compute"
       -- buildcommands('"'.. vulkan_sdk ..'/Bin/glslangValidator.exe" -V -o "../shaders/%{file.name}.spv" "%{file.relpath}"')

-- project "Editor"
--     location "%{wks.location}/src"
--     language "C++"
--     cppdialect "C++20"

--     targetdir("../lib/")
--     targetname("%{prj.name}_%{cfg.buildcfg}")
--     objdir("../temp/%{prj.name}_%{cfg.buildcfg}")

--     defines { }

--     files {
--         "editor/**.h",
--         "editor/**.hpp",
--         "editor/**.cpp",
--         "ui/**.h",
--         "ui/**.hpp",
--         "ui/**.cpp",
--     }

--     includedirs {
--         ".",
--         -- External includes
--         table.unpack(shared_includes)
--     }

--     libdirs {
--     }
    
--     links {
--         "Engine",
--     }

--     filter "configurations:Debug"
--         defines {"_DEBUG", "%{wks.name}_DEBUG"}
--         runtime "Debug"
--         symbols "on"
--    --[[ filter "configurations:Release"
--         defines {"_RELEASE", "%{wks.name}_RELEASE"}
--         runtime "Release"
--         optimize "on"
--     filter "configurations:Retail"
--         defines {"_RETAIL", "%{wks.name}_RETAIL"}
--         runtime "Release"
--         optimize "on"--]]

--     filter "system:windows"
--         kind "StaticLib"
--         staticruntime "off"
--         symbols "on"
--         systemversion "latest"
--         -- warnings "Extra"
--         sdlchecks "true"
--         flags { 
--         --	"FatalCompileWarnings",
--             "MultiProcessorCompile"
--         }
--         links {
--         }

--         defines {
--             "WIN32",
--             "_CRT_SECURE_NO_WARNINGS", 
--             "_LIB", 
--             "_WIN32_WINNT=0x0601",
--             "SYSTEM_WINDOWS"
--         }
    
project "Game"
    location "%{wks.location}/src/"
    kind "WindowedApp"--"WindowedApp" (it forces win32)
    language "C++"
    cppdialect "C++20"
   -- flags {"/SUBSYSTEM:CONSOLE"}

    pchheader "gamepch.h"
    pchsource "gamepch.cpp"

    targetdir("../bin/")
    debugdir("../bin/")
    targetname("%{prj.name}_%{cfg.buildcfg}")
    objdir("../temp/%{prj.name}/%{cfg.buildcfg}")
    
    files {
       -- "main.cpp",
        "gamepch.h",
        "gamepch.cpp",

        "Game/**.h",
        "Game/**.hpp",
        "Game/**.cpp",
        --[[
        "ui/**.h",
        "ui/**.hpp",
        "ui/**.cpp",
        "editor/**.h",
        "editor/**.hpp",
        "editor/**.cpp"
        --]]
    }

    vpaths {
        ["Precompiled Headers"] = {"**pch.h", "**pch.cpp"}
    }
    
    removefiles {
       -- "game/gamepch.cpp"
    }

    includedirs {
        ".",
        "Game/",
        "$(WindowsSDK_IncludePath)",
        -- External includes
        table.unpack(shared_includes)
    }

    links {
         "Engine",
        --"CommonUtilities"
    }
    disablewarnings {"4201", "4702"}
    libdirs { 
        "../lib/",
        "$(WindowsSDK_LibraryPath_x64)"
    }
    filter "configurations:Debug"
        defines {"_DEBUG", "%{wks.name}_DEBUG"}
        runtime "Debug"
        symbols "on"
        links { --[["Editor"]] }
    filter "configurations:Release"
        defines {"_RELEASE", "%{wks.name}_RELEASE"}
        runtime "Release"
        optimize "on"
    filter "configurations:Retail"
        defines {"_RETAIL", "%{wks.name}_RETAIL"}
        runtime "Release"
        optimize "on"
    
    filter "system:windows"
        symbols "on"
        systemversion "latest"
        warnings "Extra"
        flags { 
			"FatalCompileWarnings",
			"MultiProcessorCompile"
		}

		defines {
			"WIN32",
            "_CRT_SECURE_NO_WARNINGS",
			"_LIB",
			"SYSTEM_WINDOWS"
		}