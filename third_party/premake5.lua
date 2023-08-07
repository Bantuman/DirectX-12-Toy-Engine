shared_includes = 
{
    "../third_party/DirectXTex/DirectXTex", 
    "../third_party/DirectXTex/DDSTextureLoader",
    "../third_party/imgui",
    "../third_party"
}

project "External"
    location "%{wks.location}/third_party"
    language "C++"
    cppdialect "C++20"

    targetdir("../lib/")
    targetname("%{prj.name}_%{cfg.buildcfg}")
    objdir("../temp/%{prj.name}_%{cfg.buildcfg}")

    defines {
        "FASTGLTF_IS_X86"
    }

    undefines {
    }

    files {
        "**.h",
        "**.hpp",
        "**.cpp",
    }
    
    --vpaths {
    --    ["Precompiled Headers"] = {"**pch.h", "**pch.cpp"}
    --}

    includedirs {
       ".",
       "imgui",
       "spdlog",
       "signals",
       "simdjson",
       "DirectXTex",
       "DirectXTex/DirectXTex",
       "$(WindowsSDK_IncludePath)"
    }

    libdirs {
        "../lib/",
    }
    
    links {
    }
    disablewarnings { "4702" }
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
			--"_CRT_SECURE_NO_WARNINGS", 
			"_LIB", 
			"_WIN32_WINNT=0x0601",
			"SYSTEM_WINDOWS"
		}
    