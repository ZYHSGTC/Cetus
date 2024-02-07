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
	{	// 定义一个结构体，用于表示帧缓冲区附件
		VkFormat format;							// 定义一个变量，用于存放附件的图像格式 
		VkImage image;								// 定义一个变量，用于存放附件的图像对象 
		VkDeviceMemory memory;						// 定义一个变量，用于存放附件的图像内存对象 
		VkImageView view;							// 定义一个变量，用于存放附件的图像视图对象 
		VkImageSubresourceRange subresourceRange;	// 定义一个结构体，用于存放附件的图像子资源范围 
		VkAttachmentDescription description;		// 定义一个结构体，用于存放附件的描述信息

		bool hasDepth()								// 定义一个函数，用于判断附件是否包含深度方面
		{
			std::vector<VkFormat> formats = 
			{
				VK_FORMAT_D16_UNORM,				// 16位无符号归一化的深度格式
				VK_FORMAT_X8_D24_UNORM_PACK32,		// 24位无符号归一化的深度格式，打包为32位，忽略最低8位
				VK_FORMAT_D32_SFLOAT,				// 32位浮点数的深度格式
				VK_FORMAT_D16_UNORM_S8_UINT,		// 16位无符号归一化的深度格式，加上8位无符号整数的模板格式
				VK_FORMAT_D24_UNORM_S8_UINT,		// 24位无符号归一化的深度格式，加上8位无符号整数的模板格式
				VK_FORMAT_D32_SFLOAT_S8_UINT,		// 32位浮点数的深度格式，加上8位无符号整数的模板格式
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		bool hasStencil()
		{	// 定义一个函数，用于判断附件是否包含模板方面
			std::vector<VkFormat> formats = 
			{
				VK_FORMAT_S8_UINT,					// 8位无符号整数的模板格式
				VK_FORMAT_D16_UNORM_S8_UINT,		// 16位无符号归一化的深度格式，加上8位无符号整数的模板格式
				VK_FORMAT_D24_UNORM_S8_UINT,		// 24位无符号归一化的深度格式，加上8位无符号整数的模板格式
				VK_FORMAT_D32_SFLOAT_S8_UINT,		// 32位浮点数的深度格式，加上8位无符号整数的模板格式
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		bool isDepthStencil()
		{	// 定义一个函数，用于判断附件是否包含深度或模板方面
			return(hasDepth() || hasStencil());
		}

	};

	// 定义一个结构体，用于描述附件的创建信息 
	struct AttachmentCreateInfo 
	{ 
		uint32_t width, height;				// 定义两个变量，用于存放附件的宽度和高度 
		uint32_t layerCount;				// 定义一个变量，用于存放附件的层数 
		VkFormat format;					// 定义一个变量，用于存放附件的格式 
		VkImageUsageFlags usage;			// 定义一个变量，用于存放附件的用途标志 
		VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT; // 定义一个变量，用于存放附件的采样数，初始值为VK_SAMPLE_COUNT_1_BIT，表示每个像素只有一个采样点 
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
		{	// 定义一个函数，用于添加帧缓冲区附件
			Cetus::FramebufferAttachment attachment;

			// 设定附件格式
			attachment.format = createinfo.format;

			// 设定附件图像
			VkImageCreateInfo image = Cetus::initializers::imageCreateInfo();
			image.imageType = VK_IMAGE_TYPE_2D;			// 设置图像的类型，固定为VK_IMAGE_TYPE_2D，表示二维图像
			image.format = createinfo.format;			// 设置图像的格式，来自附件创建信息中的格式
			image.extent.width = createinfo.width;		// 设置图像的宽度，来自附件创建信息中的宽度
			image.extent.height = createinfo.height;	// 设置图像的高度，来自附件创建信息中的高度
			image.extent.depth = 1;						// 设置图像的深度，固定为1，表示只有一层
			image.mipLevels = 1;						// 设置图像的mip等级，固定为1，表示只有一个mip等级
			image.arrayLayers = createinfo.layerCount;	// 设置图像的数组层数，来自附件创建信息中的层数
			image.samples = createinfo.imageSampleCount;// 设置图像的采样数，来自附件创建信息中的采样数
			image.tiling = VK_IMAGE_TILING_OPTIMAL;		// 设置图像的平铺方式，固定为VK_IMAGE_TILING_OPTIMAL，表示使用最佳的平铺方式
			image.usage = createinfo.usage;				// 设置图像的用途，来自附件创建信息中的用途
			VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &image, nullptr, &attachment.image));

			// 设定附件图像内存
			VkMemoryAllocateInfo memAlloc = Cetus::initializers::memoryAllocateInfo();
			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, attachment.image, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAlloc, nullptr, &attachment.memory));
			VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, attachment.image, attachment.memory, 0));


			// 设定附件子资源范围
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
			attachment.subresourceRange = {};								// 将附件的子资源范围结构体初始化为0
			attachment.subresourceRange.aspectMask = aspectMask;			// 将附件的子资源范围结构体中的方面掩码设置为之前计算的方面掩码
			attachment.subresourceRange.levelCount = 1;						// 将附件的子资源范围结构体中的mip等级数量设置为1，表示只有一个mip等级
			attachment.subresourceRange.layerCount = createinfo.layerCount; // 将附件的子资源范围结构体中的数组层数量设置为附件创建信息中的层数

			// 创建图像视图
			// 一个图像可以有多个不同类型的图像视图。图像视图是指图像的一个子集，可以用VkImageViewCreateInfo结构体来描述。
			//	VkImageViewCreateInfo包含以下成员：
			// 	viewType：一个VkImageViewType枚举值，用来指定图像数据应该如何被解释。
			//		viewType参数允许你把图像视为1D纹理，2D纹理，3D纹理和立方体贴图等等。
			// 	format：一个VkFormat枚举值，用来指定图像的格式。
			// 	components：一个VkComponentMapping结构体，用来指定颜色通道的交换方式。
			//		例如，你可以把所有的通道映射到红色通道，得到一个单色纹理。你也可以把0和1的常量值映射到一个通道。
			// 	subresourceRange：一个VkImageSubresourceRange结构体，用来描述图像的用途和应该访问的图像的哪一部分。
			//		例如，如果它应该被视为一个没有任何mipmap层级的2D纹理深度纹理。
			VkImageViewCreateInfo imageView = Cetus::initializers::imageViewCreateInfo();
			imageView.viewType = (createinfo.layerCount == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;	// 设置图像视图的类型，根据附件创建信息中的层数，如果为1，就设置为VK_IMAGE_VIEW_TYPE_2D，表示二维图像视图，否则设置为VK_IMAGE_VIEW_TYPE_2D_ARRAY，表示二维数组图像视图
			imageView.format = createinfo.format;																		// 设置图像视图的格式，来自附件创建信息中的格式
			imageView.subresourceRange = attachment.subresourceRange;													// 设置图像视图的子资源范围，来自附件的子资源范围结构体
			imageView.subresourceRange.aspectMask = (attachment.hasDepth()) ? VK_IMAGE_ASPECT_DEPTH_BIT : aspectMask;	// 设置图像视图的子资源范围中的方面掩码，根据附件是否包含深度方面，如果是，就设置为VK_IMAGE_ASPECT_DEPTH_BIT，表示只包含深度方面，否则设置为之前计算的方面掩码
			// 上面这段源代码的注释todo: workaround for depth + stencil attachments意思是这是一个临时的解决方案，用于处理深度和模板附件的问题。
			//	具体的问题是，如果附件同时包含深度和模板方面，那么在创建图像视图时，不能只选择其中一个方面，而必须选择所有的方面，否则会导致验证层的错误。这可能是Vulkan规范的一个缺陷或者不一致。
			// 为了避免这个问题，这段代码在设置aspectMask时，使用了一个条件表达式，如果附件包含深度方面，就设置为VK_IMAGE_ASPECT_DEPTH_BIT，表示只选择深度方面，
			//  否则设置为之前计算的方面掩码，表示选择颜色或者深度模板方面。这样做的目的是为了让图像视图的方面掩码和附件的描述信息结构体中的方面掩码一致，从而避免验证层的错误。
			imageView.image = attachment.image;																			// 设置图像视图的图像对象，来自附件的图像对象
			VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &imageView, nullptr, &attachment.view));

			// 创建附件描述
			attachment.description = {};
			attachment.description.samples = createinfo.imageSampleCount;				// 将附件的描述信息结构体中的采样数设置为附件创建信息中的采样数
			attachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// 将附件的描述信息结构体中的加载操作设置为VK_ATTACHMENT_LOAD_OP_CLEAR，表示在渲染开始时，清除附件的内容
			// 将附件的描述信息结构体中的存储操作设置为根据附件创建信息中的用途标志，如果包含VK_IMAGE_USAGE_SAMPLED_BIT，表示附件可以作为采样图像，
			// 就设置为VK_ATTACHMENT_STORE_OP_STORE，表示在渲染结束时，保留附件的内容，
			// 否则设置为VK_ATTACHMENT_STORE_OP_DONT_CARE，表示在渲染结束时，不关心附件的内容
			attachment.description.storeOp = (createinfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE; 
			attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// 将附件的描述信息结构体中的模板加载操作设置为VK_ATTACHMENT_LOAD_OP_DONT_CARE，表示在渲染开始时，不关心附件的模板内容
			attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// 将附件的描述信息结构体中的模板存储操作设置为VK_ATTACHMENT_STORE_OP_DONT_CARE，表示在渲染结束时，不关心附件的模板内容
			attachment.description.format = createinfo.format;							// 将附件的描述信息结构体中的格式设置为附件创建信息中的格式
			attachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// 将附件的描述信息结构体中的初始布局设置为VK_IMAGE_LAYOUT_UNDEFINED，表示在渲染开始时，不关心附件的图像布局
			if (attachment.hasDepth() || attachment.hasStencil())						// 如果附件包含深度或模板方面，使用附件对象的isDepthStencil函数进行判断
			{	// 将附件的描述信息结构体中的最终布局设置为VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL，表示在渲染结束时，将附件的图像布局转换为深度模板只读的最佳布局
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; 
			}
			else
			{	// 将附件的描述信息结构体中的最终布局设置为VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL，表示在渲染结束时，将附件的图像布局转换为着色器只读的最佳布局
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; 
			}
			attachments.push_back(attachment);

			return static_cast<uint32_t>(attachments.size() - 1);
		}

		VkResult createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode adressMode)
		{	// 定义一个函数，用于创建采样器对象
			// 参数是三个枚举值，分别表示放大过滤器，缩小过滤器和寻址模式 
			VkSamplerCreateInfo samplerInfo = Cetus::initializers::samplerCreateInfo();
			samplerInfo.magFilter = magFilter;		// 将采样器创建信息结构体中的放大过滤器设置为参数magFilter，表示当采样的纹素大于像素时，使用的过滤方式 
			samplerInfo.minFilter = minFilter;		// 将采样器创建信息结构体中的缩小过滤器设置为参数minFilter，表示当采样的纹素小于像素时，使用的过滤方式 
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // 将采样器创建信息结构体中的mip贴图模式设置为VK_SAMPLER_MIPMAP_MODE_LINEAR，表示当采样的mip等级不是整数时，使用线性插值的方式 
			samplerInfo.addressModeU = adressMode;	// 将采样器创建信息结构体中的U方向寻址模式设置为参数adressMode，表示当采样的纹理坐标超出[0,1]范围时，U方向的处理方式 
			samplerInfo.addressModeV = adressMode;	// 将采样器创建信息结构体中的V方向寻址模式设置为参数adressMode，表示当采样的纹理坐标超出[0,1]范围时，V方向的处理方式 
			samplerInfo.addressModeW = adressMode;	// 将采样器创建信息结构体中的W方向寻址模式设置为参数adressMode，表示当采样的纹理坐标超出[0,1]范围时，W方向的处理方式 
			samplerInfo.mipLodBias = 0.0f;			// 将采样器创建信息结构体中的mip等级偏移量设置为0.0f，表示不对mip等级进行偏移 
			samplerInfo.maxAnisotropy = 1.0f;		// 将采样器创建信息结构体中的最大各向异性设置为1.0f，表示不使用各向异性过滤 
			samplerInfo.minLod = 0.0f;				// 将采样器创建信息结构体中的最小mip等级设置为0.0f，表示从最大的mip等级开始采样 
			samplerInfo.maxLod = 1.0f;				// 将采样器创建信息结构体中的最大mip等级设置为1.0f，表示到最小的mip等级结束采样 
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; // 将采样器创建信息结构体中的边界颜色设置为VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE，表示当采样的纹理坐标超出[0,1]范围时，使用不透明的白色作为边界颜色 
			return vkCreateSampler(vulkanDevice->logicalDevice, &samplerInfo, nullptr, &sampler);
		}

		VkResult createRenderPass()
		{
			std::vector<VkAttachmentDescription> attachmentDescriptions;
			for (auto& attachment : attachments)
			{
				attachmentDescriptions.push_back(attachment.description);
			};


			// 设置子通道附件，及子通道的处理对象，它们是渲染操作的输入或输出；
			std::vector<VkAttachmentReference> colorReferences;	// 定义一个std::vector容器，存储VkAttachmentReference结构体，表示颜色附件的引用信息，如附件的索引和布局
			VkAttachmentReference depthReference = {};			// 定义一个VkAttachmentReference结构体，表示深度模板附件的引用信息，用默认值初始化
			bool hasDepth = false;								// 定义一个布尔变量，表示是否有深度模板附件，初始值为false
			bool hasColor = false;								// 定义一个布尔变量，表示是否有颜色附件，初始值为false

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


			// 设置子通道依赖
			std::array<VkSubpassDependency, 2> dependencies;	// 定义一个std::array容器，存储VkSubpassDependency结构体，表示子通道之间的依赖关系，容器的大小为2，表示有两个依赖关系

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;	// 表示依赖的源subpass是渲染通道之外的命令，例如前面的传输或计算命令
			dependencies[0].dstSubpass = 0;						// 表示依赖的目标subpass是渲染通道中的第一个subpass，即subpass 0
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;			// 表示依赖的源阶段是管线的最后一个阶段，即底部阶段，表示在这个阶段之前的所有命令都必须完成
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// 表示依赖的目标阶段是管线的颜色附件输出阶段，即在片段着色器之后，将颜色值写入颜色附件的阶段
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;						// 表示依赖的源访问掩码是内存读取，即在源阶段之前的所有命令都必须完成对内存的读取操作
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // 表示依赖的目标访问掩码是颜色附件读取和写入，即在目标阶段之后的所有命令都必须等待对颜色附件的读取和写入操作完成
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;					// 表示依赖的标志是按区域的，即只有在相同区域的源和目标命令之间才需要同步，而不是整个帧缓冲区
			// 在开始渲染通道之前，确保所有的外部命令都已经完成，避免数据竞争或不一致的问题 。
			// 0之前，确保颜色附件的内容已经准备好，避免读取无效的数据或覆盖有效的数据 。
			// 0之后，确保颜色附件的内容已经写入，避免后续的命令或subpass读取错误的数据或修改未完成的数据 。
 
			dependencies[1].srcSubpass = 0;						// 将第二个依赖关系的srcSubpass成员（表示源子通道的索引）赋值为0，表示该依赖关系的源是第一个子通道
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;	// 将第二个依赖关系的dstSubpass成员（表示目标子通道的索引）赋值为VK_SUBPASS_EXTERNAL，表示该依赖关系的目标是外部的
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// 将第二个依赖关系的srcStageMask成员（表示源阶段掩码）赋值为VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT，表示该依赖关系的源是颜色附件输出阶段
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;			// 将第二个依赖关系的dstStageMask成员（表示目标阶段掩码）赋值为VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT，表示该依赖关系的目标是管线的最后一个阶段
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // 将第二个依赖关系的srcAccessMask成员（表示源访问掩码）赋值为VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT，表示该依赖关系的源需要读写颜色附件
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;						// 将第二个依赖关系的dstAccessMask成员（表示目标访问掩码）赋值为VK_ACCESS_MEMORY_READ_BIT，表示该依赖关系的目标需要读取内存
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;					// 将第二个依赖关系的dependencyFlags成员（表示依赖标志）赋值为VK_DEPENDENCY_BY_REGION_BIT，表示该依赖关系是按区域的，即只影响相同区域的像素


			// 创建渲染通道
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = attachmentDescriptions.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();
			VK_CHECK_RESULT(vkCreateRenderPass(vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &renderPass));


			// 创建帧缓冲
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