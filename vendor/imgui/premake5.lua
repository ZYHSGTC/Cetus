workspace"ImGui"
   architecture "x86_64"
   configurations { "Debug", "Release" }

project "ImGui"
	kind "StaticLib"
	language "C++"
    staticruntime "on"
	targetdir ("bin")
	objdir ("bin-int")

	files
	{
		"imconfig.h",
		"imgui.h",
		"imgui.cpp",
		"imgui_draw.cpp",
		"imgui_internal.h",
		"imgui_tables.cpp",
		"imgui_widgets.cpp",
		"imstb_rectpack.h",
		"imstb_textedit.h",
		"imstb_truetype.h",
		"imgui_demo.cpp"
	}

filter "system:windows"
	systemversion "latest"
	cppdialect "C++17"

filter "configurations:Debug"
    -- �������Է��ţ�������Գ���
    symbols "On"
	staticruntime "on"
	runtime "Debug"

filter "configurations:Release"
    -- �����Ż����룬��߳�������
    optimize "On"
	staticruntime "on"
    runtime "Debug"
