#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


struct Vertex {             // ����һ���ṹ�� Vertex����λ������ɫ��������
    glm::vec2 pos;          // ����һ�� glm::vec2 ���͵ĳ�Ա���� pos����ʾ�����λ��
    glm::vec3 color;        // ����һ�� glm::vec3 ���͵ĳ�Ա���� color����ʾ�������ɫ

    //  VkVertexInputBindingDescription�ṹ��������������������󶨵���Ϣ�������������ָ�Ӷ��㻺�����л�ȡ�������ݵķ�ʽ��
    //      ÿ�����㻺�����İ󶨺���ָʹ��binding��Ա��������ʶһ�����㻺��������һ���������ڴ�Ĳ�ͬ���֣��ڴ�������ʱ��Ҫ���Ӧ�Ķ�����ɫ��ƥ��
    //      һ�����߿����ж�������������ÿ��������Ӧһ�����㻺��������һ���������ڴ�Ĳ�ͬ���֡�
    //      ����ṹ��������³�Ա��
    //      binding������ṹ�������İ󶨺ţ������ڶ�����ɫ�������ö������ݡ�
    //      stride�����㻺�����Ĳ�����ָʹ��stride��Ա������ָ��������������Ԫ��(�綥��)֮����ֽڿ�ࡣ
    //      inputRate��һ��VkVertexInputRateö��ֵ��ָ���������Եĵ�ַ�Ǹ��ݶ�����������ʵ������������ġ�
    //          �����������ܵ�ֵ��
    //          VK_VERTEX_INPUT_RATE_VERTEX����ʾ�������Եĵ�ַ�Ǹ��ݶ�������������ģ���ÿ�����㶼���Լ�������ֵ��
    //          VK_VERTEX_INPUT_RATE_INSTANCE����ʾ�������Եĵ�ַ�Ǹ���ʵ������������ģ���ÿ��ʵ�������Լ�������ֵ��
    // ����һ����̬��Ա���� getBindingDescription������һ�� VkVertexInputBindingDescription �ṹ�壬��ʾ���㻺�����İ�����
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // ��ʾÿ������ʹ��һ�� Vertex �ṹ�������

