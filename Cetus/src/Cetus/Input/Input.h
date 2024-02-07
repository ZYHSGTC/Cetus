#pragma once

#include "KeyCodes.h"

#include <glm/glm.hpp>

namespace Cetus {

	class Input												// 声明一个名为Input的类，用于封装输入相关的功能
	{
	public:
		static bool IsKeyDown(KeyCode keycode);				// 定义一个静态函数，用于判断给定的键码是否被按下
		static bool IsMouseButtonDown(MouseButton button);	// 定义一个静态函数，用于判断给定的鼠标按钮是否被按下

		static glm::vec2 GetMousePosition();				// 定义一个静态函数，用于获取鼠标的位置，返回一个二维向量

		static void SetCursorMode(CursorMode mode);			// 定义一个静态函数，用于设置鼠标的模式，如正常、隐藏、捕捉等
	};

}
