#pragma once

#include <algorithm>
#include <iterator>
#include <vector>
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"
#include "VulkanTools.h"

namespace Cetus
{
	struct FramebufferAttachment
	{	// ����һ���ṹ�壬���ڱ�ʾ֡����������
		VkFormat format;							// ����һ�����������ڴ�Ÿ�����ͼ���ʽ 
		VkImage image;								// ����һ�����������ڴ�Ÿ�����ͼ����� 
		VkDeviceMemory memory;						// ����һ�����������ڴ�Ÿ�����ͼ���ڴ���� 
		VkImageView view;							// ����һ�����������ڴ�Ÿ�����ͼ����ͼ���� 
		VkImageSubresourceRange subresourceRange;	// ����һ���ṹ�壬���ڴ�Ÿ�����ͼ������Դ��Χ 
		VkAttachmentDescription description;		// ����һ���ṹ�壬���ڴ�Ÿ�����������Ϣ

		bool hasDepth()								// ����һ�������������жϸ����Ƿ������ȷ���
		{
			std::vector<VkFormat> formats = 
			{
				VK_FORMAT_D16_UNORM,				// 16λ�޷��Ź�һ������ȸ�ʽ
				VK_FORMAT_X8_D24_UNORM_PACK32,		// 24λ�޷��Ź�һ������ȸ�ʽ�����Ϊ32λ���������8λ
				VK_FORMAT_D32_SFLOAT,				// 32λ����������ȸ�ʽ
				VK_FORMAT_D16_UNORM_S8_UINT,		// 16λ�޷��Ź�һ������ȸ�ʽ������8λ�޷���������ģ���ʽ
				VK_FORMAT_D24_UNORM_S8_UINT,		// 24λ�޷��Ź�һ������ȸ�ʽ������8λ�޷���������ģ���ʽ
				VK_FORMAT_D32_SFLOAT_S8_UINT,		// 32λ����������ȸ�ʽ������8λ�޷���������ģ���ʽ
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		bool hasStencil()
		{	// ����һ�������������жϸ����Ƿ����ģ�巽��
			std::vector<VkFormat> formats = 
			{
				VK_FORMAT_S8_UINT,					// 8λ�޷���������ģ���ʽ
				VK_FORMAT_D16_UNORM_S8_UINT,		// 16λ�޷��Ź�һ������ȸ�ʽ������8λ�޷���������ģ���ʽ
				VK_FORMAT_D24_UNORM_S8_UINT,		// 24λ�޷��Ź�һ������ȸ�ʽ������8λ�޷���������ģ���ʽ
				VK_FORMAT_D32_SFLOAT_S8_UINT,		// 32λ����������ȸ�ʽ������8λ�޷���������ģ���ʽ
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		bool isDepthStencil()
		{	// ����һ�������������жϸ����Ƿ������Ȼ�ģ�巽��
			return(hasDepth() || hasStencil());
		}

	};

	// ����һ���ṹ�壬�������������Ĵ�����Ϣ 
	struct AttachmentCreateInfo 
	{ 
		uint32_t width, height;				// �����������������ڴ�Ÿ����Ŀ�Ⱥ͸߶� 
		uint32_t layerCount;				// ����һ�����������ڴ�Ÿ����Ĳ��� 
		VkFormat format;					// ����һ�����������ڴ�Ÿ����ĸ�ʽ 
		VkImageUsageFlags usage;			// ����һ�����������ڴ�Ÿ�������;��־ 
		VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT; // ����һ�����������ڴ�Ÿ����Ĳ���������ʼֵΪVK_SAMPLE_COUNT_1_BIT����ʾÿ������ֻ��һ�������� 
	};

	struct Framebuffer
	{
	private:
		Cetus::VulkanDevice *vulkanDevice;
	public:
		uint32_t width, height;
		VkFramebuffer framebuffer;
		VkRenderPass renderPass;
		VkSampler sampler;
		std::vector<Cetus::FramebufferAttachment> attachments;

		Framebuffer(Cetus::VulkanDevice *vulkanDevice)
		{
			assert(vulkanDevice);
			this->vulkanDevice = vulkanDevice;
		}

		~Framebuffer()
		{
			assert(vulkanDevice);
			for (auto attachment : attachments)
			{
				vkDestroyImage(vulkanDevice->logicalDevice, attachment.image, nullptr);
				vkDestroyImageView(vulkanDevice->logicalDevice, attachment.view, nullptr);
				vkFreeMemory(vulkanDevice->logicalDevice, attachment.memory, nullptr);
			}
			vkDestroySampler(vulkanDevice->logicalDevice, sampler, nullptr);
			vkDestroyRenderPass(vulkanDevice->logicalDevice, renderPass, nullptr);
			vkDestroyFramebuffer(vulkanDevice->logicalDevice, framebuffer, nullptr);
		}

		uint32_t addAttachment(Cetus::AttachmentCreateInfo createinfo)
		{	// ����һ���������������֡����������
			Cetus::FramebufferAttachment attachment;

			// �趨������ʽ
			attachment.format = createinfo.format;

			// �趨����ͼ��
			VkImageCreateInfo image = Cetus::initializers::imageCreateInfo();
			image.imageType = VK_IMAGE_TYPE_2D;			// ����ͼ������ͣ��̶�ΪVK_IMAGE_TYPE_2D����ʾ��άͼ��
			image.format = createinfo.format;			// ����ͼ��ĸ�ʽ�����Ը���������Ϣ�еĸ�ʽ
			image.extent.width = createinfo.width;		// ����ͼ��Ŀ�ȣ����Ը���������Ϣ�еĿ��
			image.extent.height = createinfo.height;	// ����ͼ��ĸ߶ȣ����Ը���������Ϣ�еĸ߶�
			image.extent.depth = 1;						// ����ͼ�����ȣ��̶�Ϊ1����ʾֻ��һ��
			image.mipLevels = 1;						// ����ͼ���mip�ȼ����̶�Ϊ1����ʾֻ��һ��mip�ȼ�
			image.arrayLayers = createinfo.layerCount;	// ����ͼ���������������Ը���������Ϣ�еĲ���
			image.samples = createinfo.imageSampleCount;// ����ͼ��Ĳ����������Ը���������Ϣ�еĲ�����
			image.tiling = VK_IMAGE_TILING_OPTIMAL;		// ����ͼ���ƽ�̷�ʽ���̶�ΪVK_IMAGE_TILING_OPTIMAL����ʾʹ����ѵ�ƽ�̷�ʽ
			image.usage = createinfo.usage;				// ����ͼ�����;�����Ը���������Ϣ�е���;
			VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &image, nullptr, &attachment.image));

			// �趨����ͼ���ڴ�
			VkMemoryAllocateInfo memAlloc = Cetus::initializers::memoryAllocateInfo();
			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, attachment.image, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAlloc, nullptr, &attachment.memory));
			VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, attachment.image, attachment.memory, 0));


