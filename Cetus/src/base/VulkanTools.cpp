#include "VulkanTools.h"

const std::string getAssetPath()
{
	return "../Cetus/assets/";
}

const std::string getShaderBasePath()
{
	return "./../shaders/";
}

namespace Cetus
{
	namespace tools
	{
		bool errorModeSilent = false;

		std::string errorString(VkResult errorCode)
		{
			switch (errorCode)
			{
#define STR(r) case VK_ ##r: return #r
				// 如果r是NOT_READY，那么宏展开的结果是：
				// case VK_NOT_READY: return “NOT_READY”;
				STR(NOT_READY);
				STR(TIMEOUT);
				STR(EVENT_SET);
				STR(EVENT_RESET);
				STR(INCOMPLETE);
				STR(ERROR_OUT_OF_HOST_MEMORY);
				STR(ERROR_OUT_OF_DEVICE_MEMORY);
				STR(ERROR_INITIALIZATION_FAILED);
				STR(ERROR_DEVICE_LOST);
				STR(ERROR_MEMORY_MAP_FAILED);
				STR(ERROR_LAYER_NOT_PRESENT);
				STR(ERROR_EXTENSION_NOT_PRESENT);
				STR(ERROR_FEATURE_NOT_PRESENT);
				STR(ERROR_INCOMPATIBLE_DRIVER);
				STR(ERROR_TOO_MANY_OBJECTS);
				STR(ERROR_FORMAT_NOT_SUPPORTED);
				STR(ERROR_SURFACE_LOST_KHR);
				STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
				STR(SUBOPTIMAL_KHR);
				STR(ERROR_OUT_OF_DATE_KHR);
				STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
				STR(ERROR_VALIDATION_FAILED_EXT);
				STR(ERROR_INVALID_SHADER_NV);
				STR(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT);
#undef STR
			default:
				return "UNKNOWN_ERROR";
			}
		}

		std::string physicalDeviceTypeString(VkPhysicalDeviceType type)
		{
			switch (type)
			{
#define STR(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r
				STR(OTHER);
				STR(INTEGRATED_GPU);
				STR(DISCRETE_GPU);
				STR(VIRTUAL_GPU);
				STR(CPU);
#undef STR
			default: return "UNKNOWN_DEVICE_TYPE";
			}
		}

		VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat)
		{
			std::vector<VkFormat> formatList = {	// 定义深度格式的列表
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM
			};

			
			for (auto& format : formatList)			// 遍历深度格式列表
			{
				VkFormatProperties formatProps;		// 用于存储格式属性的变量
				// 查询物理设备对格式的属性
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

				// 检查格式是否支持深度模板附件
				if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				{
					*depthFormat = format;			// 如果支持，将深度格式保存到depthFormat指针指向的位置，并返回true
					return true;
				}
			}

			// 如果没有找到支持的深度格式，返回false
			return false;
		}

		VkBool32 getSupportedDepthStencilFormat(VkPhysicalDevice physicalDevice, VkFormat* depthStencilFormat)
		{
			// 定义深度模板格式的列表
			std::vector<VkFormat> formatList = {
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
			};

			// 遍历深度模板格式列表
			for (auto& format : formatList)
			{
				// 用于存储格式属性的变量
				VkFormatProperties formatProps;
				// 查询物理设备对格式的属性
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

				// 检查格式是否支持深度模板附件
				if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				{
					// 如果支持，将深度模板格式保存到depthStencilFormat指针指向的位置，并返回true
					*depthStencilFormat = format;
					return true;
				}
			}

			// 如果没有找到支持的深度模板格式，返回false
			return false;
		}


		VkBool32 formatHasStencil(VkFormat format)
		{
			std::vector<VkFormat> stencilFormats = {
				VK_FORMAT_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			};
			return std::find(stencilFormats.begin(), stencilFormats.end(), format) != std::end(stencilFormats);
		}

		// Returns if a given format support LINEAR filtering
		VkBool32 formatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

			if (tiling == VK_IMAGE_TILING_OPTIMAL)
				return formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

			if (tiling == VK_IMAGE_TILING_LINEAR)
				return formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

