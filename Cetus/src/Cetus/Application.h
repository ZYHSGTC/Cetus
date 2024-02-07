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

void check_vk_result(VkResult err);			// ����һ�����������ڼ��Vulkan�ķ��ؽ���Ƿ�Ϊ�ɹ���������ǣ����׳��쳣

struct GLFWwindow;							// ǰ������һ���ṹ�壬���ڱ�ʾGLFW���еĴ��ڶ��� struct GLFWwindow;

namespace Cetus {

	struct ApplicationSpecification			// ����һ���ṹ�壬���ڱ�ʾӦ�ó���Ĺ�񣬰������ơ���ȡ��߶ȵ�
	{
		std::string Name = "Cetus App";		// ����һ��std::string���͵ĳ�Ա���������ڴ洢Ӧ�ó�������ƣ�Ĭ��Ϊ"Cetus App"
		uint32_t Width = 1600;				// ����һ��uint32_t���͵ĳ�Ա���������ڴ洢Ӧ�ó���Ŀ�ȣ�Ĭ��Ϊ1600
		uint32_t Height = 900;				// ����һ��uint32_t���͵ĳ�Ա���������ڴ洢Ӧ�ó���ĸ߶ȣ�Ĭ��Ϊ900
	};


	class Application						// ����һ����ΪApplication���࣬���ڷ�װӦ�ó�����صĹ���
	{
	public:
		// ����һ�����캯�������ڴ���һ��Application���󣬽���һ��ApplicationSpecification���͵Ĳ���������ָ��Ӧ�ó���Ĺ��Ĭ��ΪApplicationSpecification()
		Application(const ApplicationSpecification& applicationSpecification = ApplicationSpecification());
		~Application();				// ����һ��������������������һ��Application����

		static Application& Get();	// ����һ����̬���������ڻ�ȡ��ǰ��Application���������

		void Run();					// ����һ����Ա��������������Ӧ�ó������ѭ��
		// ����һ����Ա�������������ò˵����Ļص�����������һ��std::function<void()>���͵Ĳ���������ָ���˵����Ĺ���
		void SetMenubarCallback(const std::function<void()>& menubarCallback) { m_MenubarCallback = menubarCallback; }
		
		template<typename T>		// ����һ��ģ���Ա������������ͼ��ջ�����һ��ͼ�㣬ʹ��ģ�����T��Ϊͼ������ͣ�Ҫ��T������Layer������
		void PushLayer()			// ʹ��static_assert��䣬���T�Ƿ���Layer�����࣬������ǣ������ʱ����
		{							// ʹ��std::make_shared����������һ��T���͵�����ָ�룬��������ӵ�ͼ��ջ��ĩβ��Ȼ�������OnAttach����
			// is_base_of������һ��ģ���࣬���ڲ���һ�������Ƿ�����һ�����͵Ļ��ࡣ�����﷨�ǣ�template <class Base, class Derived> struct is_base_of;
			//����Base��Ҫ���ԵĻ��࣬Derived��Ҫ���Ե������ࡣ���Derived�Ǵ�Base�̳л�����������ͬ�ķ������ࣨ����cv�޶���������ôis_base_of<Base, Derived>::value����true��������false
			static_assert(std::is_base_of<Layer, T>::value, "Pushed type is not subclass of Layer!");
			// ��C++14֮��emplace_back���Է��ضԲ���Ԫ�ص����á�
			m_LayerStack.emplace_back(std::make_shared<T>())->OnAttach();
		}

		// ����һ����Ա������������ͼ��ջ�����һ��ͼ�㣬����һ��std::shared_ptr<Layer>���͵Ĳ���������ָ��ͼ���ָ��
		void PushLayer(const std::shared_ptr<Layer>& layer) { m_LayerStack.emplace_back(layer); layer->OnAttach(); }

		void Close();													// ����һ����Ա���������ڹر�Ӧ�ó���

		float GetTime();												// ����һ����Ա���������ڻ�ȡӦ�ó��������ʱ�䣬����һ��������
		GLFWwindow* GetWindowHandle() const { return m_WindowHandle; }	// ����һ����Ա���������ڻ�ȡӦ�ó���Ĵ��ھ��������һ��GLFWwindow���͵�ָ�룬ʹ��const���Σ���ʾ�����޸Ķ����״̬

