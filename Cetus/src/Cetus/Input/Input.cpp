#include "Input.h"

#include "Cetus/Application.h"

#include <GLFW/glfw3.h>

namespace Cetus {

	bool Input::IsKeyDown(KeyCode keycode)								// 定义Input类的静态函数IsKeyDown，用于判断给定的键码是否被按下
	{
		GLFWwindow* windowHandle = Application::Get().GetWindowHandle();// 获取当前的窗口句柄，使用Application类的静态函数Get
		int state = glfwGetKey(windowHandle, (int)keycode);				// 调用GLFW库的函数glfwGetKey，传入窗口句柄和键码，返回键的状态
		return state == GLFW_PRESS || state == GLFW_REPEAT;				// 判断键的状态是否为按下或重复，返回布尔值
	}

	bool Input::IsMouseButtonDown(MouseButton button)					// 定义Input类的静态函数IsMouseButtonDown，用于判断给定的鼠标按钮是否被按下
	{
		GLFWwindow* windowHandle = Application::Get().GetWindowHandle();// 获取当前的窗口句柄，使用Application类的静态函数Get
		int state = glfwGetMouseButton(windowHandle, (int)button);		// 调用GLFW库的函数glfwGetMouseButton，传入窗口句柄和鼠标按钮，返回按钮的状态
		return state == GLFW_PRESS;										// 判断按钮的状态是否为按下，返回布尔值
	}

	glm::vec2 Input::GetMousePosition()									// 定义Input类的静态函数GetMousePosition，用于获取鼠标的位置，返回一个二维向量
	{
		GLFWwindow* windowHandle = Application::Get().GetWindowHandle();// 获取当前的窗口句柄，使用Application类的静态函数Get

		double x, y;													// 定义两个双精度浮点变量，用于存储鼠标的x和y坐标
		glfwGetCursorPos(windowHandle, &x, &y);							// 调用GLFW库的函数glfwGetCursorPos，传入窗口句柄和两个变量的地址，获取鼠标的位置
		return { (float)x, (float)y };									// 返回一个二维向量，将x和y转换为单精度浮点数
	}

	void Input::SetCursorMode(CursorMode mode)							// 定义Input类的静态函数SetCursorMode，用于设置鼠标的模式，如正常、隐藏、捕捉等
	{
		GLFWwindow* windowHandle = Application::Get().GetWindowHandle();// 获取当前的窗口句柄，使用Application类的静态函数Get
		glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);// 调用GLFW库的函数glfwSetInputMode，传入窗口句柄、鼠标模式的选项和鼠标模式的值，设置鼠标的模式
	}

}