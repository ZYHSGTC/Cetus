#include "VulkanDebug.h"
#include <iostream>

namespace Cetus
{
	namespace debug
	{
		// ������������ָ�����ͣ��ֱ����ڴ��������ٺͻص�Vulkan�ĵ��Թ��� 
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
		VkDebugUtilsMessengerEXT debugUtilsMessenger;

		// ����һ���ص����������ڴ���Vulkan�ĵ�����Ϣ
		VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,		// ������Ϣ�����س̶ȣ�������VERBOSE, INFO, WARNING��ERROR
			VkDebugUtilsMessageTypeFlagsEXT messageType,				// ������Ϣ�����ͣ�������GENERAL, VALIDATION��PERFORMANCE
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,	// ������Ϣ�ľ������ݣ�������ϢID, ��Ϣ���ƺ���Ϣ�ı�
			void* pUserData)											// �û��Զ�������ݣ��������ڴ���һЩ�������Ϣ
		{
			std::string prefix("");										// ����һ�����ַ��������ڴ�ŵ�����Ϣ��ǰ׺

			if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
				prefix = "VERBOSE: ";
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
				prefix = "INFO: ";
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
				prefix = "WARNING: ";
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
				prefix = "ERROR: ";
			}


			std::stringstream debugMessage;
			debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "][" << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;