		static VkInstance GetInstance();								// ����һ����̬���������ڻ�ȡVulkan��ʵ�����󣬷���һ��VkInstance���͵�ֵ
		static VkPhysicalDevice GetPhysicalDevice();					// ����һ����̬���������ڻ�ȡVulkan�������豸���󣬷���һ��VkPhysicalDevice���͵�ֵ
		static VkDevice GetDevice();									// ����һ����̬���������ڻ�ȡVulkan���߼��豸���󣬷���һ��VkDevice���͵�ֵ

		static VkCommandBuffer GetCommandBuffer(bool begin);			// ����һ����̬���������ڻ�ȡVulkan���������󣬽���һ������ֵ��Ϊ����������ָ���Ƿ�ʼ��¼�������һ��VkCommandBuffer���͵�ֵ
		static void FlushCommandBuffer(VkCommandBuffer commandBuffer);	// ����һ����̬�����������ύVulkan���������󣬽���һ��VkCommandBuffer���͵Ĳ���������ָ��Ҫ�ύ�������
		// ��ֵ��һ�ֱ��ʽ��ֵ��𣬱�ʾһ���������ҿɱ��ƶ��ı��ʽ����ֵһ���ǲ���Ѱַ�ĳ��������ڱ��ʽ��ֵ�����д�����������ʱ���󣬶����Եġ���ֵ���ܳ����ڸ�ֵ���ʽ����ߣ�Ҳ���ܱ��޸ġ���ֵ����������ʼ����ֵ���ã�ʵ���ƶ����壬��߳�������12��
		//	���磬���±��ʽ��ֵ������ֵ��
		//	����ֵ(�ַ�������ֵ����)������1����a��, true��
		//	����ֵΪ�����õĺ������û���������أ����磺str.substr(1, 2), str1 + str2, or it++
		//	�����������Լ����ʽ(a++, a�C)
		//	�������ʽ
		//	�߼����ʽ
		//	�Ƚϱ��ʽ
		//	ȡ��ַ���ʽ
		//	lambda���ʽ
		static void SubmitResourceFree(std::function<void()>&& func);	// ����һ����̬�����������ύ��Դ�ͷŵĺ���������һ��std::function<void()>���͵���ֵ������Ϊ����������ָ��Ҫִ�е���Դ�ͷŵĺ���

		struct AppSettings {
			bool enableValidationLayers = true;
		}m_Settings;
	private:
		void Init();										// ����һ��˽�г�Ա���������ڳ�ʼ��Ӧ�ó���
		void Shutdown();									// ����һ��˽�г�Ա���������ڹر�Ӧ�ó���
	private:
		ApplicationSpecification m_Specification;			// ����һ��ApplicationSpecification���͵�˽�г�Ա���������ڴ洢Ӧ�ó���Ĺ��
		GLFWwindow* m_WindowHandle = nullptr;				// ����һ��GLFWwindow���͵�˽�г�Ա���������ڴ洢Ӧ�ó���Ĵ��ھ������ʼ��Ϊnullptr
		bool m_Running = false;								// ����һ���������͵�˽�г�Ա���������ڴ洢Ӧ�ó��������״̬����ʼ��Ϊfalse

		float m_TimeStep = 0.0f;							// ����һ���������͵�˽�г�Ա���������ڴ洢Ӧ�ó����ʱ�䲽������ʼ��Ϊ0.0f
		float m_FrameTime = 0.0f;							// ����һ���������͵�˽�г�Ա���������ڴ洢Ӧ�ó����֡ʱ�䣬��ʼ��Ϊ0.0f
		float m_LastFrameTime = 0.0f;						// ����һ���������͵�˽�г�Ա���������ڴ洢Ӧ�ó������һ֡ʱ�䣬��ʼ��Ϊ0.0f

		std::vector<std::shared_ptr<Layer>> m_LayerStack;	// ����һ��std::vector<std::shared_ptr<Layer>>���͵�˽�г�Ա���������ڴ洢Ӧ�ó����ͼ��ջ
		std::function<void()> m_MenubarCallback;			// ����һ��std::function<void()>���͵�˽�г�Ա���������ڴ洢Ӧ�ó���Ĳ˵����Ļص�����
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);	// ����һ��ȫ�ֺ����������ɿͻ���ʵ�֣�����һ��Application���󣬽��������������ֱ��������в����ĸ�����ֵ������һ��Application���͵�ָ��
}