        return bindingDescription;
    }

    // VkVertexInputAttributeDescription�ṹ�����������������������Ե���Ϣ����������������ָ�Ӷ��㻺�����л�ȡ���ĵ����������λ�ã���ɫ��6������ṹ��������³�Ա��
    //      location����������ڶ�����ɫ���ж�Ӧ��location��š�layout(position=0) in vec3 aPos
    //      binding��������Դ��ĸ��������л�ȡ���ݡ�
    //          һ�����㻺������һ�� binding������ʾ���㻺������ vkCmdBindVertexBuffers �����еİ󶨺š�
    //          ÿ����������Ҳ��һ�� binding������ʾ�����Դ��ĸ����㻺������ȡ���ݡ�
    //      format������������ݵĴ�С�����ͣ�ʹ��VkFormatö��ֵ��ʾ��
    //      offset�������������ڰ�������Ԫ����ʼλ�õ��ֽ�ƫ������
    // Vulkan�����VkVertexInputAttributeDescription�ṹ�������е�offset��Ա����������ÿ���������Ե�ƫ������
    //      ���ƫ����������ڰ�������Ԫ����ʼλ�õ��ֽ�ƫ������
    //      ���磬���һ�����㻺���������������������ԣ�λ�ú���ɫ��ÿ������ռ��12���ֽڣ���ô��һ�����Ե�ƫ������0���ڶ������Ե�ƫ������12��
    // Vulkan�����VkVertexInputAttributeDescription�ṹ�������е�location��Ա������ȷ��ÿ���������Ե�location��ţ�
    //      �������������ڶ�����ɫ�������ö������ݵġ�
    //      ���磬���һ�����㻺���������������������ԣ�λ�ú���ɫ����ô����ָ����һ�����Ե�location���Ϊ0���ڶ������Ե�location���Ϊ1��
    // ͬһ�󶨵��VkVertexInputAttributeDescription�������ƫ����Ӧ�õ��ڻ�С����һ�󶨵���������VkVertexInputBindingDescription���stride��
    //      ������Ϊstride��ʾ������������Ԫ��֮����ֽڿ�࣬��offset��ʾÿ�����������Ԫ����ʼλ�õ��ֽ�ƫ������
    //      �����ƫ��������stride����ô�ͻᵼ�������ص�������
    // ����һ����̬��Ա���� getAttributeDescriptions������һ����������Ԫ�ص� std::array����ʾ���㻺������ÿ�����Ե�����
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        // ���� attributeDescriptions �ĵ�һ��Ԫ�أ���ʾ pos ���Ե�����
        attributeDescriptions[0].binding = 0;                       // ��ʾ pos ���������Ķ��㻺�����İ󶨺�
        attributeDescriptions[0].location = 0;                      // ��ʾ pos ��������ɫ���е�λ�ú�Ϊ0
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;  // ��ʾ pos ���Ե����ݸ�ʽ������32λ������ (float)
        attributeDescriptions[0].offset = offsetof(Vertex, pos);    // ��ʾ pos ������Vertex�ṹ���е��ֽ�ƫ����

        // ���� attributeDescriptions �ĵڶ���Ԫ�أ���ʾ color ���Ե�����
        attributeDescriptions[1].binding = 0;                       // ��ʾ color ���������Ķ��㻺�����İ󶨺�
        attributeDescriptions[1].location = 1;                      // ��ʾ color ��������ɫ���е�λ�ú�Ϊ1
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;// ��ʾ color ���Ե����ݸ�ʽ������32λ������ (float)
        attributeDescriptions[1].offset = offsetof(Vertex, color);  // ��ʾ color ������ Vertex �ṹ���е��ֽ�ƫ����

        return attributeDescriptions;
    }
};

