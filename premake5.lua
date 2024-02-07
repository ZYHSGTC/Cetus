workspace "Cetus"
	architecture "x86_64"
    -- 设置生成项目文件的位置，这里是在当前目录下的build文件夹中
	configurations {"Debug","Release"}
	startproject "CetusApp"
	characterset "MBCS"

	filter "system:windows"
		systemversion "latest"
		defines { "WL_PLATFORM_WINDOWS" }
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Guide"
include "CetusApp"
include "Cetus"