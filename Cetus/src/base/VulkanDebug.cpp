#include "VulkanDebug.h"
#include <iostream>

namespace Cetus
{
	namespace debug
	{
		// 定义三个函数指针类型，分别用于创建、销毁和回调Vulkan的调试工具 
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
		VkDebugUtilsMessengerEXT debugUtilsMessenger;

		// 定义一个回调函数，用于处理Vulkan的调试信息
		VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,		// 调试信息的严重程度，可以是VERBOSE, INFO, WARNING或ERROR
			VkDebugUtilsMessageTypeFlagsEXT messageType,				// 调试信息的类型，可以是GENERAL, VALIDATION或PERFORMANCE
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,	// 调试信息的具体内容，包括消息ID, 消息名称和消息文本
			void* pUserData)											// 用户自定义的数据，可以用于传递一些额外的信息
		{
			std::string prefix("");										// 定义一个空字符串，用于存放调试信息的前缀

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

			// 如果调试信息的严重程度是ERROR或以上，就将调试信息输出到标准错误流
			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
				std::cerr << debugMessage.str() << "\n";
			} else {			// 否则，将调试信息输出到标准输出流
				std::cout << debugMessage.str() << "\n";
			}
			fflush(stdout);		// 刷新标准输出流，确保调试信息被显示出来，强制刷新缓冲区，把缓冲区的内容发送到屏幕上。
			// printf 这样的函数不是直接打印到屏幕上的，而是先放在一个缓冲区中（stdout）中。
			// 如果收到了一个换行符，就会把这个缓冲区的内容打印到屏幕上，并清空。
			// 而 fflush 的作用就是直接把缓冲区的内容打印到屏幕上，并清空缓冲区。不必等换行符。

			// 第一个原因是，标准输出流stdout是行缓冲的，也就是说，只有当你输出了一个换行符或者缓冲区满了的时候，才会把缓冲区的内容真正输出到屏幕上。
			//  但是，这里的换行符是指C语言中的转义字符\n，而不是C++中的std::endl。std::endl除了输出一个换行符之外，还会调用fflush(stdout)来刷新输出流。
			//  所以，如果你用std::cout << debugMessage.str() << std::endl; 来输出，那么就不需要再用fflush(stdout)了，因为std::endl已经帮你做了这个工作。
			//  但是，如果你用std::cout << debugMessage.str() << “\n”; 来输出，那么就需要再用fflush(stdout)来确保输出及时生效，否则可能会出现输出的延迟或乱序。 
			// 第二个原因是，这里的输出语句不是只有一个，而是有两个，分别是std::cerr和std::cout。std::cerr是标准错误流，用于输出错误信息，而std::cout是标准输出流，用于输出普通信息。
			//  这两个流都是继承自std::ostream的类，但是它们的缓冲方式不同。std::cerr是不带缓冲的，也就是说，它会立即把输出内容发送到屏幕上，而不会等待换行符或者缓冲区满了。 
			//  而std::cout是带缓冲的，也就是说，它会先把输出内容存放在缓冲区中，然后等待换行符或者缓冲区满了再发送到屏幕上。
			//  这样就可能导致一个问题，那就是std::cerr和std::cout的输出顺序可能会不一致，即使它们在代码中是按照一定的逻辑顺序写的。
			//  为了避免这个问题，就需要用fflush(stdout)来同步两个流的输出，即在每次用std::cout输出之后，都用fflush(stdout)来刷新输出流，
			//  这样就可以保证std::cerr和std::cout的输出顺序和逻辑一致。
			return VK_FALSE;
		}

		// 定义一个函数，用于设置调试工具
		void setupDebugging(VkInstance instance)
		{
			// PFN = Pointer to Function of 
			vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
			vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
			
			// 定义一个结构体，用于描述调试工具的创建信息
			// 设置要捕获的调试信息的严重程度，这里只捕获WARNING和ERROR
			// 设置要捕获的调试信息的类型，这里只捕获GENERAL和VALIDATION
			// 设置回调函数
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
		// 定义三个函数指针类型，分别用于开始、结束和插入调试标签
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
		{	// 定义一个函数，用于开始一个调试标签
			// 参数是命令缓冲区对象，标签的标题和颜色
			if (!vkCmdBeginDebugUtilsLabelEXT) {					// 如果开始调试标签的函数指针为空，就直接返回
				return;
			}

			// 调试标签是一种用于在Vulkan的命令缓冲区中添加注释的功能，可以帮助开发者和调试工具更好地理解和分析命令的执行情况。
			//	调试标签可以用于标记一个命令缓冲区的开始和结束，或者插入一个单独的标签。调试标签可以包含一个名称和一个颜色，用于在调试工具中区分不同的标签。
			// 结构体VkDebugUtilsLabelEXT是用于描述调试标签的信息的结构体，它有以下字段：
			// 	sType：一个VkStructureType值，用于标识这个结构体的类型，必须是VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT。
			// 	pNext：一个指针，用于指向扩展这个结构体的其他结构体，或者为NULL。
			// 	pLabelName：一个指针，指向一个以空字符结尾的UTF - 8字符串，表示标签的名称。
			// 	color：一个浮点数数组，表示标签的颜色，包含RGBA值，范围是0.0到1.0。如果所有元素都是0.0，那么颜色会被忽略。34
			VkDebugUtilsLabelEXT labelInfo{};						// 定义一个结构体，用于描述调试标签的信息
			labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName = caption.c_str();					// 设置标签的标题
			memcpy(labelInfo.color, &color[0], sizeof(float) * 4);	// 设置标签的颜色，用memcpy函数将glm::vec4类型的颜色复制到float数组中
			vkCmdBeginDebugUtilsLabelEXT(cmdbuffer, &labelInfo);	// 调用开始调试标签的函数，传入命令缓冲区对象和标签信息结构体
		}

		void cmdEndLabel(VkCommandBuffer cmdbuffer)					// 参数是命令缓冲区对象
		{	// 定义一个函数，用于结束一个调试标签
			if (!vkCmdEndDebugUtilsLabelEXT) {						// 如果结束调试标签的函数指针为空，就直接返回
				return;
			}
			vkCmdEndDebugUtilsLabelEXT(cmdbuffer);					// 调用结束调试标签的函数，传入命令缓冲区对象
		}
	};

}