const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // VkBuffer ��һ��������������Ĳ�͸�������
    //  ������������һ��������������ݽṹ�����������ڶ���Ŀ�ģ�������Ϊ�������ݡ��������ݡ�ͳһ���������洢�������ȡ�
    // VkDeviceMemory ��һ�������豸�ڴ����Ĳ�͸�������
    //  �豸�ڴ������һ�ַ������豸�ϵ��ڴ�飬�����Ա������������ͼ�����ʹ�á�
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();//
        createCommandBuffers();
        createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }

    void cleanupSwapChain() {
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void cleanup() {
        cleanupSwapChain();

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        // ����vkDestroyBuffer�����������豸�����vertexBuffer����Ϳ�ָ�룬����vertexBuffer����
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        // ����vkFreeMemory�����������豸�����vertexBufferMemory����Ϳ�ָ�룬�ͷ�vertexBufferMemory����
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createFramebuffers();
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("examples/vert.spv");
        auto fragShaderCode = readFile("examples/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
        
        // �����Ƕ���ɫ�����л�����������ָ��
        // VkPipelineVertexInputStateCreateInfo�ṹ������������ͼ�ι����еĶ�������״̬����Ϣ�������������ֶΣ�
        // sType������ΪVK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO����ʾ�ṹ������͡�
        // pNext������ָ��һ����չ�ṹ�壬�����ṩ�������Ϣ������ΪNULL��
        // flags�������ֶΣ�����Ϊ0��
        // vertexBindingDescriptionCount��ָ���������������������pVertexBindingDescriptions����ĳ��ȡ�
        // pVertexBindingDescriptions��ָ��һ��VkVertexInputBindingDescription�ṹ�����飬��������ÿ�����㻺�����İ󶨺š��������������ʡ�
        // vertexAttributeDescriptionCount��ָ������������������������pVertexAttributeDescriptions����ĳ��ȡ�
        // pVertexAttributeDescriptions��ָ��һ��VkVertexInputAttributeDescription�ṹ������; 
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;     
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    // ����һ������ createVertexBuffer�����ڴ������㻺����
    void createVertexBuffer() {
        // VkBufferCreateInfo �ṹ����������������������Ĳ����������������ֶΣ�
        //  sType:��ΪVK_STRUCTURE_TYPE_BUFFER_CREATE_INFO��
        //  pNext:NULL����һ��ָ����չ�ṹ���ָ�롣
        //  flags:һ��VkBufferCreateFlagsλ���룬��ʾ����������Ķ�������������Ƿ�֧��ϡ��󶨡�ϡ�����Ż�ϡ������ȡ�
        //      Ŀǰ��Vulkan API ֻ������һ����Ч�� VkBufferCreateFlags ֵ���� VK_BUFFER_CREATE_SPARSE_BINDING_BIT������ʾ����������֧��ϡ��󶨡�
        //      ϡ�����һ��������������ֻ�󶨲����ڴ�ļ���������������ڴ������ʺ����ܡ�
        //  size:һ�� VkDeviceSize ֵ����ʾ������������ֽڴ�С��
        //  usage:һ�� VkBufferUsageFlags λ���룬��ʾ�������������;�������Ƿ��������㻺������������������ͳһ���������洢�������ȡ�
        //      VkBufferUsageFlags����Щֵ��
        //      VK_BUFFER_USAGE_TRANSFER_SRC_BIT����ʾ���������������Ϊ���������Դ��
        //      VK_BUFFER_USAGE_TRANSFER_DST_BIT����ʾ���������������Ϊ���������Ŀ�ꡣ
        //      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT����ʾ���������������Ϊͳһ�����������ڴ洢��ɫ���еĳ������ݡ�
        //      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT����ʾ���������������Ϊ�洢�����������ڴ洢��ɫ���еĿɱ����ݡ�
        //      VK_BUFFER_USAGE_INDEX_BUFFER_BIT����ʾ���������������Ϊ���������������ڴ洢���������е��������ݡ�
        //      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT����ʾ���������������Ϊ���㻺���������ڴ洢��������״̬�еĶ������ݡ�
        //  sharingMode��һ�� VkSharingMode ֵ����ʾ����������Ĺ���ģʽ�������Ƿ���Ա������������ʡ�
        //      VK_SHARING_MODE_EXCLUSIVE����ʾ����������ֻ�ܱ�һ��������ʹ�ã���ʹ��ǰ�����������Ȩת�ơ�
        //      VK_SHARING_MODE_CONCURRENT����ʾ������������Ա����������ͬʱʹ�ã������������Ȩת�ơ�
        //  queueFamilyIndexCount��һ�� uint32_t ֵ����ʾ pQueueFamilyIndices �����Ԫ�ظ�����
        //  pQueueFamilyIndices��һ��ָ�� uint32_t ֵ�����飬��ʾ���Է��ʸû���������Ķ�����������
        //      ��ֻ�ڹ���ģʽΪ VK_SHARING_MODE_CONCURRENT ʱ��Ч������������ʵ�ֿ���������豸����Դ����
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(vertices[0]) * vertices.size();
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }


        // VkMemoryRequirements �ṹ����������ʾ�����������ͼ�������ڴ�����ġ������������ֶΣ�
        //  size����ʾ������ڴ����ֽڴ�С��
        //  alignment����ʾ������ڴ����ֽڶ���ֵ��
        //  memoryTypeBits����ʾ֧�ָö�����ڴ����͵�λ���롣
        //      λi������1ʱ����ζ�������豸�ڴ�������(�ҵ�����3��)�ĵ�i���ڴ�����֧�ָû���������
        VkMemoryRequirements memRequirements;   // ������������ȡ������������ڴ������
        vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

        // VkMemoryAllocateInfo �ṹ�������������豸�ڴ����Ĳ����������������ֶΣ�
        //  sType����ΪVK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO��
        //  pNext��NULL ����һ��ָ����չ�ṹ���ָ�롣
        //  allocationSize��һ�� VkDeviceSize ֵ����ʾҪ������ڴ����ֽڴ�С��
        //  memoryTypeIndex��һ�� uint32_t ֵ����ʾ�������豸�ڴ�������ѡ����ڴ����͵�������
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;    //��һ���ҳ���һ��֧�ָû�����������ڴ����͵������������ҵ�����1��Ȼ����Ϊ�������ڴ�����
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        // ���㻺�������ڴ���ζ�Ž�һ�� VkBuffer �����һ�� VkDeviceMemory �������������
        // ʹ�ö��㻺�������Է����豸�ڴ��е����ݡ�����Ҫ���� vkBindBufferMemory ���������ṩ���ߵľ���Լ�һ���ֽ�ƫ����
        vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

        void* data;
        // vkMapMemory ������������һ���豸�ڴ����ӳ�䵽������ַ�ռ��1�����������²�����
        //  device��ָ��ӵ�и��ڴ������߼��豸�����
        //  memory��ָ��Ҫӳ����豸�ڴ��������
        //  offset��ָ�����ڴ����ʼ���ֽ�ƫ������
        //  size��ָ��Ҫӳ����ڴ淶Χ���ֽڴ�С������ VK_WHOLE_SIZE ��ʾ�� offset ���ڴ����ĩβ��
        //  flags������Ϊδ��ʹ�ã�����Ϊ 0��
        //  ppData��ָ��һ�� void* �����ĵ�ַ�����ڷ���ӳ�䷶Χ�������ɷ���ָ�롣
        vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferInfo.size); // �� vertices �����е����ݸ��Ƶ�ӳ���ڴ�������
        vkUnmapMemory(device, vertexBufferMemory);              // ȡ��ӳ�� vertexBufferMemory ����
    }

    // ��һ������Ϊ�������ʺϵ��ڴ�����(�ж��)���ڶ�������������Ҫ�����͡�����Ŀ�����ҵ���һ�����ʺϵ�ǰ�������ģ�ͬʱ����Щ�ʺϵĻ�����ѡ��ӵ������MemoryProperty���ڴ�����������
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        // Vulkan�е��ڴ��Ϊ�����ࣺHost Memory �� Device Memory��
        //  Host Memory ��ָ CPU ����ֱ�ӷ��ʵ�ϵͳ�ڴ棬Device Memory ��ָ GPU ����ֱ�ӷ��ʵ��Կ��ڴ档
        // �ڴ����һ���ڴ����Ļ��ƣ������Զ�̬�ط�����ͷ��ڴ�顣�ڴ��ͨ���ɲ���ϵͳ������ʱ���ṩ��Ҳ�����ɳ���Ա�Լ�ʵ�֡������������ֶΣ�

        // VkPhysicalDeviceMemoryProperties ��һ���ṹ�壬���������������豸���ڴ�����2�������������ֶΣ�
        //  memoryTypeCount����ʾmemoryTypes�������ЧԪ�ظ������ҵ�����3����
        //  memoryTypes��һ������ VK_MAX_MEMORY_TYPES �� VkMemoryType �ṹ������飬��ʾ�������������ڴ�ѵ��ڴ����͡�
        //      propertyFlags��һ�� VkMemoryPropertyFlags λ���룬��ʾ���ڴ����͵����ԣ������Ƿ���Ա�����ӳ����ʡ��Ƿ����������ǻ���ĵȡ�
        //          VkMemoryPropertyFlags ��һ�� VkFlags ���͵�λ���룬������ָ���ڴ����͵����ԡ�
        //          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT��������Ա�ʾ����ڴ��Ƿ������豸���صģ�Ҳ�����Կ����Դ档
        //              �����ڴ�����Կ�������˵�����ģ����Ƕ��� CPU ������˵���ܺ������߲����á�ʹ�������ڴ�������ͼ����Ⱦ�����ܣ�������Ҫע�����ݵĴ����ͬ����
        //          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT��������Ա�ʾ����ڴ���Ա� CPU ���ʣ�Ҳ���ǿ���ͨ�� vkMapMemory ����ӳ�䵽 CPU �ĵ�ַ�ռ䡣
        //              �����ڴ���������洢 CPU ���ɻ��޸ĵ����ݣ�Ȼ�󴫵ݸ��Կ�ʹ�á�ʹ�������ڴ���Է��� CPU �� GPU �����ݽ�����������Ҫע���ڴ��һ���Ժͻ������⡣
        //          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT��������Ա�ʾ����ڴ��� CPU �� GPU ֮����һ�µģ�Ҳ����˵����Ҫ���� vkFlushMappedMemoryRanges �� vkInvalidateMappedMemoryRanges ������ˢ�»�ʧЧ CPU �Ļ��档
        //              �����ڴ���Լ� CPU �� GPU ������ͬ�������ǿ��ܻ�����һЩ���ܻ������ĸ���ĵ�����
        //          VK_MEMORY_PROPERTY_HOST_CACHED_BIT��������Ա�ʾ����ڴ��� CPU �����л���ģ�Ҳ����˵ CPU ��������ڴ���û�л���Ŀ졣
        //              ���ǣ��л�����ڴ���ܲ���һ�µģ�Ҳ����˵��Ҫ���� vkFlushMappedMemoryRanges �� vkInvalidateMappedMemoryRanges ��������֤ CPU �� GPU ֮���������ȷ�ԡ�
        //              ʹ�������ڴ������� CPU �ķ����ٶȣ�������Ҫע�⻺������ͬ�����⡣
        //          VK_MEMORY_PROPERTY_HOST_CACHED_BIT����ʾ�����ڴ����ͷ�����ڴ����������л��档��������δ������ڴ�ȷ��ʻ�����ڴ�������δ������ڴ���������һ�µ�1��
        //          VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT����ʾ�����ڴ�����ֻ�����豸�����ڴ档�ڴ����Ͳ���ͬʱ����VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT��VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT�����⣬����ĺ��ڴ������ʵ�ְ����ṩ�����ӳٷ�����ڴ�������1��
        //          VK_MEMORY_PROPERTY_PROTECTED_BIT����ʾ�����ڴ�����ֻ�����豸�����ڴ棬�������ܱ����Ķ��в��������ڴ档�ڴ����Ͳ���ͬʱ����VK_MEMORY_PROPERTY_PROTECTED_BIT���κ�VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT��VK_MEMORY_PROPERTY_HOST_COHERENT_BIT��VK_MEMORY_PROPERTY_HOST_CACHED_BIT1��
        //          VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD����ʾ�����ڴ����ͷ�����ڴ���豸�������Զ����úͿɼ��ġ�����һ����AMD�ṩ����չ2��
        //          VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD����ʾ�����ڴ����ͷ�����ڴ����豸��û�л��档δ������豸�ڴ������豸һ�µġ���Ҳ��һ����AMD�ṩ����չ2��
        //          VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV����ʾ�ⲿ�豸����ֱ�ӷ��������ڴ����ͷ�����ڴ档����һ����NVIDIA�ṩ����չ3��
        //          VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM����ʾVkMemoryPropertyFlagBitsö������Чֵ�����������������һ�������ı�־λ���������ڴ�����ͷ�Χ��֤4��
        //          �ҵ�����������ϣ�
        //          1:VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        //          2:VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT&VK_MEMORY_PROPERTY_HOST_CACHED_BIT
        //          3:VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        //      heapIndex����ʾ���ڴ����Ͷ�Ӧ���ڴ�ѵ�����������С��memoryHeapCount�ֶΡ�
        //  memoryHeapCount����ʾ memoryHeaps �������ЧԪ�ظ������ҵ�����1����
        //  memoryHeaps��һ������ VK_MAX_MEMORY_HEAPS �� VkMemoryHeap �ṹ������飬��ʾ�������������ڴ���ڴ�ѡ�
        //      �����������ֶΣ�
        //      size��һ�� VkDeviceSize ֵ����ʾ���ڴ�ѵ��ֽڴ�С��
        //      flags��һ�� VkMemoryHeapFlags λ���룬��ʾ���ڴ�ѵ����ԣ������Ƿ����豸���صġ��Ƿ�֧�ֶ�ʵ���ȡ�
        //          VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ����ʾ���ڴ�Ѷ�Ӧ���豸�����ڴ棬��GPU����ֱ�ӷ��ʵ��Դ档
        //              �豸�����ڴ�����в�ͬ�����������ڴ棨�� CPU ����ֱ�ӷ��ʵ�ϵͳ�ڴ棩������������֧�ֵ��ڴ����ԡ�
        //          VK_MEMORY_HEAP_MULTI_INSTANCE_BIT ����ʾ��һ�������������豸���߼��豸�У����ڴ����ÿ�������豸ʵ���ĸ�����
        //              Ĭ������£����������ڴ�ѷ�����ڴ�ᱻ���Ƶ�ÿ�������豸ʵ�����ڴ���С����ֱ�־λͨ������֧�� SLI �� Crossfire �ȶ� GPU ����2��
        //          VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR ������ VK_KHR_device_group_creation ��չ�ṩ�ģ����ڼ��ݾɰ汾�� Vulkan API3��
        //          VK_MEMORY_HEAP_FLAG_BITS_MAX_ENUM ����ʾ VkMemoryHeapFlagBits ö������Чֵ�����������������һ�������ı�־λ���������ڴ�����ͷ�Χ��֤��
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties); //��ȡ�����豸���ڴ����ԣ�������洢��memProperties��

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            // typeFilter & (1 << i)��1 << i ��һ��λ������ʽ������ʾ������ 1 ���� i λ
            //      ���������������� typeFilter �ĵ� i λ�Ƿ�Ϊ 1�����Ϊ 1����ʾ�� i ���ڴ������ǿ��õģ����򲻿��á�  
            // (memProperties.memoryTypes[i].propertyFlags & properties) == properties��
            //      ����������������� i ���ڴ����͵������Ƿ������ properties ��ָ�����������ԡ�
            //      ���������ҪVK_MEMORY_PROPERTY_HOST_VISIBLE_BIT&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            //      ��һ���ڴ�������ϾͿ�������
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;   // ����������������ص�ǰ�ڴ����͵�����
            }
        }

        // ���û���ҵ����ʵ��ڴ����ͣ��׳�һ������ʱ�����쳣�������������Ϣ
        throw std::runtime_error("failed to find suitable memory type!");
    }


    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;

        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)swapChainExtent.width;
            viewport.height = (float)swapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            // ����һ�� VkBuffer ���͵����� vertexBuffers������ʼ��Ϊ���� vertexBuffer ����ĵ�Ԫ�����飬��ʾҪ�󶨵Ķ��㻺�����б�
            VkBuffer vertexBuffers[] = { vertexBuffer };
            // ����һ�� VkDeviceSize ���͵����� offsets������ʼ��Ϊ���� 0 �ĵ�Ԫ�����飬��ʾÿ�����㻺�������ֽ�ƫ�����б�
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            // vkCmdBindVertexBuffers �����������󶨶��㻺������һ����������ģ��Ա��ں����Ļ���������ʹ�á����������²�����
            //  commandBuffer��ָ��Ҫ��¼�����������������
            //  firstBinding��ָ����һ����������󶨵�����������״̬�ᱻ��������¡�
            //  bindingCount��ָ��Ҫ����״̬�Ķ�������󶨵�������
            //  pBuffers��ָ��һ��������������������顣
            //  pOffsets��ָ��һ������������ƫ���������顣
            // �ú����������ǽ� pBuffers �� pOffsets �е�Ԫ�� i �滻Ϊ��ǰ״̬�Ķ�������� firstBinding + i������ i ��[0, bindingCount) ��Χ�ڡ�
            // ��������󶨻����Ϊ�ӻ����� pBuffers[i] �Ŀ�ʼ������ƫ���� pOffsets[i] ��ʼ��
            vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}