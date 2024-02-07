#include "VulkanSwapChain.h"

void VulkanSwapChain::connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{	// 让交换链安装到vulkan示例与设备上；
	this->instance = instance;
	this->physicalDevice = physicalDevice;
	this->device = device;
}

void VulkanSwapChain::initSurface(void* platformHandle, void* platformWindow)
{
	// 初始化表面
	VkResult err = VK_SUCCESS;
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
	surfaceCreateInfo.hwnd = (HWND)platformWindow;
	err = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
	if (err != VK_SUCCESS) {
		Cetus::tools::exitFatal("Could not create surface!", err);	// 调用自定义的函数，输出错误信息并退出程序
	}

	// 获得同时支持图形与呈现的队列
	uint32_t queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
	assert(queueCount >= 1);
	std::vector<VkQueueFamilyProperties> queueProps(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

	std::vector<VkBool32> supportsPresent(queueCount);
	for (uint32_t i = 0; i < queueCount; i++) 
	{	// 调用vkGetPhysicalDeviceSurfaceSupportKHR函数，传入物理设备对象，队列族索引，表面对象和布尔值的地址，获取队列族是否支持表面呈现的布尔值
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);
	}

	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
	uint32_t presentQueueNodeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < queueCount; i++) 
	{
		if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)	// 如果队列族的属性包含VK_QUEUE_GRAPHICS_BIT，表示这个队列族支持图形操作
		{
			if (graphicsQueueNodeIndex == UINT32_MAX)	// 如果图形队列族的索引还没有被赋值
			{
				graphicsQueueNodeIndex = i;				// 将当前队列族的索引赋值给图形队列族的索引
			}

			if (supportsPresent[i] == VK_TRUE)			// 如果当前队列族支持表面呈现
			{
				graphicsQueueNodeIndex = i;				// 将当前队列族的索引赋值给图形队列族的索引
				presentQueueNodeIndex = i;				// 将当前队列族的索引赋值给呈现队列族的索引
				break;
			}
		}
	}
	if (presentQueueNodeIndex == UINT32_MAX)			// 如果呈现队列族的索引还没有被赋值，表示没有找到支持表面呈现的队列族
	{	// 处理没有图形队列功能的呈现队列特殊情况
		for (uint32_t i = 0; i < queueCount; ++i)		// 遍历每个队列族
		{
			if (supportsPresent[i] == VK_TRUE)			// 如果当前队列族支持表面呈现
			{
				presentQueueNodeIndex = i;				// 将当前队列族的索引赋值给呈现队列族的索引
				break;
			}
		}
	}
	if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX) 
	{	// 如果图形队列族或者呈现队列族的索引都是无效值，表示没有找到合适的队列族
		Cetus::tools::exitFatal("Could not find a graphics and/or presenting queue!", -1);
	}	// 调用自定义的函数，输出错误信息并退出程序

	if (graphicsQueueNodeIndex != presentQueueNodeIndex) 
	{	// 如果图形队列族和呈现队列族的索引不相同，表示它们是不同的队列族
		Cetus::tools::exitFatal("Separate graphics and presenting queues are not supported yet!", -1);
	}	// 调用自定义的函数，输出错误信息并退出程序，因为目前还不支持分离的图形和呈现队列

	queueNodeIndex = graphicsQueueNodeIndex;


	// 初始化表面的颜色格式与颜色空间
	uint32_t formatCount;
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL));
	assert(formatCount > 0);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));

	VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];
	std::vector<VkFormat> preferredImageFormats = { 
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R8G8B8A8_UNORM, 
		VK_FORMAT_A8B8G8R8_UNORM_PACK32 
	};

	for (auto& availableFormat : surfaceFormats) {
		if (std::find(preferredImageFormats.begin(), preferredImageFormats.end(), availableFormat.format) != preferredImageFormats.end()) {
			selectedFormat = availableFormat;
			break;
		}
	}

	colorFormat = selectedFormat.format;
	colorSpace = selectedFormat.colorSpace;
}

