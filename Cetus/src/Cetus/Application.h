#pragma once

#include "Layer.h"
#include "base/VulkanDebug.h"
#include "base/VulkanDevice.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "imgui.h"
#include "vulkan/vulkan.h"

void check_vk_result(VkResult err);			// 声明一个函数，用于检查Vulkan的返回结果是否为成功，如果不是，则抛出异常

struct GLFWwindow;							// 前置声明一个结构体，用于表示GLFW库中的窗口对象 struct GLFWwindow;

namespace Cetus {

	struct ApplicationSpecification			// 定义一个结构体，用于表示应用程序的规格，包括名称、宽度、高度等
	{
		std::string Name = "Cetus App";		// 定义一个std::string类型的成员变量，用于存储应用程序的名称，默认为"Cetus App"
		uint32_t Width = 1600;				// 定义一个uint32_t类型的成员变量，用于存储应用程序的宽度，默认为1600
		uint32_t Height = 900;				// 定义一个uint32_t类型的成员变量，用于存储应用程序的高度，默认为900
	};


	class Application						// 声明一个名为Application的类，用于封装应用程序相关的功能
	{
	public:
		// 定义一个构造函数，用于创建一个Application对象，接受一个ApplicationSpecification类型的参数，用于指定应用程序的规格，默认为ApplicationSpecification()
		Application(const ApplicationSpecification& applicationSpecification = ApplicationSpecification());
		~Application();				// 定义一个析构函数，用于销毁一个Application对象

		static Application& Get();	// 定义一个静态函数，用于获取当前的Application对象的引用

		void Run();					// 定义一个成员函数，用于运行应用程序的主循环
		// 定义一个成员函数，用于设置菜单栏的回调函数，接受一个std::function<void()>类型的参数，用于指定菜单栏的功能
		void SetMenubarCallback(const std::function<void()>& menubarCallback) { m_MenubarCallback = menubarCallback; }
		
		template<typename T>		// 定义一个模板成员函数，用于向图层栈中添加一个图层，使用模板参数T作为图层的类型，要求T必须是Layer的子类
		void PushLayer()			// 使用static_assert语句，检查T是否是Layer的子类，如果不是，则编译时报错
		{							// 使用std::make_shared函数，创建一个T类型的智能指针，并将其添加到图层栈的末尾，然后调用其OnAttach函数
			// is_base_of函数是一个模板类，用于测试一个类型是否是另一个类型的基类。它的语法是：template <class Base, class Derived> struct is_base_of;
			//其中Base是要测试的基类，Derived是要测试的派生类。如果Derived是从Base继承或者两者是相同的非联合类（忽略cv限定符），那么is_base_of<Base, Derived>::value就是true，否则是false
			static_assert(std::is_base_of<Layer, T>::value, "Pushed type is not subclass of Layer!");
			// 在C++14之后，emplace_back可以返回对插入元素的引用。
			m_LayerStack.emplace_back(std::make_shared<T>())->OnAttach();
		}

		// 定义一个成员函数，用于向图层栈中添加一个图层，接受一个std::shared_ptr<Layer>类型的参数，用于指定图层的指针
		void PushLayer(const std::shared_ptr<Layer>& layer) { m_LayerStack.emplace_back(layer); layer->OnAttach(); }

		void Close();													// 定义一个成员函数，用于关闭应用程序

		float GetTime();												// 定义一个成员函数，用于获取应用程序的运行时间，返回一个浮点数
		GLFWwindow* GetWindowHandle() const { return m_WindowHandle; }	// 定义一个成员函数，用于获取应用程序的窗口句柄，返回一个GLFWwindow类型的指针，使用const修饰，表示不会修改对象的状态

		static VkInstance GetInstance();								// 定义一个静态函数，用于获取Vulkan的实例对象，返回一个VkInstance类型的值
		static VkPhysicalDevice GetPhysicalDevice();					// 定义一个静态函数，用于获取Vulkan的物理设备对象，返回一个VkPhysicalDevice类型的值
		static VkDevice GetDevice();									// 定义一个静态函数，用于获取Vulkan的逻辑设备对象，返回一个VkDevice类型的值

		static VkCommandBuffer GetCommandBuffer(bool begin);			// 定义一个静态函数，用于获取Vulkan的命令缓冲对象，接受一个布尔值作为参数，用于指定是否开始记录命令，返回一个VkCommandBuffer类型的值
		static void FlushCommandBuffer(VkCommandBuffer commandBuffer);	// 定义一个静态函数，用于提交Vulkan的命令缓冲对象，接受一个VkCommandBuffer类型的参数，用于指定要提交的命令缓冲
		// 右值是一种表达式的值类别，表示一个不具名且可被移动的表达式。右值一般是不可寻址的常量，或在表达式求值过程中创建的无名临时对象，短暂性的。右值不能出现在赋值表达式的左边，也不能被修改。右值可以用来初始化右值引用，实现移动语义，提高程序性能12。
		//	例如，以下表达式的值都是右值：
		//	字面值(字符串字面值除外)，例如1，‘a’, true等
		//	返回值为非引用的函数调用或操作符重载，例如：str.substr(1, 2), str1 + str2, or it++
		//	后置自增和自减表达式(a++, a–)
		//	算术表达式
		//	逻辑表达式
		//	比较表达式
		//	取地址表达式
		//	lambda表达式
		static void SubmitResourceFree(std::function<void()>&& func);	// 定义一个静态函数，用于提交资源释放的函数，接受一个std::function<void()>类型的右值引用作为参数，用于指定要执行的资源释放的函数

		struct AppSettings {
			bool enableValidationLayers = true;
		}m_Settings;
	private:
		void Init();										// 定义一个私有成员函数，用于初始化应用程序
		void Shutdown();									// 定义一个私有成员函数，用于关闭应用程序
	private:
		ApplicationSpecification m_Specification;			// 定义一个ApplicationSpecification类型的私有成员变量，用于存储应用程序的规格
		GLFWwindow* m_WindowHandle = nullptr;				// 定义一个GLFWwindow类型的私有成员变量，用于存储应用程序的窗口句柄，初始化为nullptr
		bool m_Running = false;								// 定义一个布尔类型的私有成员变量，用于存储应用程序的运行状态，初始化为false

		float m_TimeStep = 0.0f;							// 定义一个浮点类型的私有成员变量，用于存储应用程序的时间步长，初始化为0.0f
		float m_FrameTime = 0.0f;							// 定义一个浮点类型的私有成员变量，用于存储应用程序的帧时间，初始化为0.0f
		float m_LastFrameTime = 0.0f;						// 定义一个浮点类型的私有成员变量，用于存储应用程序的上一帧时间，初始化为0.0f

		std::vector<std::shared_ptr<Layer>> m_LayerStack;	// 定义一个std::vector<std::shared_ptr<Layer>>类型的私有成员变量，用于存储应用程序的图层栈
		std::function<void()> m_MenubarCallback;			// 定义一个std::function<void()>类型的私有成员变量，用于存储应用程序的菜单栏的回调函数
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);	// 声明一个全局函数，用于由客户端实现，创建一个Application对象，接受两个参数，分别是命令行参数的个数和值，返回一个Application类型的指针
}