			// �趨��������Դ��Χ
			VkImageAspectFlags aspectMask = VK_FLAGS_NONE;
			if (createinfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			{
				aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}
			if (createinfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				if (attachment.hasDepth())
				{
					aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				}
				if (attachment.hasStencil())
				{
					aspectMask = aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}
			assert(aspectMask > 0);
			attachment.subresourceRange = {};								// ������������Դ��Χ�ṹ���ʼ��Ϊ0
			attachment.subresourceRange.aspectMask = aspectMask;			// ������������Դ��Χ�ṹ���еķ�����������Ϊ֮ǰ����ķ�������
			attachment.subresourceRange.levelCount = 1;						// ������������Դ��Χ�ṹ���е�mip�ȼ���������Ϊ1����ʾֻ��һ��mip�ȼ�
			attachment.subresourceRange.layerCount = createinfo.layerCount; // ������������Դ��Χ�ṹ���е��������������Ϊ����������Ϣ�еĲ���

			// ����ͼ����ͼ
			// һ��ͼ������ж����ͬ���͵�ͼ����ͼ��ͼ����ͼ��ָͼ���һ���Ӽ���������VkImageViewCreateInfo�ṹ����������
			//	VkImageViewCreateInfo�������³�Ա��
			// 	viewType��һ��VkImageViewTypeö��ֵ������ָ��ͼ������Ӧ����α����͡�
			//		viewType�����������ͼ����Ϊ1D����2D����3D�������������ͼ�ȵȡ�
			// 	format��һ��VkFormatö��ֵ������ָ��ͼ��ĸ�ʽ��
			// 	components��һ��VkComponentMapping�ṹ�壬����ָ����ɫͨ���Ľ�����ʽ��
			//		���磬����԰����е�ͨ��ӳ�䵽��ɫͨ�����õ�һ����ɫ������Ҳ���԰�0��1�ĳ���ֵӳ�䵽һ��ͨ����
			// 	subresourceRange��һ��VkImageSubresourceRange�ṹ�壬��������ͼ�����;��Ӧ�÷��ʵ�ͼ�����һ���֡�
			//		���磬�����Ӧ�ñ���Ϊһ��û���κ�mipmap�㼶��2D�����������
			VkImageViewCreateInfo imageView = Cetus::initializers::imageViewCreateInfo();
			imageView.viewType = (createinfo.layerCount == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;	// ����ͼ����ͼ�����ͣ����ݸ���������Ϣ�еĲ��������Ϊ1��������ΪVK_IMAGE_VIEW_TYPE_2D����ʾ��άͼ����ͼ����������ΪVK_IMAGE_VIEW_TYPE_2D_ARRAY����ʾ��ά����ͼ����ͼ
			imageView.format = createinfo.format;																		// ����ͼ����ͼ�ĸ�ʽ�����Ը���������Ϣ�еĸ�ʽ
			imageView.subresourceRange = attachment.subresourceRange;													// ����ͼ����ͼ������Դ��Χ�����Ը���������Դ��Χ�ṹ��
			imageView.subresourceRange.aspectMask = (attachment.hasDepth()) ? VK_IMAGE_ASPECT_DEPTH_BIT : aspectMask;	// ����ͼ����ͼ������Դ��Χ�еķ������룬���ݸ����Ƿ������ȷ��棬����ǣ�������ΪVK_IMAGE_ASPECT_DEPTH_BIT����ʾֻ������ȷ��棬��������Ϊ֮ǰ����ķ�������
			// �������Դ�����ע��todo: workaround for depth + stencil attachments��˼������һ����ʱ�Ľ�����������ڴ�����Ⱥ�ģ�帽�������⡣
			//	����������ǣ��������ͬʱ������Ⱥ�ģ�巽�棬��ô�ڴ���ͼ����ͼʱ������ֻѡ������һ�����棬������ѡ�����еķ��棬����ᵼ����֤��Ĵ����������Vulkan�淶��һ��ȱ�ݻ��߲�һ�¡�
			// Ϊ�˱���������⣬��δ���������aspectMaskʱ��ʹ����һ���������ʽ���������������ȷ��棬������ΪVK_IMAGE_ASPECT_DEPTH_BIT����ʾֻѡ����ȷ��棬
			//  ��������Ϊ֮ǰ����ķ������룬��ʾѡ����ɫ�������ģ�巽�档��������Ŀ����Ϊ����ͼ����ͼ�ķ�������͸�����������Ϣ�ṹ���еķ�������һ�£��Ӷ�������֤��Ĵ���
			imageView.image = attachment.image;																			// ����ͼ����ͼ��ͼ��������Ը�����ͼ�����
			VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &imageView, nullptr, &attachment.view));

			// ������������
			attachment.description = {};
			attachment.description.samples = createinfo.imageSampleCount;				// ��������������Ϣ�ṹ���еĲ���������Ϊ����������Ϣ�еĲ�����
			attachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// ��������������Ϣ�ṹ���еļ��ز�������ΪVK_ATTACHMENT_LOAD_OP_CLEAR����ʾ����Ⱦ��ʼʱ���������������
			// ��������������Ϣ�ṹ���еĴ洢��������Ϊ���ݸ���������Ϣ�е���;��־���������VK_IMAGE_USAGE_SAMPLED_BIT����ʾ����������Ϊ����ͼ��
			// ������ΪVK_ATTACHMENT_STORE_OP_STORE����ʾ����Ⱦ����ʱ���������������ݣ�
			// ��������ΪVK_ATTACHMENT_STORE_OP_DONT_CARE����ʾ����Ⱦ����ʱ�������ĸ���������
			attachment.description.storeOp = (createinfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE; 
			attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// ��������������Ϣ�ṹ���е�ģ����ز�������ΪVK_ATTACHMENT_LOAD_OP_DONT_CARE����ʾ����Ⱦ��ʼʱ�������ĸ�����ģ������
			attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// ��������������Ϣ�ṹ���е�ģ��洢��������ΪVK_ATTACHMENT_STORE_OP_DONT_CARE����ʾ����Ⱦ����ʱ�������ĸ�����ģ������
			attachment.description.format = createinfo.format;							// ��������������Ϣ�ṹ���еĸ�ʽ����Ϊ����������Ϣ�еĸ�ʽ
			attachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// ��������������Ϣ�ṹ���еĳ�ʼ��������ΪVK_IMAGE_LAYOUT_UNDEFINED����ʾ����Ⱦ��ʼʱ�������ĸ�����ͼ�񲼾�
			if (attachment.hasDepth() || attachment.hasStencil())						// �������������Ȼ�ģ�巽�棬ʹ�ø��������isDepthStencil���������ж�
			{	// ��������������Ϣ�ṹ���е����ղ�������ΪVK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL����ʾ����Ⱦ����ʱ����������ͼ�񲼾�ת��Ϊ���ģ��ֻ������Ѳ���
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; 
			}
			else
			{	// ��������������Ϣ�ṹ���е����ղ�������ΪVK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL����ʾ����Ⱦ����ʱ����������ͼ�񲼾�ת��Ϊ��ɫ��ֻ������Ѳ���
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; 
			}
			attachments.push_back(attachment);

			return static_cast<uint32_t>(attachments.size() - 1);
		}

		VkResult createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode adressMode)
		{	// ����һ�����������ڴ�������������
			// ����������ö��ֵ���ֱ��ʾ�Ŵ����������С��������Ѱַģʽ 
			VkSamplerCreateInfo samplerInfo = Cetus::initializers::samplerCreateInfo();
			samplerInfo.magFilter = magFilter;		// ��������������Ϣ�ṹ���еķŴ����������Ϊ����magFilter����ʾ�����������ش�������ʱ��ʹ�õĹ��˷�ʽ 
			samplerInfo.minFilter = minFilter;		// ��������������Ϣ�ṹ���е���С����������Ϊ����minFilter����ʾ������������С������ʱ��ʹ�õĹ��˷�ʽ 
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // ��������������Ϣ�ṹ���е�mip��ͼģʽ����ΪVK_SAMPLER_MIPMAP_MODE_LINEAR����ʾ��������mip�ȼ���������ʱ��ʹ�����Բ�ֵ�ķ�ʽ 
			samplerInfo.addressModeU = adressMode;	// ��������������Ϣ�ṹ���е�U����Ѱַģʽ����Ϊ����adressMode����ʾ���������������곬��[0,1]��Χʱ��U����Ĵ���ʽ 
			samplerInfo.addressModeV = adressMode;	// ��������������Ϣ�ṹ���е�V����Ѱַģʽ����Ϊ����adressMode����ʾ���������������곬��[0,1]��Χʱ��V����Ĵ���ʽ 
			samplerInfo.addressModeW = adressMode;	// ��������������Ϣ�ṹ���е�W����Ѱַģʽ����Ϊ����adressMode����ʾ���������������곬��[0,1]��Χʱ��W����Ĵ���ʽ 
			samplerInfo.mipLodBias = 0.0f;			// ��������������Ϣ�ṹ���е�mip�ȼ�ƫ��������Ϊ0.0f����ʾ����mip�ȼ�����ƫ�� 
			samplerInfo.maxAnisotropy = 1.0f;		// ��������������Ϣ�ṹ���е���������������Ϊ1.0f����ʾ��ʹ�ø������Թ��� 
			samplerInfo.minLod = 0.0f;				// ��������������Ϣ�ṹ���е���Сmip�ȼ�����Ϊ0.0f����ʾ������mip�ȼ���ʼ���� 
			samplerInfo.maxLod = 1.0f;				// ��������������Ϣ�ṹ���е����mip�ȼ�����Ϊ1.0f����ʾ����С��mip�ȼ��������� 
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; // ��������������Ϣ�ṹ���еı߽���ɫ����ΪVK_BORDER_COLOR_FLOAT_OPAQUE_WHITE����ʾ���������������곬��[0,1]��Χʱ��ʹ�ò�͸���İ�ɫ��Ϊ�߽���ɫ 
			return vkCreateSampler(vulkanDevice->logicalDevice, &samplerInfo, nullptr, &sampler);
		}

