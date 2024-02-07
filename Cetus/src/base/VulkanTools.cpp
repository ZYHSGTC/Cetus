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
				// ���r��NOT_READY����ô��չ���Ľ���ǣ�
				// case VK_NOT_READY: return ��NOT_READY��;
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
			std::vector<VkFormat> formatList = {	// ������ȸ�ʽ���б�
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM
			};

			
			for (auto& format : formatList)			// ������ȸ�ʽ�б�
			{
				VkFormatProperties formatProps;		// ���ڴ洢��ʽ���Եı���
				// ��ѯ�����豸�Ը�ʽ������
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

				// ����ʽ�Ƿ�֧�����ģ�帽��
				if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				{
					*depthFormat = format;			// ���֧�֣�����ȸ�ʽ���浽depthFormatָ��ָ���λ�ã�������true
					return true;
				}
			}

			// ���û���ҵ�֧�ֵ���ȸ�ʽ������false
			return false;
		}

		VkBool32 getSupportedDepthStencilFormat(VkPhysicalDevice physicalDevice, VkFormat* depthStencilFormat)
		{
			// �������ģ���ʽ���б�
			std::vector<VkFormat> formatList = {
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
			};

			// �������ģ���ʽ�б�
			for (auto& format : formatList)
			{
				// ���ڴ洢��ʽ���Եı���
				VkFormatProperties formatProps;
				// ��ѯ�����豸�Ը�ʽ������
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

				// ����ʽ�Ƿ�֧�����ģ�帽��
				if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				{
					// ���֧�֣������ģ���ʽ���浽depthStencilFormatָ��ָ���λ�ã�������true
					*depthStencilFormat = format;
					return true;
				}
			}

			// ���û���ҵ�֧�ֵ����ģ���ʽ������false
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
		{	// ����һ����������������ͼ��Ĳ���
			// �����ֱ�Ϊ�����������ͼ�񣬾ɵĲ��֣��µĲ��֣�����Դ��Χ��Դ�׶����룬Ŀ��׶�����
			// ����һ��VkImageMemoryBarrier���͵ı��������ڴ洢ͼ���ڴ����ϵ���Ϣ��������
			VkImageMemoryBarrier imageMemoryBarrier = Cetus::initializers::imageMemoryBarrier();
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
 
			// ���ݲ���oldImageLayout�Ĳ�ͬ���������imageMemoryBarrier������srcAccessMask�ֶΣ���ʾ��ͼ�񲼾ֱ任֮ǰ����Щ���͵ķ�����Ҫ������
			switch (oldImageLayout)
			{
				// ����ɵĲ���Ϊδ���壬��ʾͼ������ݲ���Ҫ������Ҫ�����κη���
			case VK_IMAGE_LAYOUT_UNDEFINED:			
				imageMemoryBarrier.srcAccessMask = 0;
				break; 
				// ����ɵĲ���ΪԤ��ʼ������ʾͼ���������������CPU��д�룬��Ҫ��������д��ķ���
			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;
				// ����ɵĲ���Ϊ��ɫ�������ţ���ʾͼ�����������ɫ����д�룬��Ҫ������ɫ����д��ķ���
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
				// ����ɵĲ���Ϊ���ģ�帽�����ţ���ʾͼ������������ģ�帽��д�룬��Ҫ�������ģ�帽��д��ķ���
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
				// ����ɵĲ���Ϊ����Դ���ţ���ʾͼ��������ɴ��������ȡ����Ҫ���������ȡ�ķ���
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
				// ����ɵĲ���Ϊ����Ŀ�����ţ���ʾͼ��������ɴ������д�룬��Ҫ��������д��ķ���
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
				// ����ɵĲ���Ϊ��ɫ��ֻ�����ţ���ʾͼ�����������ɫ����ȡ����Ҫ������ɫ����ȡ�ķ���
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				break;
			}

			// ���ݲ���newImageLayout�Ĳ�ͬ���������imageMemoryBarrier������dstAccessMask�ֶΣ���ʾ��ͼ�񲼾ֱ任֮����Щ���͵ķ��ʿ��Ա�����
			switch (newImageLayout)
			{
				// ����µĲ���Ϊ����Ŀ�����ţ���ʾͼ������ݽ����������д�룬��Ҫ���ô���д��ķ���
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
				// ����µĲ���Ϊ����Դ���ţ���ʾͼ������ݽ������������ȡ����Ҫ���ô����ȡ�ķ���
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
				// ����µĲ���Ϊ��ɫ�������ţ���ʾͼ������ݽ�����ɫ����д�룬��Ҫ������ɫ����д��ķ���
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
				// ����µĲ���Ϊ���ģ�帽�����ţ���ʾͼ������ݽ������ģ�帽��д�룬��Ҫ�������ģ�帽��д��ķ���
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
				// ����µĲ���Ϊ��ɫ��ֻ�����ţ���ʾͼ������ݽ�����ɫ����ȡ����Ҫ������ɫ����ȡ�ķ���
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
				// ���ͼ�񲼾ֱ任֮ǰ��û�������κη��ʣ�����Ҫ�������������д��ʹ���д��ķ��ʣ��Ա�֤����һ����
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				break;
			}

			vkCmdPipelineBarrier(	// ��¼һ�����������������ʵ��ͼ�񲼾ֵı任
				cmdbuffer,			// �������
				srcStageMask,		// Դ�׶�����
				dstStageMask,		// Ŀ��׶�����
				0,					// ������־������Ϊ0��ʾû�����������
				0, nullptr,			// �ڴ����ϵ�������ָ�룬����Ϊ0��nullptr��ʾû���ڴ�����
				0, nullptr,			// ���������ϵ�������ָ�룬����Ϊ0��nullptr��ʾû�л���������
				1, &imageMemoryBarrier); // ͼ�����ϵ�������ָ�룬����Ϊ1��&imageMemoryBarrier��ʾ��һ��ͼ������
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
		{	// ����һ�������������˳�������ʾ������Ϣ
			if (!errorModeSilent) { 
				// �������ģʽ���Ǿ�Ĭ�ģ�Ҳ����˵����Ҫ��ʾ������Ϣ 
				//����MessageBoxA�����������ָ�룬������Ϣ��C����ַ�������ָ�����Ϣ�����ʽ����ʾһ�����д���ͼ�����Ϣ��
				MessageBox(NULL, message.c_str(), NULL, MB_OK | MB_ICONERROR);
				// MessageBoxA������һ��Windows API��������������Ļ����ʾһ����Ϣ�����û�����һЩ��Ϣ���߽���һЩѡ��
				// 	MB_OK | MB_ICONERROR��һ������������ָ����Ϣ�����ʽ���������������İ�λ������Ľ����
				// 	MB_OK��ʾ��Ϣ��ֻ��һ��ȷ����ť���û��������Ϣ��ͻ�رա�
				// 	MB_ICONERROR��ʾ��Ϣ����ʾһ������ͼ�꣬������ʾ�û����������صĴ���
				// 	MessageBoxA�������÷��ǣ�
				// 	int MessageBoxA(HWND hWnd, // ��Ϣ��ĸ����ھ�������ΪNULL����ʾû�и����� 
				//					LPCSTR lpText, // ��Ϣ����ı����ݣ��������Կ��ַ���β��C����ַ��� 
				//					LPCSTR lpCaption, // ��Ϣ��ı��⣬�������Կ��ַ���β��C����ַ��� 
				//					UINT uType // ��Ϣ�����ʽ�������Ƕ�������İ�λ������Ľ�� );
				// 	��������һ����������ʾ�û�����İ�ť�ı�ʶ����
				// 	���磬���������ʾһ�����д���ͼ���ȷ����ť����Ϣ��������"Could not create surface!����������"Error�������������д��
				//	MessageBoxA(NULL, ��Could not create surface!��, ��Error��, MB_OK | MB_ICONERROR);
			}
			std::cerr << message << "\n";	// ��������Ϣ�������׼�������������� 
			exit(exitCode);					// ����exit�����������˳��룬���������ִ��
		}

		void exitFatal(const std::string& message, VkResult resultCode)
		{	// ����һ�����غ����������˳�������ʾ������Ϣ
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
