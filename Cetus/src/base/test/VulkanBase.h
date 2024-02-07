#pragma once

#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <ShellScalingAPI.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <numeric>
#include <ctime>
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include <sys/stat.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

#include "vulkan/vulkan.h"
#include <vulkan/vulkan_win32.h>

#include "base/CommandLineParser.hpp"
#include "base/keycodes.hpp"
#include "base/VulkanTools.h"
#include "base/VulkanDebug.h"
#include "base/VulkanSwapChain.h"
#include "base/VulkanBuffer.h"
#include "base/VulkanDevice.h"
#include "base/VulkanTexture.h"
#include "base/VulkanUIOverlay.h"

#include "base/VulkanInitializers.hpp"
#include "base/camera.hpp"

class VulkanBase
{
public:
	VulkanBase(bool enableValidation = false);
	virtual ~VulkanBase();

	bool initVulkan();
	virtual VkResult createInstance(bool enableValidation);
	virtual void getEnabledExtensions();
	virtual void getEnabledFeatures();

	HWND setupWindow(HINSTANCE hinstance, WNDPROC wndproc);

	virtual void prepare();
	virtual void setupRenderPass();
	virtual void setupDepthStencil();
	virtual void setupFrameBuffer();
	virtual void recordCommandBuffers();

	void renderLoop();
	virtual void render() = 0;
	void prepareFrame();
	void submitFrame();
	void drawUI(const VkCommandBuffer commandBuffer);
	virtual void OnUpdateUIOverlay(Cetus::UIOverlay* overlay);

	void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void windowResize();
	virtual void windowResized();
	virtual void viewChanged();
	virtual void keyPressed(uint32_t);
	virtual void mouseMoved(double x, double y, bool& handled);
	virtual void OnHandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

	struct WindowData
	{
		uint32_t width = 1280;
		uint32_t height = 720;
		std::string name = "vulkanExample";
	}windowdata;
	struct Settings {
		bool requiresStencil = false;
		bool validation = false;
		bool fullscreen = false;
		bool overlay = true;
		bool vsync = false;		// ֻ��ı佻��������ģʽ
	} settings;
protected:
	std::vector<std::string> supportedInstanceExtensions;
	// ʵ����չ����������ȫ�ֵģ����ǿ���Ӱ�����е������豸���߼��豸��ʵ����չ��������VK_KHR_��VK_EXT_Ϊǰ׺��
	// �豸��չ���������Ǿֲ��ģ�����ֻӰ�촴�����ǵ��豸��
	std::vector<const char*> enabledInstanceExtensions;
	std::vector<const char*> enabledDeviceExtensions;
	VkPhysicalDeviceFeatures enabledFeatures{};

	// ��һ�� ����ʵ���ͱ���
	VkInstance instance;
	VkPhysicalDevice physicalDevice;

	// �ڶ��� �߼��豸�Ͷ��м���
	Cetus::VulkanDevice* vulkanDevice;
	VkDevice device;
	VkQueue queue;

	// ������ ��������(��Ҫ�豸����������ǰ����֮��)�������뽻����
	HINSTANCE windowInstance;
	HWND window;
	VulkanSwapChain swapChain;
	void* deviceCreatepNextChain = nullptr;

	// ���Ĳ� ��Ⱦ·��
	VkFormat depthFormat;
	VkRenderPass renderPass = VK_NULL_HANDLE;

	// ���岽 ͼ�ι���
	std::vector<VkShaderModule> shaderModules;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkPipelineCache pipelineCache;

	// ������ ͼ����ͼ��֡����
	struct {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} depthStencil;
	std::vector<VkFramebuffer>frameBuffers;

	// ���߲� ����غ������
	VkCommandPool cmdPool;
	std::vector<VkCommandBuffer> drawCmdBuffers;

	// �ڰ˲� ��ѭ��
	VkSubmitInfo submitInfo;
	std::vector<VkFence> waitFences;
	struct {
		VkSemaphore presentComplete;
		VkSemaphore renderComplete;
	} semaphores;
	uint32_t currentBuffer = 0;

	bool renderingAllowed = false;
	bool windowResizing = false;
	bool cameraViewUpdated = false;


	Cetus::UIOverlay UIOverlay;
	Camera camera;

	uint32_t FPSCounter = 0;	// ֡����¼��
	uint32_t FPS;				// ÿ��֡��FPS
	float	 frameTimer = 1.0f;	// ÿ֡��ʱ��
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp;	// �ϴθ���֡����ʱ��

	glm::vec2 mousePos;
	struct {
		bool left = false;
		bool right = false;
		bool middle = false;
	} mouseButtons;
private:
	void setupConsole(std::string title);
	void setupDPIAwareness();

	void initSwapchain();
	void setupSwapChain();
	void createPipelineCache();
	void createCommandPool();
	void createCommandBuffers();
	void createSynchronizationPrimitives();

	void destroyCommandBuffers();
	void nextFrame();
	void updateOverlay();
	void handleMouseMove(int32_t x, int32_t y);
	std::string getWindowTitle();
};
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)						\
{																									\
	if (vulkanExample != NULL)																		\
	{																								\
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);									\
	}																								\
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));												\
}																									\
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)									\
{																									\
	vulkanExample = new VulkanExample();															\
	vulkanExample->initVulkan();																	\
	vulkanExample->setupWindow(hInstance, WndProc);													\
	vulkanExample->prepare();																		\
	vulkanExample->renderLoop();																	\
	delete(vulkanExample);																			\
	return 0;																						\
}