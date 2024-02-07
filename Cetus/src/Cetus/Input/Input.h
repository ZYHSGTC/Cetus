#pragma once

#include "KeyCodes.h"

#include <glm/glm.hpp>

namespace Cetus {

	class Input												// ����һ����ΪInput���࣬���ڷ�װ������صĹ���
	{
	public:
		static bool IsKeyDown(KeyCode keycode);				// ����һ����̬�����������жϸ����ļ����Ƿ񱻰���
		static bool IsMouseButtonDown(MouseButton button);	// ����һ����̬�����������жϸ�������갴ť�Ƿ񱻰���

		static glm::vec2 GetMousePosition();				// ����һ����̬���������ڻ�ȡ����λ�ã�����һ����ά����

		static void SetCursorMode(CursorMode mode);			// ����һ����̬������������������ģʽ�������������ء���׽��
	};

}
