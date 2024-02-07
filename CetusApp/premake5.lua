project "CetusApp"
	kind "ConsoleApp"
	language "C++"
	cppdialect "c++17"

	  -- 设置生成二进制文件的位置，这里是在当前目录下的bin文件夹中，并根据配置选项创建子文件夹
	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	-- 设置项目的源文件和头文件这里是使用通配符来添加所有在src文件夹中的.h和.cpp文件
	files 
	{	
	}
	-- 设置项目的附加包含目录，这里是添加include文件夹作为头文件的搜索路径
	includedirs 
	{
		"../vendor/imgui",
		"../vendor/glfw/include",
		"../vendor/ktx/include",
		"../vendor/tinygltf",		--得加上才能使用这个库，注意..
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
    -- 开启调试符号，方便调试程序
    symbols "On"
	staticruntime "on"
	runtime "Debug"

filter "configurations:Release"
    -- 开启优化代码，提高程序性能
    optimize "On"
	staticruntime "on"
    runtime "Debug"