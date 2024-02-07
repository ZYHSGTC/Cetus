workspace "Cetus"
	architecture "x86_64"
    -- ����������Ŀ�ļ���λ�ã��������ڵ�ǰĿ¼�µ�build�ļ�����
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