			// ���������Ϣ�����س̶���ERROR�����ϣ��ͽ�������Ϣ�������׼������
			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
				std::cerr << debugMessage.str() << "\n";
			} else {			// ���򣬽�������Ϣ�������׼�����
				std::cout << debugMessage.str() << "\n";
			}
			fflush(stdout);		// ˢ�±�׼�������ȷ��������Ϣ����ʾ������ǿ��ˢ�»��������ѻ����������ݷ��͵���Ļ�ϡ�
			// printf �����ĺ�������ֱ�Ӵ�ӡ����Ļ�ϵģ������ȷ���һ���������У�stdout���С�
			// ����յ���һ�����з����ͻ����������������ݴ�ӡ����Ļ�ϣ�����ա�
			// �� fflush �����þ���ֱ�Ӱѻ����������ݴ�ӡ����Ļ�ϣ�����ջ����������صȻ��з���

			// ��һ��ԭ���ǣ���׼�����stdout���л���ģ�Ҳ����˵��ֻ�е��������һ�����з����߻��������˵�ʱ�򣬲Ż�ѻ����������������������Ļ�ϡ�
			//  ���ǣ�����Ļ��з���ָC�����е�ת���ַ�\n��������C++�е�std::endl��std::endl�������һ�����з�֮�⣬�������fflush(stdout)��ˢ���������
			//  ���ԣ��������std::cout << debugMessage.str() << std::endl; ���������ô�Ͳ���Ҫ����fflush(stdout)�ˣ���Ϊstd::endl�Ѿ������������������
			//  ���ǣ��������std::cout << debugMessage.str() << ��\n��; ���������ô����Ҫ����fflush(stdout)��ȷ�������ʱ��Ч��������ܻ����������ӳٻ����� 
			// �ڶ���ԭ���ǣ�����������䲻��ֻ��һ�����������������ֱ���std::cerr��std::cout��std::cerr�Ǳ�׼���������������������Ϣ����std::cout�Ǳ�׼����������������ͨ��Ϣ��
			//  �����������Ǽ̳���std::ostream���࣬�������ǵĻ��巽ʽ��ͬ��std::cerr�ǲ�������ģ�Ҳ����˵������������������ݷ��͵���Ļ�ϣ�������ȴ����з����߻��������ˡ� 
			//  ��std::cout�Ǵ�����ģ�Ҳ����˵�������Ȱ�������ݴ���ڻ������У�Ȼ��ȴ����з����߻����������ٷ��͵���Ļ�ϡ�
			//  �����Ϳ��ܵ���һ�����⣬�Ǿ���std::cerr��std::cout�����˳����ܻ᲻һ�£���ʹ�����ڴ������ǰ���һ�����߼�˳��д�ġ�
			//  Ϊ�˱���������⣬����Ҫ��fflush(stdout)��ͬ�������������������ÿ����std::cout���֮�󣬶���fflush(stdout)��ˢ���������
			//  �����Ϳ��Ա�֤std::cerr��std::cout�����˳����߼�һ�¡�
			return VK_FALSE;
		}

		// ����һ���������������õ��Թ���
		void setupDebugging(VkInstance instance)
		{
			// PFN = Pointer to Function of 
			vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
			vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
			
			// ����һ���ṹ�壬�����������Թ��ߵĴ�����Ϣ
			// ����Ҫ����ĵ�����Ϣ�����س̶ȣ�����ֻ����WARNING��ERROR
			// ����Ҫ����ĵ�����Ϣ�����ͣ�����ֻ����GENERAL��VALIDATION
			// ���ûص�����
			VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
			debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			debugUtilsMessengerCI.pfnUserCallback = debugUtilsMessengerCallback;
			VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCI, nullptr, &debugUtilsMessenger);
			assert(result == VK_SUCCESS);
		}

		void freeDebugCallback(VkInstance instance)
		{
			if (debugUtilsMessenger != VK_NULL_HANDLE)
			{
				vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
			}
		}
	}

	namespace debugutils
	{
		// ������������ָ�����ͣ��ֱ����ڿ�ʼ�������Ͳ�����Ա�ǩ
		PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{ nullptr };
		PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{ nullptr };
		PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{ nullptr };

		void setup(VkInstance instance)
		{
			vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
			vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
			vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
		}

		void cmdBeginLabel(VkCommandBuffer cmdbuffer, std::string caption, glm::vec4 color)
		{	// ����һ�����������ڿ�ʼһ�����Ա�ǩ
			// ����������������󣬱�ǩ�ı������ɫ
			if (!vkCmdBeginDebugUtilsLabelEXT) {					// �����ʼ���Ա�ǩ�ĺ���ָ��Ϊ�գ���ֱ�ӷ���
				return;
			}

			// ���Ա�ǩ��һ��������Vulkan��������������ע�͵Ĺ��ܣ����԰��������ߺ͵��Թ��߸��õ����ͷ��������ִ�������
			//	���Ա�ǩ�������ڱ��һ����������Ŀ�ʼ�ͽ��������߲���һ�������ı�ǩ�����Ա�ǩ���԰���һ�����ƺ�һ����ɫ�������ڵ��Թ��������ֲ�ͬ�ı�ǩ��
			// �ṹ��VkDebugUtilsLabelEXT�������������Ա�ǩ����Ϣ�Ľṹ�壬���������ֶΣ�
			// 	sType��һ��VkStructureTypeֵ�����ڱ�ʶ����ṹ������ͣ�������VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT��
			// 	pNext��һ��ָ�룬����ָ����չ����ṹ��������ṹ�壬����ΪNULL��
			// 	pLabelName��һ��ָ�룬ָ��һ���Կ��ַ���β��UTF - 8�ַ�������ʾ��ǩ�����ơ�
			// 	color��һ�����������飬��ʾ��ǩ����ɫ������RGBAֵ����Χ��0.0��1.0���������Ԫ�ض���0.0����ô��ɫ�ᱻ���ԡ�34
			VkDebugUtilsLabelEXT labelInfo{};						// ����һ���ṹ�壬�����������Ա�ǩ����Ϣ
			labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName = caption.c_str();					// ���ñ�ǩ�ı���
			memcpy(labelInfo.color, &color[0], sizeof(float) * 4);	// ���ñ�ǩ����ɫ����memcpy������glm::vec4���͵���ɫ���Ƶ�float������
			vkCmdBeginDebugUtilsLabelEXT(cmdbuffer, &labelInfo);	// ���ÿ�ʼ���Ա�ǩ�ĺ��������������������ͱ�ǩ��Ϣ�ṹ��
		}

		void cmdEndLabel(VkCommandBuffer cmdbuffer)					// �����������������
		{	// ����һ�����������ڽ���һ�����Ա�ǩ
			if (!vkCmdEndDebugUtilsLabelEXT) {						// ����������Ա�ǩ�ĺ���ָ��Ϊ�գ���ֱ�ӷ���
				return;
			}
			vkCmdEndDebugUtilsLabelEXT(cmdbuffer);					// ���ý������Ա�ǩ�ĺ��������������������
		}
	};

}
