#include "Input.h"

#include "Cetus/Application.h"

#include <GLFW/glfw3.h>

namespace Cetus {

	bool Input::IsKeyDown(KeyCode keycode)								// ����Input��ľ�̬����IsKeyDown�������жϸ����ļ����Ƿ񱻰���
	{
		GLFWwindow* windowHandle = Application::Get().GetWindowHandle();// ��ȡ��ǰ�Ĵ��ھ����ʹ��Application��ľ�̬����Get
		int state = glfwGetKey(windowHandle, (int)keycode);				// ����GLFW��ĺ���glfwGetKey�����봰�ھ���ͼ��룬���ؼ���״̬
		return state == GLFW_PRESS || state == GLFW_REPEAT;				// �жϼ���״̬�Ƿ�Ϊ���»��ظ������ز���ֵ
	}

	bool Input::IsMouseButtonDown(MouseButton button)					// ����Input��ľ�̬����IsMouseButtonDown�������жϸ�������갴ť�Ƿ񱻰���
	{
		GLFWwindow* windowHandle = Application::Get().GetWindowHandle();// ��ȡ��ǰ�Ĵ��ھ����ʹ��Application��ľ�̬����Get
		int state = glfwGetMouseButton(windowHandle, (int)button);		// ����GLFW��ĺ���glfwGetMouseButton�����봰�ھ������갴ť�����ذ�ť��״̬
		return state == GLFW_PRESS;										// �жϰ�ť��״̬�Ƿ�Ϊ���£����ز���ֵ
	}

	glm::vec2 Input::GetMousePosition()									// ����Input��ľ�̬����GetMousePosition�����ڻ�ȡ����λ�ã�����һ����ά����
	{
		GLFWwindow* windowHandle = Application::Get().GetWindowHandle();// ��ȡ��ǰ�Ĵ��ھ����ʹ��Application��ľ�̬����Get

		double x, y;													// ��������˫���ȸ�����������ڴ洢����x��y����
		glfwGetCursorPos(windowHandle, &x, &y);							// ����GLFW��ĺ���glfwGetCursorPos�����봰�ھ�������������ĵ�ַ����ȡ����λ��
		return { (float)x, (float)y };									// ����һ����ά��������x��yת��Ϊ�����ȸ�����
	}

	void Input::SetCursorMode(CursorMode mode)							// ����Input��ľ�̬����SetCursorMode��������������ģʽ�������������ء���׽��
	{
		GLFWwindow* windowHandle = Application::Get().GetWindowHandle();// ��ȡ��ǰ�Ĵ��ھ����ʹ��Application��ľ�̬����Get
		glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);// ����GLFW��ĺ���glfwSetInputMode�����봰�ھ�������ģʽ��ѡ������ģʽ��ֵ����������ģʽ
	}

}