			return false;
		}

		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{	// 定义一个函数，用于设置图像的布局
			// 参数分别为：命令缓冲区，图像，旧的布局，新的布局，子资源范围，源阶段掩码，目标阶段掩码
			// 定义一个VkImageMemoryBarrier类型的变量，用于存储图像内存屏障的信息，并调用
			VkImageMemoryBarrier imageMemoryBarrier = Cetus::initializers::imageMemoryBarrier();
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
 
			// 根据参数oldImageLayout的不同情况，设置imageMemoryBarrier变量的srcAccessMask字段，表示在图像布局变换之前，哪些类型的访问需要被阻塞
			switch (oldImageLayout)
			{
				// 如果旧的布局为未定义，表示图像的内容不重要，不需要阻塞任何访问
			case VK_IMAGE_LAYOUT_UNDEFINED:			
				imageMemoryBarrier.srcAccessMask = 0;
				break; 
				// 如果旧的布局为预初始化，表示图像的内容由主机（CPU）写入，需要阻塞主机写入的访问
			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;
				// 如果旧的布局为颜色附件最优，表示图像的内容由颜色附件写入，需要阻塞颜色附件写入的访问
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
				// 如果旧的布局为深度模板附件最优，表示图像的内容由深度模板附件写入，需要阻塞深度模板附件写入的访问
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
				// 如果旧的布局为传输源最优，表示图像的内容由传输操作读取，需要阻塞传输读取的访问
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
				// 如果旧的布局为传输目的最优，表示图像的内容由传输操作写入，需要阻塞传输写入的访问
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
				// 如果旧的布局为着色器只读最优，表示图像的内容由着色器读取，需要阻塞着色器读取的访问
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				break;
			}

			// 根据参数newImageLayout的不同情况，设置imageMemoryBarrier变量的dstAccessMask字段，表示在图像布局变换之后，哪些类型的访问可以被启用
			switch (newImageLayout)
			{
				// 如果新的布局为传输目的最优，表示图像的内容将被传输操作写入，需要启用传输写入的访问
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
				// 如果新的布局为传输源最优，表示图像的内容将被传输操作读取，需要启用传输读取的访问
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
				// 如果新的布局为颜色附件最优，表示图像的内容将被颜色附件写入，需要启用颜色附件写入的访问
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
				// 如果新的布局为深度模板附件最优，表示图像的内容将被深度模板附件写入，需要启用深度模板附件写入的访问
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
				// 如果新的布局为着色器只读最优，表示图像的内容将被着色器读取，需要启用着色器读取的访问
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
				// 如果图像布局变换之前，没有阻塞任何访问，就需要额外地阻塞主机写入和传输写入的访问，以保证数据一致性
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				break;
			}

			vkCmdPipelineBarrier(	// 记录一个管线屏障命令，用于实现图像布局的变换
				cmdbuffer,			// 命令缓冲区
				srcStageMask,		// 源阶段掩码
				dstStageMask,		// 目标阶段掩码
				0,					// 依赖标志，这里为0表示没有特殊的依赖
				0, nullptr,			// 内存屏障的数量和指针，这里为0和nullptr表示没有内存屏障
				0, nullptr,			// 缓冲区屏障的数量和指针，这里为0和nullptr表示没有缓冲区屏障
				1, &imageMemoryBarrier); // 图像屏障的数量和指针，这里为1和&imageMemoryBarrier表示有一个图像屏障
		}

		// Fixed sub resource on first mip level and layer
		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;
			setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
		}

		void insertImageMemoryBarrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange)
		{
			VkImageMemoryBarrier imageMemoryBarrier = Cetus::initializers::imageMemoryBarrier();
			imageMemoryBarrier.srcAccessMask = srcAccessMask;
			imageMemoryBarrier.dstAccessMask = dstAccessMask;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		void exitFatal(const std::string& message, int32_t exitCode)
		{	// 定义一个函数，用于退出程序并显示错误信息
			if (!errorModeSilent) { 
				// 如果错误模式不是静默的，也就是说，需要显示错误信息 
				//调用MessageBoxA函数，传入空指针，错误信息的C风格字符串，空指针和消息框的样式，显示一个带有错误图标的消息框
				MessageBox(NULL, message.c_str(), NULL, MB_OK | MB_ICONERROR);
				// MessageBoxA函数是一个Windows API函数，用于在屏幕上显示一个消息框，让用户看到一些信息或者进行一些选择。
				// 	MB_OK | MB_ICONERROR是一个常量，用于指定消息框的样式，它是两个常量的按位或运算的结果。
				// 	MB_OK表示消息框只有一个确定按钮，用户点击后消息框就会关闭。
				// 	MB_ICONERROR表示消息框显示一个错误图标，用于提示用户发生了严重的错误。
				// 	MessageBoxA函数的用法是：
				// 	int MessageBoxA(HWND hWnd, // 消息框的父窗口句柄，如果为NULL，表示没有父窗口 
				//					LPCSTR lpText, // 消息框的文本内容，必须是以空字符结尾的C风格字符串 
				//					LPCSTR lpCaption, // 消息框的标题，必须是以空字符结尾的C风格字符串 
				//					UINT uType // 消息框的样式，可以是多个常量的按位或运算的结果 );
				// 	函数返回一个整数，表示用户点击的按钮的标识符。
				// 	例如，如果你想显示一个带有错误图标和确定按钮的消息框，内容是"Could not create surface!“，标题是"Error”，你可以这样写：
				//	MessageBoxA(NULL, “Could not create surface!”, “Error”, MB_OK | MB_ICONERROR);
			}
			std::cerr << message << "\n";	// 将错误信息输出到标准错误流，并换行 
			exit(exitCode);					// 调用exit函数，传入退出码，结束程序的执行
		}

		void exitFatal(const std::string& message, VkResult resultCode)
		{	// 定义一个重载函数，用于退出程序并显示错误信息
			exitFatal(message, (int32_t)resultCode);
		}

		VkShaderModule loadShader(const std::string& filename, VkDevice device)
		{
			size_t shaderSize;
			char* shaderCode{ nullptr };

			std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

			if (is.is_open())
			{
				shaderSize = is.tellg();
				is.seekg(0, std::ios::beg);
				shaderCode = new char[shaderSize];
				is.read(shaderCode, shaderSize);
				is.close();
				assert(shaderSize > 0);
			}
			if (shaderCode)
			{
				VkShaderModuleCreateInfo shaderModuleCreateInfo{};
				shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				shaderModuleCreateInfo.codeSize = shaderSize;
				shaderModuleCreateInfo.pCode = (uint32_t*)shaderCode;

				VkShaderModule shaderModule;
				VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule));

				delete[] shaderCode;

				return shaderModule;
			}
			else
			{
				std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
				return VK_NULL_HANDLE;
			}
		}

		bool fileExists(const std::string &filename)
		{
			std::ifstream f(filename.c_str());
			return !f.fail();
		}

		uint32_t alignedSize(uint32_t value, uint32_t alignment)
        {
	        return (value + alignment - 1) & ~(alignment - 1);
		}

		size_t alignedSize(size_t value, size_t alignment)
		{
			return (value + alignment - 1) & ~(alignment - 1);
		}

	}
}
