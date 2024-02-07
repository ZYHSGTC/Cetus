project "CetusApp"
	kind "ConsoleApp"
	language "C++"
	cppdialect "c++17"

	  -- �������ɶ������ļ���λ�ã��������ڵ�ǰĿ¼�µ�bin�ļ����У�����������ѡ������ļ���
	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	-- ������Ŀ��Դ�ļ���ͷ�ļ�������ʹ��ͨ��������������src�ļ����е�.h��.cpp�ļ�
	files 
	{	
	}
	-- ������Ŀ�ĸ��Ӱ���Ŀ¼�����������include�ļ�����Ϊͷ�ļ�������·��
	includedirs 
	{
		"../vendor/imgui",
		"../vendor/glfw/include",
		"../vendor/ktx/include",
		"../vendor/tinygltf",		--�ü��ϲ���ʹ������⣬ע��..
		"../Cetus/src",

		"D:/Vulkan/Include",

		"%{prj.name}/src",
		"%{prj.name}"
	}

	links
    {
        "Cetus"
    }

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