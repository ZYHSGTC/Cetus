#pragma once

#include <stdlib.h>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "VulkanTools.h"

typedef struct _SwapChainBuffers {
	VkImage image;
	VkImageView view;
} SwapChainBuffer;

class VulkanSwapChain
{
private: 
	VkInstance	instance;
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;
public:
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;	
	uint32_t queueNodeIndex = UINT32_MAX;
	
	uint32_t imageCount;
	std::vector<SwapChainBuffer> buffers;

	VkFormat		colorFormat;
	VkColorSpaceKHR colorSpace;

	void		connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
	void		initSurface(void* platformHandle, void* platformWindow);
	void		create(uint32_t* width, uint32_t* height, bool vsync = false, bool fullscreen = false);
	VkResult	acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
	VkResult	queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);
	void		cleanup();
};
