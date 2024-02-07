#include "VulkanSwapChain.h"

void VulkanSwapChain::connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{	// �ý�������װ��vulkanʾ�����豸�ϣ�
	this->instance = instance;
	this->physicalDevice = physicalDevice;
	this->device = device;
}

void VulkanSwapChain::initSurface(void* platformHandle, void* platformWindow)
{
	// ��ʼ������
	VkResult err = VK_SUCCESS;
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
	surfaceCreateInfo.hwnd = (HWND)platformWindow;
	err = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
	if (err != VK_SUCCESS) {
		Cetus::tools::exitFatal("Could not create surface!", err);	// �����Զ���ĺ��������������Ϣ���˳�����
	}

	// ���ͬʱ֧��ͼ������ֵĶ���
	uint32_t queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
	assert(queueCount >= 1);
	std::vector<VkQueueFamilyProperties> queueProps(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

	std::vector<VkBool32> supportsPresent(queueCount);
	for (uint32_t i = 0; i < queueCount; i++) 
	{	// ����vkGetPhysicalDeviceSurfaceSupportKHR���������������豸���󣬶������������������Ͳ���ֵ�ĵ�ַ����ȡ�������Ƿ�֧�ֱ�����ֵĲ���ֵ
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);
	}

	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
	uint32_t presentQueueNodeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < queueCount; i++) 
	{
		if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)	// �������������԰���VK_QUEUE_GRAPHICS_BIT����ʾ���������֧��ͼ�β���
		{
			if (graphicsQueueNodeIndex == UINT32_MAX)	// ���ͼ�ζ������������û�б���ֵ
			{
				graphicsQueueNodeIndex = i;				// ����ǰ�������������ֵ��ͼ�ζ����������
			}

			if (supportsPresent[i] == VK_TRUE)			// �����ǰ������֧�ֱ������
			{
				graphicsQueueNodeIndex = i;				// ����ǰ�������������ֵ��ͼ�ζ����������
				presentQueueNodeIndex = i;				// ����ǰ�������������ֵ�����ֶ����������
				break;
			}
		}
	}
	if (presentQueueNodeIndex == UINT32_MAX)			// ������ֶ������������û�б���ֵ����ʾû���ҵ�֧�ֱ�����ֵĶ�����
	{	// ����û��ͼ�ζ��й��ܵĳ��ֶ����������
		for (uint32_t i = 0; i < queueCount; ++i)		// ����ÿ��������
		{
			if (supportsPresent[i] == VK_TRUE)			// �����ǰ������֧�ֱ������
			{
				presentQueueNodeIndex = i;				// ����ǰ�������������ֵ�����ֶ����������
				break;
			}
		}
	}
	if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX) 
	{	// ���ͼ�ζ�������߳��ֶ����������������Чֵ����ʾû���ҵ����ʵĶ�����
		Cetus::tools::exitFatal("Could not find a graphics and/or presenting queue!", -1);
	}	// �����Զ���ĺ��������������Ϣ���˳�����

	if (graphicsQueueNodeIndex != presentQueueNodeIndex) 
	{	// ���ͼ�ζ�����ͳ��ֶ��������������ͬ����ʾ�����ǲ�ͬ�Ķ�����
		Cetus::tools::exitFatal("Separate graphics and presenting queues are not supported yet!", -1);
	}	// �����Զ���ĺ��������������Ϣ���˳�������ΪĿǰ����֧�ַ����ͼ�κͳ��ֶ���

	queueNodeIndex = graphicsQueueNodeIndex;


	// ��ʼ���������ɫ��ʽ����ɫ�ռ�
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
{	// ����һ�����������ڴ��������������پɽ�����(�����ؽ�)������������ͼ����ͼ����ͼ
	// �����ǽ������Ŀ�Ⱥ͸߶ȵ�ָ�룬�Ƿ����ô�ֱͬ���Ĳ���ֵ���Ƿ�ȫ���Ĳ���ֵ
	// ����һ�����������ڴ�žɵĽ��������󣬳�ʼֵΪ��Ա����swapChain
	VkSwapchainKHR oldSwapchain = swapChain;

	// ����һ���ṹ�壬���ڴ�ű����������Ϣ
	VkSurfaceCapabilitiesKHR surfCaps;
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCaps));

	// ����һ�����������ڴ�ų���ģʽ����Ϣ����СΪ����ģʽ������
	uint32_t presentModeCount;
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL));
	assert(presentModeCount > 0);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

	// ����һ���ṹ�壬���ڴ�Ž������ĳߴ���Ϣ
	VkExtent2D swapchainExtent = {};
	if (surfCaps.currentExtent.width == (uint32_t)-1)	// �������ĵ�ǰ�ߴ�Ŀ����-1����ʾ����ĳߴ��ǿɱ��
	{
		swapchainExtent.width = *width;
		swapchainExtent.height = *height;
	}
	else												// �������ĵ�ǰ�ߴ�Ŀ�Ȳ���-1����ʾ����ĳߴ��ǹ̶���
	{
		swapchainExtent = surfCaps.currentExtent;		// ���������ĳߴ�����Ϊ����ĵ�ǰ�ߴ�
		*width = surfCaps.currentExtent.width;
		*height = surfCaps.currentExtent.height;
	}


	// ����һ�����������ڴ�Ž������ĳ���ģʽ����ʼֵΪVK_PRESENT_MODE_FIFO_KHR����ʾ�Ƚ��ȳ��Ķ���ģʽ
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	if (!vsync)															// ��������ô�ֱͬ��
	{
		for (size_t i = 0; i < presentModeCount; i++)					// ����ÿ�����õĳ���ģʽ
		{
			if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)			// �����ǰ����ģʽ��VK_PRESENT_MODE_MAILBOX_KHR����ʾ����ģʽ�����Ա���˺�ѣ�ͬʱ�������
			{
				swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				break;													// ����ѡ������ģʽ
			}
			if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)		// �����ǰ����ģʽ��VK_PRESENT_MODE_IMMEDIATE_KHR����ʾ����ģʽ����������޶ȵؼ����ӳ٣������ܻ����˺��
			{
				swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
	}

	// ����һ�����������ڴ�Ž�������ͼ����������ʼֵΪ����������Ϣ�е���Сͼ��������һ����ʾ�����ܶ��ʹ�û�����
	uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
	if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
	{	//�������������Ϣ�е����ͼ����������0���ҽ�������ͼ�������������ͼ������
		desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
	}

	// ����һ�����������ڴ�Ž�������Ԥ�任��־
	VkSurfaceTransformFlagsKHR preTransform;
	if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{	// �������֧��VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR��־����ʾ����Ҫ��ͼ������κα任
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{	// ������治֧��VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR��־
		// ����������Ԥ�任��־����Ϊ����ĵ�ǰ�任��־����ʾ���ձ����Ҫ����б任 
		preTransform = surfCaps.currentTransform;
	}

	// ����һ�����������ڴ�Ž������ĸ���͸���ȱ�־����ʼֵΪVK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR����ʾͼ���͸���Ȳ���Ӱ������ͼ��
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
		// ����һ�����������ڴ�����ȵĸ���͸���ȱ�־���������ȼ�����
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,			// ��ʾͼ���͸���Ȳ���Ӱ������ͼ��
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,	// ��ʾͼ�����ɫֵ�Ѿ�Ԥ����͸����
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,	// ��ʾͼ�����ɫֵ��Ҫ���͸����
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,			// ��ʾͼ���͸���Ȼ�̳��Դ���ϵͳ
	};
	for (auto& compositeAlphaFlag : compositeAlphaFlags) {
		if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		};
	}

	// ����������������һ���ṹ�壬���������������Ĵ�����Ϣ 
	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.surface = surface;
	swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
	swapchainCI.imageFormat = colorFormat;
	swapchainCI.imageColorSpace = colorSpace;
	swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
	swapchainCI.imageArrayLayers = 1;
	swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;			// ���ý�������ͼ����;��ΪVK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT����ʾͼ�������Ϊ��ɫ����
	swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;				// ���ý�������ͼ����ģʽ���̶�ΪVK_SHARING_MODE_EXCLUSIVE����ʾÿ��ͼ��ֻ�ܱ�һ��������ӵ��
	swapchainCI.queueFamilyIndexCount = 0;									// ���ý������Ķ����������������̶�Ϊ0����ʾ����Ҫָ����������������û�н��а�װ
	swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchainCI.compositeAlpha = compositeAlpha;
	swapchainCI.presentMode = swapchainPresentMode;
	swapchainCI.clipped = VK_TRUE;											// ���ý������Ĳü���־���̶�ΪVK_TRUE����ʾ����Ҫ���ֱ��ڵ�������
	swapchainCI.oldSwapchain = oldSwapchain;								// ���ý������ľɽ�������������֮ǰ����ı���oldSwapchain����ʾ�����´���������ʱ�����Ը��þɽ���������Դ
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		// �������֧��VK_IMAGE_USAGE_TRANSFER_SRC_BIT��־����ʾͼ�������Ϊ����Դ
		// ����������ͼ����;����VK_IMAGE_USAGE_TRANSFER_SRC_BIT��־����ʾͼ�������Ϊ����Դ
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		// �������֧��VK_IMAGE_USAGE_TRANSFER_DST_BIT��־����ʾͼ�������Ϊ����Ŀ��
		// ����������ͼ����;����VK_IMAGE_USAGE_TRANSFER_DST_BIT��־����ʾͼ�������Ϊ����Ŀ��
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain));


	// ���پɽ�����
	if (oldSwapchain != VK_NULL_HANDLE) 
	{	// ����ɽ����������ǿ�ָ�룬��ʾ��Ҫ���پɽ�����
		for (uint32_t i = 0; i < imageCount; i++)
		{
			vkDestroyImageView(device, buffers[i].view, nullptr);
		}
		vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
	}
	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL));

	
	// ���´���������ͼ����ͼ����ͼ
	// ����vkGetSwapchainImagesKHR�����������豸���󣬽���������ͼ�������ĵ�ַ������������ָ�룬��ȡ��������ͼ�����
	std::vector<VkImage> images;
	images.resize(imageCount);
	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

	buffers.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++)
	{
		buffers[i].image = images[i];

		// ����һ���ṹ�壬��������ͼ����ͼ�Ĵ�����Ϣ
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = colorFormat;
		colorAttachmentView.components = {
			// ����ͼ����ͼ����ɫ����ӳ�䣬��ʾ����Ҫ�ı���ɫ������˳���ֵ 
			VK_COMPONENT_SWIZZLE_R, // R����ӳ��ΪR���� 
			VK_COMPONENT_SWIZZLE_G, // G����ӳ��ΪG���� 
			VK_COMPONENT_SWIZZLE_B, // B����ӳ��ΪB���� 
			VK_COMPONENT_SWIZZLE_A	// A����ӳ��ΪA����
		}; 
		// ����ͼ����ͼ������Դ��Χ�ķ������룬�̶�ΪVK_IMAGE_ASPECT_COLOR_BIT����ʾֻ������ɫ����
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;	// ����ͼ����ͼ������Դ��Χ�Ļ���mip�ȼ����̶�Ϊ0����ʾ������mip�ȼ���ʼ 
		colorAttachmentView.subresourceRange.levelCount = 1;	// ����ͼ����ͼ������Դ��Χ��mip�ȼ��������̶�Ϊ1����ʾֻ��һ��mip�ȼ� 
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;// ����ͼ����ͼ������Դ��Χ�Ļ�������㣬�̶�Ϊ0����ʾ�ӵ�һ������㿪ʼ 
		colorAttachmentView.subresourceRange.layerCount = 1;	// ����ͼ����ͼ������Դ��Χ��������������̶�Ϊ1����ʾֻ��һ������� 
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;	// ����ͼ����ͼ�����ͣ��̶�ΪVK_IMAGE_VIEW_TYPE_2D����ʾ��άͼ��
		colorAttachmentView.flags = 0;
		colorAttachmentView.image = buffers[i].image;

		VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr, &buffers[i].view));
	}
}

VkResult VulkanSwapChain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex)
{
	// ����vkAcquireNextImageKHR�����������豸���󣬽��������󣬳�ʱʱ�䣨���޴󣩣��ź������󣬿�ָ�루��ʾ��ʹ��Χ�����󣩣�ͼ�������ĵ�ַ����ȡ����������һ������ͼ�񣬲�����һ��VkResultֵ����ʾ�����Ľ��
	return vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, imageIndex);
}

VkResult VulkanSwapChain::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore)
{
	// ����һ�����������ڽ���������ͼ���ύ��������
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;
	if (waitSemaphore != VK_NULL_HANDLE)
	{	// �������waitSemaphore���ǿ�ָ�룬��ʾ��Ҫ�ȴ��ź��� 
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &waitSemaphore;
	}
	return vkQueuePresentKHR(queue, &presentInfo);
}

void VulkanSwapChain::cleanup()
{
	if (swapChain != VK_NULL_HANDLE)
	{	// �����Ա����swapChain���ǿ�ָ�룬��ʾ�������������
		// ����ÿ��������ͼ��
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