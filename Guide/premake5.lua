project "Guide"
	kind "ConsoleApp"
	language "C++"
	cppdialect "c++17"
	  -- 设置生成二进制文件的位置，这里是在当前目录下的bin文件夹中，并根据配置选项创建子文件夹
	targetdir ("bin/%{cfg.buildcfg}")
	objdir ("bin-int/%{cfg.buildcfg}")
	--buildoptions { "/bigobj" }
	--defines { "WIN32" }

	-- 设置项目的源文件和头文件这里是使用通配符来添加所有在src文件夹中的.h和.cpp文件

	-- 设置项目的附加包含目录，这里是添加include文件夹作为头文件的搜索路径
	includedirs 
	{
		"../vendor",
		"../vendor/stb",
		"../vendor/tiny_obj_loader",
		"../vendor/glfw",
		"D:/Vulkan/Include"
	}
	--设置附加库目录
	libdirs 
	{ 
		"vendor/glfw/lib",
		"D:/Vulkan/Lib"
	}
	-- 设置项目需要链接的库，这里是链接GLFW和OpenGL库
	links {"vulkan-1.lib", "glfw3.lib"}

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