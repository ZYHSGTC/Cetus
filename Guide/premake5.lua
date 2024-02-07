project "Guide"
	kind "ConsoleApp"
	language "C++"
	cppdialect "c++17"
	  -- �������ɶ������ļ���λ�ã��������ڵ�ǰĿ¼�µ�bin�ļ����У�����������ѡ������ļ���
	targetdir ("bin/%{cfg.buildcfg}")
	objdir ("bin-int/%{cfg.buildcfg}")
	--buildoptions { "/bigobj" }
	--defines { "WIN32" }

	-- ������Ŀ��Դ�ļ���ͷ�ļ�������ʹ��ͨ��������������src�ļ����е�.h��.cpp�ļ�

	-- ������Ŀ�ĸ��Ӱ���Ŀ¼�����������include�ļ�����Ϊͷ�ļ�������·��
	includedirs 
	{
		"../vendor",
		"../vendor/stb",
		"../vendor/tiny_obj_loader",
		"../vendor/glfw",
		"D:/Vulkan/Include"
	}
	--���ø��ӿ�Ŀ¼
	libdirs 
	{ 
		"vendor/glfw/lib",
		"D:/Vulkan/Lib"
	}
	-- ������Ŀ��Ҫ���ӵĿ⣬����������GLFW��OpenGL��
	links {"vulkan-1.lib", "glfw3.lib"}

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