		VkResult createRenderPass()
		{
			std::vector<VkAttachmentDescription> attachmentDescriptions;
			for (auto& attachment : attachments)
			{
				attachmentDescriptions.push_back(attachment.description);
			};


			// ������ͨ������������ͨ���Ĵ��������������Ⱦ����������������
			std::vector<VkAttachmentReference> colorReferences;	// ����һ��std::vector�������洢VkAttachmentReference�ṹ�壬��ʾ��ɫ������������Ϣ���總���������Ͳ���
			VkAttachmentReference depthReference = {};			// ����һ��VkAttachmentReference�ṹ�壬��ʾ���ģ�帽����������Ϣ����Ĭ��ֵ��ʼ��
			bool hasDepth = false;								// ����һ��������������ʾ�Ƿ������ģ�帽������ʼֵΪfalse
			bool hasColor = false;								// ����һ��������������ʾ�Ƿ�����ɫ��������ʼֵΪfalse

			uint32_t attachmentIndex = 0;
			for (auto& attachment : attachments)
			{
				if (attachment.isDepthStencil())
				{
					assert(!hasDepth);
					depthReference.attachment = attachmentIndex;
					depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					hasDepth = true;
				}
				else
				{
					colorReferences.push_back({ attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
					hasColor = true;
				}
				attachmentIndex++;
			};

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			if (hasColor)
			{
				subpass.pColorAttachments = colorReferences.data();
				subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			}
			if (hasDepth)
			{
				subpass.pDepthStencilAttachment = &depthReference;
			}


			// ������ͨ������
			std::array<VkSubpassDependency, 2> dependencies;	// ����һ��std::array�������洢VkSubpassDependency�ṹ�壬��ʾ��ͨ��֮���������ϵ�������Ĵ�СΪ2����ʾ������������ϵ

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;	// ��ʾ������Դsubpass����Ⱦͨ��֮����������ǰ��Ĵ�����������
			dependencies[0].dstSubpass = 0;						// ��ʾ������Ŀ��subpass����Ⱦͨ���еĵ�һ��subpass����subpass 0
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;			// ��ʾ������Դ�׶��ǹ��ߵ����һ���׶Σ����ײ��׶Σ���ʾ������׶�֮ǰ����������������
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// ��ʾ������Ŀ��׶��ǹ��ߵ���ɫ��������׶Σ�����Ƭ����ɫ��֮�󣬽���ɫֵд����ɫ�����Ľ׶�
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;						// ��ʾ������Դ�����������ڴ��ȡ������Դ�׶�֮ǰ���������������ɶ��ڴ�Ķ�ȡ����
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // ��ʾ������Ŀ�������������ɫ������ȡ��д�룬����Ŀ��׶�֮��������������ȴ�����ɫ�����Ķ�ȡ��д��������
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;					// ��ʾ�����ı�־�ǰ�����ģ���ֻ������ͬ�����Դ��Ŀ������֮�����Ҫͬ��������������֡������
			// �ڿ�ʼ��Ⱦͨ��֮ǰ��ȷ�����е��ⲿ����Ѿ���ɣ��������ݾ�����һ�µ����� ��
			// 0֮ǰ��ȷ����ɫ�����������Ѿ�׼���ã������ȡ��Ч�����ݻ򸲸���Ч������ ��
			// 0֮��ȷ����ɫ�����������Ѿ�д�룬��������������subpass��ȡ��������ݻ��޸�δ��ɵ����� ��
 
			dependencies[1].srcSubpass = 0;						// ���ڶ���������ϵ��srcSubpass��Ա����ʾԴ��ͨ������������ֵΪ0����ʾ��������ϵ��Դ�ǵ�һ����ͨ��
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;	// ���ڶ���������ϵ��dstSubpass��Ա����ʾĿ����ͨ������������ֵΪVK_SUBPASS_EXTERNAL����ʾ��������ϵ��Ŀ�����ⲿ��
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// ���ڶ���������ϵ��srcStageMask��Ա����ʾԴ�׶����룩��ֵΪVK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT����ʾ��������ϵ��Դ����ɫ��������׶�
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;			// ���ڶ���������ϵ��dstStageMask��Ա����ʾĿ��׶����룩��ֵΪVK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT����ʾ��������ϵ��Ŀ���ǹ��ߵ����һ���׶�
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // ���ڶ���������ϵ��srcAccessMask��Ա����ʾԴ�������룩��ֵΪVK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT����ʾ��������ϵ��Դ��Ҫ��д��ɫ����
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;						// ���ڶ���������ϵ��dstAccessMask��Ա����ʾĿ��������룩��ֵΪVK_ACCESS_MEMORY_READ_BIT����ʾ��������ϵ��Ŀ����Ҫ��ȡ�ڴ�
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;					// ���ڶ���������ϵ��dependencyFlags��Ա����ʾ������־����ֵΪVK_DEPENDENCY_BY_REGION_BIT����ʾ��������ϵ�ǰ�����ģ���ֻӰ����ͬ���������


			// ������Ⱦͨ��
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = attachmentDescriptions.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();
			VK_CHECK_RESULT(vkCreateRenderPass(vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &renderPass));


			// ����֡����
			std::vector<VkImageView> attachmentViews;
			for (auto attachment : attachments)
			{
				attachmentViews.push_back(attachment.view);
			}

			uint32_t maxLayers = 0;
			for (auto attachment : attachments)
			{
				if (attachment.subresourceRange.layerCount > maxLayers)
				{
					maxLayers = attachment.subresourceRange.layerCount;
				}
			}

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.pAttachments = attachmentViews.data();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = maxLayers;
			VK_CHECK_RESULT(vkCreateFramebuffer(vulkanDevice->logicalDevice, &framebufferInfo, nullptr, &framebuffer));

			return VK_SUCCESS;
		}
	};
}