void VulkanSwapChain::create(uint32_t *width, uint32_t *height, bool vsync, bool fullscreen)
{	// 定义一个函数，用于创建交换链，销毁旧交换链(用于重建)，创建交换链图像与图像视图
	// 参数是交换链的宽度和高度的指针，是否启用垂直同步的布尔值，是否全屏的布尔值
	// 定义一个变量，用于存放旧的交换链对象，初始值为成员变量swapChain
	VkSwapchainKHR oldSwapchain = swapChain;

	// 定义一个结构体，用于存放表面的能力信息
	VkSurfaceCapabilitiesKHR surfCaps;
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCaps));

	// 定义一个向量，用于存放呈现模式的信息，大小为呈现模式的数量
	uint32_t presentModeCount;
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL));
	assert(presentModeCount > 0);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

	// 定义一个结构体，用于存放交换链的尺寸信息
	VkExtent2D swapchainExtent = {};
	if (surfCaps.currentExtent.width == (uint32_t)-1)	// 如果表面的当前尺寸的宽度是-1，表示表面的尺寸是可变的
	{
		swapchainExtent.width = *width;
		swapchainExtent.height = *height;
	}
	else												// 如果表面的当前尺寸的宽度不是-1，表示表面的尺寸是固定的
	{
		swapchainExtent = surfCaps.currentExtent;		// 将交换链的尺寸设置为表面的当前尺寸
		*width = surfCaps.currentExtent.width;
		*height = surfCaps.currentExtent.height;
	}


	// 定义一个变量，用于存放交换链的呈现模式，初始值为VK_PRESENT_MODE_FIFO_KHR，表示先进先出的队列模式
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	if (!vsync)															// 如果不启用垂直同步
	{
		for (size_t i = 0; i < presentModeCount; i++)					// 遍历每个可用的呈现模式
		{
			if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)			// 如果当前呈现模式是VK_PRESENT_MODE_MAILBOX_KHR，表示邮箱模式，可以避免撕裂，同时提高性能
			{
				swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				break;													// 优先选择邮箱模式
			}
			if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)		// 如果当前呈现模式是VK_PRESENT_MODE_IMMEDIATE_KHR，表示立即模式，可以最大限度地减少延迟，但可能会出现撕裂
			{
				swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
	}

	// 定义一个变量，用于存放交换链的图像数量，初始值为表面能力信息中的最小图像数量加一，表示尽可能多地使用缓冲区
	uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
	if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
	{	//如果表面能力信息中的最大图像数量大于0，且交换链的图像数量大于最大图像数量
		desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
	}

	// 定义一个变量，用于存放交换链的预变换标志
	VkSurfaceTransformFlagsKHR preTransform;
	if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{	// 如果表面支持VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR标志，表示不需要对图像进行任何变换
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{	// 如果表面不支持VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR标志
		// 将交换链的预变换标志设置为表面的当前变换标志，表示按照表面的要求进行变换 
		preTransform = surfCaps.currentTransform;
	}

	// 定义一个变量，用于存放交换链的复合透明度标志，初始值为VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR，表示图像的透明度不会影响其他图像
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
		// 定义一个向量，用于存放优先的复合透明度标志，按照优先级排序
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,			// 表示图像的透明度不会影响其他图像
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,	// 表示图像的颜色值已经预乘了透明度
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,	// 表示图像的颜色值需要后乘透明度
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,			// 表示图像的透明度会继承自窗口系统
	};
	for (auto& compositeAlphaFlag : compositeAlphaFlags) {
		if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		};
	}

	// 创建交换链，定义一个结构体，用于描述交换链的创建信息 
	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.surface = surface;
	swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
	swapchainCI.imageFormat = colorFormat;
	swapchainCI.imageColorSpace = colorSpace;
	swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
	swapchainCI.imageArrayLayers = 1;
	swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;			// 设置交换链的图像用途，为VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT，表示图像可以作为颜色附件
	swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;				// 设置交换链的图像共享模式，固定为VK_SHARING_MODE_EXCLUSIVE，表示每个图像只能被一个队列族拥有
	swapchainCI.queueFamilyIndexCount = 0;									// 设置交换链的队列族索引数量，固定为0，表示不需要指定队列族索引，还没有进行安装
	swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchainCI.compositeAlpha = compositeAlpha;
	swapchainCI.presentMode = swapchainPresentMode;
	swapchainCI.clipped = VK_TRUE;											// 设置交换链的裁剪标志，固定为VK_TRUE，表示不需要呈现被遮挡的像素
	swapchainCI.oldSwapchain = oldSwapchain;								// 设置交换链的旧交换链对象，来自之前保存的变量oldSwapchain，表示在重新创建交换链时，可以复用旧交换链的资源
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		// 如果表面支持VK_IMAGE_USAGE_TRANSFER_SRC_BIT标志，表示图像可以作为传输源
		// 将交换链的图像用途加上VK_IMAGE_USAGE_TRANSFER_SRC_BIT标志，表示图像可以作为传输源
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		// 如果表面支持VK_IMAGE_USAGE_TRANSFER_DST_BIT标志，表示图像可以作为传输目标
		// 将交换链的图像用途加上VK_IMAGE_USAGE_TRANSFER_DST_BIT标志，表示图像可以作为传输目标
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain));


	// 销毁旧交换链
	if (oldSwapchain != VK_NULL_HANDLE) 
	{	// 如果旧交换链对象不是空指针，表示需要销毁旧交换链
		for (uint32_t i = 0; i < imageCount; i++)
		{
			vkDestroyImageView(device, buffers[i].view, nullptr);
		}
		vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
	}
	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL));

	
	// 更新创建交换链图像与图像视图
	// 调用vkGetSwapchainImagesKHR函数，传入设备对象，交换链对象，图像数量的地址和向量的数据指针，获取交换链的图像对象
	std::vector<VkImage> images;
	images.resize(imageCount);
	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

	buffers.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++)
	{
		buffers[i].image = images[i];

		// 定义一个结构体，用于描述图像视图的创建信息
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = colorFormat;
		colorAttachmentView.components = {
			// 设置图像视图的颜色分量映射，表示不需要改变颜色分量的顺序或值 
			VK_COMPONENT_SWIZZLE_R, // R分量映射为R分量 
			VK_COMPONENT_SWIZZLE_G, // G分量映射为G分量 
			VK_COMPONENT_SWIZZLE_B, // B分量映射为B分量 
			VK_COMPONENT_SWIZZLE_A	// A分量映射为A分量
		}; 
		// 设置图像视图的子资源范围的方面掩码，固定为VK_IMAGE_ASPECT_COLOR_BIT，表示只包含颜色方面
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;	// 设置图像视图的子资源范围的基础mip等级，固定为0，表示从最大的mip等级开始 
		colorAttachmentView.subresourceRange.levelCount = 1;	// 设置图像视图的子资源范围的mip等级数量，固定为1，表示只有一个mip等级 
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;// 设置图像视图的子资源范围的基础数组层，固定为0，表示从第一个数组层开始 
		colorAttachmentView.subresourceRange.layerCount = 1;	// 设置图像视图的子资源范围的数组层数量，固定为1，表示只有一个数组层 
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;	// 设置图像视图的类型，固定为VK_IMAGE_VIEW_TYPE_2D，表示二维图像
		colorAttachmentView.flags = 0;
		colorAttachmentView.image = buffers[i].image;

		VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr, &buffers[i].view));
	}
}

VkResult VulkanSwapChain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex)
{
	// 调用vkAcquireNextImageKHR函数，传入设备对象，交换链对象，超时时间（无限大），信号量对象，空指针（表示不使用围栏对象），图像索引的地址，获取交换链的下一个可用图像，并返回一个VkResult值，表示操作的结果
	return vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, imageIndex);
}

VkResult VulkanSwapChain::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore)
{
	// 定义一个函数，用于将交换链的图像提交到队列中
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;
	if (waitSemaphore != VK_NULL_HANDLE)
	{	// 如果参数waitSemaphore不是空指针，表示需要等待信号量 
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &waitSemaphore;
	}
	return vkQueuePresentKHR(queue, &presentInfo);
}

void VulkanSwapChain::cleanup()
{
	if (swapChain != VK_NULL_HANDLE)
	{	// 如果成员变量swapChain不是空指针，表示交换链对象存在
		// 遍历每个交换链图像
		for (uint32_t i = 0; i < imageCount; i++)
		{
			vkDestroyImageView(device, buffers[i].view, nullptr);
		}
	}
	if (surface != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
	}
	surface = VK_NULL_HANDLE;
	swapChain = VK_NULL_HANDLE;
}