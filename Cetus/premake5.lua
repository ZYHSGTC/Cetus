project "Cetus"
	kind "StaticLib"
	language "C++"
	cppdialect "c++17"
	
	  -- �������ɶ������ļ���λ�ã��������ڵ�ǰĿ¼�µ�bin�ļ����У�����������ѡ������ļ���
	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	-- ������Ŀ��Դ�ļ���ͷ�ļ�������ʹ��ͨ��������������src�ļ����е�.h��.cpp�ļ�
	files 
	{	
		"src/**.h",
		"src/**.c",
		"src/**.cpp",
		"src/**.hpp"
	}
	-- ������Ŀ�ĸ��Ӱ���Ŀ¼�����������include�ļ�����Ϊͷ�ļ�������·��
	includedirs 
	{
		"../vendor",
		"../vendor/imgui",
		"../vendor/glfw",
		"../vendor/ktx/include",
		"../vendor/ktx/lib",
		"../vendor/ktx/other_include",
		"../vendor/stb",
		"../vendor/tiny_obj_loader",
		"../vendor/tinygltf",
		"D:/Vulkan/Include",
		"src"
	}
	--���ø��ӿ�Ŀ¼
	libdirs 
	{ 
		"../vendor/glfw/lib",
		"../vendor/imgui/bin",
		--"../vendor/ktx/lib",,"ktx.lib"
		--"vendor/assimp/lib",
		"D:/Vulkan/Lib"
	}
	-- ������Ŀ��Ҫ���ӵĿ⣬����������GLFW��OpenGL��
	links {"vulkan-1.lib", "glfw3.lib","ImGui.lib"}
	--"assimp-vc143-mtd.lib"

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
