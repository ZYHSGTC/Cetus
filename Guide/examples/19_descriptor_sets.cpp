#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
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

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
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
    VkDescriptorSetLayout descriptorSetLayout;//
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    //
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    //
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;

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
        createDescriptorSetLayout();//
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();//
        createDescriptorPool();//
        createDescriptorSets();//
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

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {//
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);//

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);//

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
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

    // ���������֡�����������������֮��Ĺ�ϵ�����������ͼʾ����ʾ��(Set 0��3����������Set 1 ��4��������
    //    +---------------------+      +---------------------+
    //    | Descriptor Set 0   |       | Descriptor Set 1   |
    //    +---------------------+      +---------------------+
    //    | Binding 0          |       | Binding 0          |
    //    | Type: Uniform      |       | Type : Sampler     |
    //    | Count : 1          |       | Count : 1          |
    //    | Stage : Vertex     |       | Stage : Fragment   |
    //    +---------------------+      +---------------------+
    //    | Binding 1          |       | Binding 1          |
    //    | Type : Storage     |       | Type : Image       |
    //    | Count : 2          |       | Count : 3          |
    //    | Stage : Compute    |       | Stage : Fragment   |
    //    +---------------------+      +---------------------+
    //                     Descriptor Layouts x2
    //
    //    +---------------------+      +---------------------+
    //    | Descriptor Set 0   |       | Descriptor Set 1   |
    //    +---------------------+      +---------------------+
    //    | Binding 0          |       | Binding 0          |
    //    | Resource A         |       | Resource E         |
    //    +---------------------+      +---------------------+
    //    | Binding 1          |       | Binding 1          |
    //    | Resource B         | --+-- | Resource F         |
    //    +---------------------+  |   +---------------------+
    //                             |
    //                             v
    //                       + ---------- +
    //                       | Resource C |
    //                       + ---------- +
    //                       Descriptor Sets/Pool
    // **************************************************************************************
    //  �������ش�����������������������Ҫ����������������������������һЩ�������ļ��ϡ�
    //      ��仰û�д������������ز�һ������������Ҫ������������ֻ��һ����������������������
    //      �������صĴ�С��������Ҫ�ڴ���ʱָ��������������˾Ͳ����ٷ����µ�����������������ջ������������ء�
    //  �����������������ص���������������������������һ�����������ϡ�
    //      �������س�ʼ�����ֱ�ָ��ÿ������������������ָ����Щ���е��������ṹ�ɶ��ٸ��������������������Ĳ����͵��������������е�������
    //      ͬһ����������������������������Ӧͬһ���󶨵㣬��Ȼ��ɫ���ϻ���ҡ�
    //      ��仰Ҳû�д�����������������ֱ�ӳ�����ģ�����ͨ��vkAllocateDescriptorSets���������������з�������ġ�
    //      ���������ֶ�����ÿ�����������а�����Щ���ͺ����������������Լ���������ɫ���еİ󶨵㡣
    //  Ȼ�������������ڵ���vkUpdateDescriptorSets�������������е�һ���󶨵�(��Ӧһ��������(����))�ļ���ͬ�����������Ӧ������ͳһ����������������ʹ����ɫ���п���ʹ��ͳһ������������
    //      ��仰Ҳû�д�����vkUpdateDescriptorSets�����������Թ���ͳһ�������������Թ����������͵���Դ�������������ͼ����ͼ�ȡ�
    //      vkUpdateDescriptorSets����ͨ��VkWriteDescriptorSet�ṹ����ָ��Ҫ�����ĸ����������е��ĸ��󶨵㣬�Լ�Ҫ�����ĸ���Դ��������ݷ�Χ��
    //  ����������descriptorSetLayout������ͼ�ι������ʹ��ͳһ����������������ͼ����ͼ��������Ϣ��
    //      ÿ��������������Ӧ��һ�����������֣���ͬ�����������������������ж����
    // **************************************************************************************
    //  ����VkDescriptorSetLayout��������飬ָ��������ʹ�õ������������֡�
    //      ÿ�������������ֿ��԰�������������󶨵㣬���ڴ洢��ͬ���ͺ���������Դ��
    //      VkDescriptorSetLayout��������Ҫ����ϢVkDescriptorSetLayoutCreateInfo�ṹ��:
    //      sType��һ��VkStructureTypeֵ�����ڱ�ʶ����ṹ������ͣ�������VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO��
    //      pNext��һ��ָ�룬����ָ��һ����չ����ṹ��������ṹ�壬����ΪNULL��
    //      flags��һ��λ���룬����ָ���������������������Ϣ��Ŀǰû�ж����κ���Ч��ֵ��
    //      bindingCount��һ������������ָ��pBindings�����е�VkDescriptorSetLayoutBinding�ṹ���������
    //      pBindings��һ��ָ�룬����ָ��һ��VkDescriptorSetLayoutBinding�ṹ�����飬������ÿ���������󶨵����Ϣ��
    //          VkDescriptorSetLayoutBinding�ṹ�壺
    //          binding��һ������������ָ������󶨵�ı�ţ�������ɫ����ʹ�õİ󶨺����Ӧ��
    //          descriptorType��һ��VkDescriptorTypeö��ֵ������ָ������󶨵�����������ͣ��绺�塢ͼ�񡢲������ȡ�
    //              VkDescriptorType��һ��ö�����ͣ�����ָ�������������͡��������¼���ֵ3��
    //              VK_DESCRIPTOR_TYPE_SAMPLER�����ڲ�������Ĳ�����
    //              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER�����ڲ��������ͼ��Ͳ�����
    //              VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE�����ڲ��������ͼ��
    //              VK_DESCRIPTOR_TYPE_STORAGE_IMAGE�����ڶ�д�����ͼ��
    //              VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER�����ڷ��ʻ������е����أ�texel���Ļ�������ͼ
    //              VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER�����ڶ�д�������е����أ�texel���Ļ�������ͼ
    //              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER�����ڷ��ʻ������е�ͳһ���ݣ�uniform data���Ļ�����
    //              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER�����ڶ�д�������еĴ洢���ݣ�storage data���Ļ�����
    //              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC�����ڷ��ʻ������е�ͳһ���ݣ�uniform data���Ļ����������ǿ����ڻ���ʱ��̬�ظı�ƫ����
    //              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC�����ڶ�д�������еĴ洢���ݣ�storage data���Ļ����������ǿ����ڻ���ʱ��̬�ظı�ƫ����
    //              VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT�����ڷ�����ͨ�����븽����subpass input attachment����ͼ��
    //          descriptorCount��һ������������ָ������󶨵���������������������1�����ʾ����󶨵���һ�����飬��������ɫ����ʹ���±������ʡ�
    //              descriptorCount����һ��ʾһ���󶨵��Ӧ�������������ͨ������ʵ����������������ݽṹ��
    //              ����ɫ����ʹ��ʱ����Ҫ��layout���η���ָ������Ĵ�С�������±���ʶ�Ӧ��Ԫ�ء����磺
    //              �ڶ�����ɫ���ж���һ��ͳһ����������
    //              layout(binding = 0) uniform UniformBufferObject {
    //                  mat4 model;
    //                  mat4 view;
    //                  mat4 proj;
    //              } ubos[3];
    //              void main() {
    //                  gl_Position = ubos[1].proj * ubos[1].view * ubos[1].model * vec4(aPos, 1.0);
    //              }
    //          stageFlags��һ��λ���룬����ָ������󶨵���Ա���Щ��ɫ���׶η��ʣ��綥�㡢Ƭ�Ρ�����ȡ�
    //              stageFlags��һ��λ�����ͣ�����ָ������������Щ��ɫ���׶ο��á��������¼���ֵ3��
    //              VK_SHADER_STAGE_VERTEX_BIT��������ɫ���׶�
    //              VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT������ϸ�ֿ�����ɫ���׶�
    //              VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT������ϸ��������ɫ���׶�
    //              VK_SHADER_STAGE_GEOMETRY_BIT��������ɫ���׶�
    //              VK_SHADER_STAGE_FRAGMENT_BIT��Ƭ����ɫ���׶�
    //              VK_SHADER_STAGE_COMPUTE_BIT��������ɫ���׶�
    //              VK_SHADER_STAGE_ALL_GRAPHICS������ͼ�ι�����ص���ɫ���׶�
    //              VK_SHADER_STAGE_ALL��������ɫ���׶�
    //          pImmutableSamplers��һ��ָ�룬����ָ������󶨵�Ĳ��ɱ���������飬
    //              �������󶨵�������������ǲ������������ͼ��������������ʹ������ֶ����̶����������󣬶�����Ҫ�����������и��¡�
    // ���������������ֵĺ���
    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};                    // ����һ�������������ְ󶨽ṹ��
        uboLayoutBinding.binding = 0;                                       // ���ð󶨵�Ϊ0
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;// ��������������Ϊͳһ������
        uboLayoutBinding.descriptorCount = 1;                               // ��������������Ϊ1
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;           // ������ɫ���׶α�־Ϊ����׶�
        uboLayoutBinding.pImmutableSamplers = nullptr;                      // ���ò��ɱ������Ϊ��ָ��

        VkDescriptorSetLayoutCreateInfo layoutInfo{};                       // ����һ�������������ִ�����Ϣ�ṹ��
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;                                        // ���ð�����Ϊ1
        layoutInfo.pBindings = &uboLayoutBinding;                           // ���ð�ָ��Ϊ���洴���İ󶨽ṹ��

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            // ����Vulkan API���������������������֣��������ʧ�ܣ��׳�����ʱ�����쳣
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }


    // ��ע��
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

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
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
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

        // VkPipelineLayoutCreateInfo�ṹ�����������ù��߲�����ص����õ�һ���ṹ�壬������ָ��������ʹ�õ������������ֺ����ͳ�����Χ��
        //  sType��һ��VkStructureTypeֵ�����ڱ�ʶ����ṹ������ͣ�������VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO��
        //  pNext��һ��ָ�룬����ָ��һ����չ����ṹ��������ṹ�壬����ΪNULL��
        //  flags��һ��λ���룬����ָ������Ĺ��߲�����Ϣ��Ŀǰû�ж����κ���Ч��ֵ������Ϊ0��
        //  setLayoutCount��һ������������ָ��pSetLayouts�����е�VkDescriptorSetLayoutֵ��������
        //  pSetLayouts��һ��ָ�룬����ָ��һ��VkDescriptorSetLayout��������飬ָ��������ʹ�õ������������֡�
        //      ÿ�������������ֿ��԰�������������󶨵㣬���ڴ洢��ͬ���ͺ���������Դ��
        //  pushConstantRangeCount��һ������������ָ��VkPushConstantRange�ṹ�������������û���趨��
        //  pPushConstantRanges��һ��ָ�룬����ָ��һ��VkPushConstantRange�ṹ�����飬ָ��������ʹ�õ����ͳ�����Χ��
        //      ÿ�����ͳ�����Χ���Զ���һ���ڴ�ռ䣬���ڴ洢һЩ�������ݣ�����������ʱͨ��vkCmdPushConstants���������¡�
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

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
            throw std::runtime_error("failed to create graphics command pool!");
        }
    }

    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createUniformBuffers() {                               // ����ͳһ�������ĺ���
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);  // ��ȡͳһ����������Ĵ�С
        // ���ֻ��һ��ͳһ����������ôÿһ֡����Ҫ�ȴ���һ֡ʹ�øû�����������(��Ⱦ����-->)ִ����ϣ����ܸ��¸�ͬһ��������
        // Ҳ���Ǳ���˸�������һ�����ٶȣ�
        // ˫֡����(���½��棬˫����)(��Ⱦ����-->)(���»���ʵ��������->)
        //  ->-->t      ->-->t...
        //        ->-->t      ->-->t...
        // Ϊ�˱������������ÿ����������Ӧһ��֡����������ʵ��˫���塣
        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);      // ͳһ���������ڴ�����ָ��

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {     // ����ÿһ֡
            // ���������ɼ�������ͬ��һ�µ�ͬһ������
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
            // ���ͬһ��������ָ��uniformBuffersMapped[i]
            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }
    // ����һ���������أ�descriptor pool�������ڷ�������������descriptor set��
    // �������أ�descriptor pool����һ�����ڷ���͹�������������descriptor set���Ķ���
    // ����������һ�����ڴ洢һ����������descriptor���Ķ�����������һ�����ڱ�ʾ��������buffer����ͼ��image������Դ�Ķ���
    void createDescriptorPool() {   
        // VkDescriptorPoolSize�ṹ������ָ���������أ�descriptor pool����ÿ�����͵������������������������³�Ա��
        //  type��һ��VkDescriptorTypeö��ֵ����ʾ�����������ͣ�
        //      VkDescriptorType��һ��ö�����ͣ�����ָ�������������͡��������¼���ֵ3��
        //      VK_DESCRIPTOR_TYPE_SAMPLER�����ڲ�������Ĳ�����
        //      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER�����ڲ��������ͼ��Ͳ�����
        //      VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE�����ڲ��������ͼ��
        //      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE�����ڶ�д�����ͼ��
        //      VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER�����ڷ��ʻ������е����أ�texel���Ļ�������ͼ
        //      VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER�����ڶ�д�������е����أ�texel���Ļ�������ͼ
        //      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER�����ڷ��ʻ������е�ͳһ���ݣ�uniform data���Ļ�����
        //      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER�����ڶ�д�������еĴ洢���ݣ�storage data���Ļ�����
        //      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC�����ڷ��ʻ������е�ͳһ���ݣ�uniform data���Ļ����������ǿ����ڻ���ʱ��̬�ظı�ƫ����
        //      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC�����ڶ�д�������еĴ洢���ݣ�storage data���Ļ����������ǿ����ڻ���ʱ��̬�ظı�ƫ����
        //      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT�����ڷ�����ͨ�����븽����subpass input attachment����ͼ��
        //  descriptorCount��һ��uint32_tֵ����ʾ�����͵���������������
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        // VkDescriptorPoolCreateInfo�ṹ������ָ���������أ�descriptor pool��������2�����������³�Ա��
        //  sType������ΪVK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO��
        //  pNext��һ��void* ָ�룬��ʾָ����չ�ṹ���ָ�룬����ΪNULL��
        //  flags��һ��VkDescriptorPoolCreateFlagsλ���룬��ʾ�������صĴ�����־��
        //      ��Щֵ��ָVkDescriptorPoolCreateFlagsλ���룬��ʾ�������أ�descriptor pool���Ĵ�����־1�����������º�������ã�
        //      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT��
        //          ��ʾ��������������е����ͷ�����������descriptor set����������ֻ�����������������ء�
        //      VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT��
        //          ��ʾ�����ڰ�����������������������е���������descriptor����������ֻ���ڰ�ǰ���¡�
        //      VK_DESCRIPTOR_POOL_CREATE_HOST_ONLY_BIT_EXT��
        //          ��ʾ���������е���������ֻ������������host���ɼ����ڴ����ͣ���CPU����ֱ�ӷ��ʻ�ӳ�䣨map�����ڴ档
        //      VK_DESCRIPTOR_POOL_CREATE_ALLOW_OVERALLOCATION_SETS_BIT_NV��
        //          ��ʾ��������������з��䳬��maxSets�������������������ǲ���֤����ɹ���
        //      VK_DESCRIPTOR_POOL_CREATE_ALLOW_OVERALLOCATION_POOLS_BIT_NV��
        //          ��ʾ��������������з��䳬��pPoolSizesָ�������������������ǲ���֤����ɹ���
        //  maxSets��һ��uint32_tֵ����ʾ���������п��Է�����������������descriptor set��������
        //  poolSizeCount��һ��uint32_tֵ����ʾ�������ش�С��descriptor pool size���ṹ���������
        //  pPoolSizes��һ��VkDescriptorPoolSize* ָ�룬��ʾָ���������ش�С�ṹ�������ָ�롣
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);// �������������п��Է�������������������ΪMAX_FRAMES_IN_FLIGHT
        poolInfo.poolSizeCount = 1;     // �����������ش�С�ṹ�������Ϊ1����ֻ��һ�����͵�������
        poolInfo.pPoolSizes = &poolSize;// �����������ش�С�ṹ���ָ��

        // ����vkCreateDescriptorPool�����������������أ���������洢��descriptorPool������
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    
    void createDescriptorSets() {       // ��������������descriptor set�������ڰ�ͳһ��������uniform buffer������ɫ����
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        // ��ʹ��MAX_FRAMES_IN_FLIGHT��Ϊ�����ĳ�ʼ��С����ʹ��descriptorSetLayout��Ϊÿ��Ԫ�صĳ�ʼֵ��
        // �����Ϳ��Դ���һ������MAX_FRAMES_IN_FLIGHT����ͬ�������������֣�descriptor set layout�������������ڷ�������������descriptor set����

        // VkDescriptorSetAllocateInfo�ṹ������ָ����descriptor pool����descriptor set�Ĳ��������������³�Ա��
        //  sType��һ��VkStructureTypeö��ֵ����ʾ�ṹ������ͣ�����ΪVK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO��
        //  pNext��һ��void* ָ�룬��ʾָ����չ�ṹ���ָ�룬����ΪNULL��
        //  descriptorPool��һ��VkDescriptorPool�������ʾҪ���з��������������������ء�
        //  descriptorSetCount��һ��uint32_tֵ����ʾҪ�������������������
        //  pSetLayouts��һ��VkDescriptorSetLayout* ָ�룬��ʾָ��descriptor set layout�����ָ�룬ÿ��Ԫ��ָ���˶�Ӧ���������Ĳ��֡�
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;          
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); 
        allocInfo.pSetLayouts = layouts.data();            
        
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);        // ����descriptorSets�����Ĵ�СΪMAX_FRAMES_IN_FLIGHT�����ڴ洢ÿһ֡��Ӧ�������������
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        // ����vkAllocateDescriptorSets�����������������з���������������������洢��descriptorSets�����С��������ʧ�ܣ��׳�����ʱ�쳣
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) { // ����ÿһ֡��Ӧ������i
            // ����һ����������������Ϣ��descriptor buffer info���ṹ�壬����ָ��Ҫ�󶨵����������еĻ�������Դ
            // VkDescriptorBufferInfo�ṹ������ָ��Ҫ�󶨵�descriptor set�еĻ�����buffer��Դ�����������³�Ա��
            //  buffer��һ��VkBuffer�������ʾҪ�󶨵Ļ�������
            //  offset��һ��VkDeviceSizeֵ����ʾҪ�󶨵Ļ�������ƫ���������ӻ���������ʼλ�ÿ�ʼ�󶨵��ֽ�λ�á�
            //  range��һ��VkDeviceSizeֵ����ʾҪ�󶨵Ļ������ķ�Χ�����󶨶����ֽڵĻ��������ݡ��������ΪVK_WHOLE_SIZE�����ʾ�󶨴�ƫ������ʼ��������ĩβ���������ݡ�
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];          // ����Ҫ�󶨵Ļ��������ΪuniformBuffers�����еĵ�i��Ԫ��
            bufferInfo.offset = 0;                          // ����Ҫ�󶨵Ļ�������ƫ����Ϊ0�����ӻ���������ʼλ�ÿ�ʼ��
            bufferInfo.range = sizeof(UniformBufferObject); // ����Ҫ�󶨵Ļ������ķ�ΧΪUniformBufferObject�ṹ��Ĵ�С����������������

            // VkWriteDescriptorSet�ṹ������ָ��������������descriptor set��ĳ��������descriptor�Ĳ��������������³�Ա��
            //  sType��һ��VkStructureTypeö��ֵ����ʾ�ṹ������ͣ�����ΪVK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET��
            //  pNext��һ��void* ָ�룬��ʾָ����չ�ṹ���ָ�룬����ΪNULL��
            //  dstSet��һ��VkDescriptorSet�������ʾҪ���µ�Ŀ������������
            //  dstBinding��һ��uint32_tֵ����ʾҪ���µ�Ŀ�������������������еİ󶨵㣨binding point����������ɫ����shader����layout(binding = ��)��Ӧ��ֵ��
            //  dstArrayElement��һ��uint32_tֵ����ʾҪ���µ�Ŀ���������������������е���ʼλ�á����dstBinding��Ӧ��������������VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK�����ʾ��ʼ�ֽ�ƫ������
            //  descriptorCount��һ��uint32_tֵ����ʾҪ���¶��ٸ������������Ԫ�ء�descriptorCount�ж��٣�pBufferInfo������ж��ٸ�Ԫ��
            //      ���dstBinding��Ӧ��������������VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK�����ʾ���¶����ֽڵ����ݡ�
            //  descriptorType��һ��VkDescriptorTypeö��ֵ����ʾҪ���µ������������ͣ���ͳһ��������uniform buffer������������sampler����ͼ��image���ȡ�
            //  pImageInfo��һ��VkDescriptorImageInfo* ָ�룬��ʾָ��ͼ���������������Ϣ�ṹ�������ָ�룬����ΪNULL��
            //      ֻ�е�descriptorType��VK_DESCRIPTOR_TYPE_SAMPLER��VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER��VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE��VK_DESCRIPTOR_TYPE_STORAGE_IMAGE��VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENTʱ����ָ�����Ч��
            //  pBufferInfo��һ��VkDescriptorBufferInfo* ָ�룬��ʾָ�򻺳����������������Ϣ�ṹ�������ָ�룬����ΪNULL��
            //      ֻ�е�descriptorType��VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER��VK_DESCRIPTOR_TYPE_STORAGE_BUFFER��VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC��VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMICʱ����ָ�����Ч��
            //  pTexelBufferView��һ��VkBufferView* ָ�룬��ʾָ�򻺳�����ͼ��buffer view����������ָ�룬����ΪNULL��
            //      ֻ�е�descriptorType��VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER��VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFERʱ����ָ�����Ч��
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];                         // ����Ҫ���µ������������ΪdescriptorSets�����еĵ�i��Ԫ��
            descriptorWrite.dstBinding = 0;                                     // ����Ҫ���µ������������������еİ󶨵�Ϊ0��������ɫ����layout(binding = 0)��Ӧ
            descriptorWrite.dstArrayElement = 0;                                // ����Ҫ���µ��������������������е���ʼλ��Ϊ0����ֻ����һ��������
            descriptorWrite.descriptorCount = 1;                                // ����Ҫ���µ�������������Ϊ1����ֻ����һ��������
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // ����Ҫ���µ�������������Ϊͳһ��������uniform buffer��
            descriptorWrite.pBufferInfo = &bufferInfo;                          // ����Ҫ���µ���������Ӧ�Ļ�������Ϣ�ṹ���ָ��
            
            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);    // ����vkUpdateDescriptorSets�������������������е�������������������Դ�󶨵���������
        }
    }


    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

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

            VkBuffer vertexBuffers[] = { vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
            // ����vkCmdBindDescriptorSets�Ĺ���
            //  vkCmdBindDescriptorSets�������ڽ�һ����������������descriptor set���󶨵�һ�����������command buffer����
            // ����vkCmdBindDescriptorSets�Ĳ���
            //  commandBuffer��һ��VkCommandBuffer�������ʾҪ���������������������
            //  pipelineBindPoint��һ��VkPipelineBindPointö��ֵ����ʾҪʹ�����������Ĺ��ߣ�pipeline�����͡�
            //      ���������ͣ�VK_PIPELINE_BIND_POINT_GRAPHICS��VK_PIPELINE_BIND_POINT_COMPUTE���ֱ��ʾͼ�ι��ߺͼ�����ߡ�
            //  layout��һ��VkPipelineLayout�������ʾ����ָ���󶨵㣨binding point���Ĺ��߲��֣�pipeline layout����
            //      ���߲�����һ�����ڶ��������������֣�descriptor set layout�������ͳ�����push constant����Χ�Ķ���
            //  firstSet��һ��uint32_tֵ����ʾҪ�󶨵ĵ�һ�����������ڹ��߲����еļ��Ϻţ�set number�������Ϻ���һ���������ֲ�ͬ���������ı�ţ���0��ʼ��
            //  descriptorSetCount��һ��uint32_tֵ����ʾҪ�󶨵���������������
            //  pDescriptorSets��һ��VkDescriptorSet* ָ�룬��ʾָ��Ҫ�󶨵�����������������ָ�롣
            //  dynamicOffsetCount��һ��uint32_tֵ����ʾ��̬ƫ������dynamic offset�������е�Ԫ����������̬ƫ������һ�������޸Ķ�̬��������dynamic buffer����ʱ��ƫ������ֵ��
            //  pDynamicOffsets��һ��uint32_t* ָ�룬��ʾָ��̬ƫ���������ָ�롣
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

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

    
    void updateUniformBuffer(uint32_t currentImage) {                       // ����һ������ͳһ��������uniform buffer���ĺ���������Ϊ��ǰͼ���������image index��
        // chrono�Ļ�������
        //  chrono��һ��C++ͷ�ļ����ṩ��һϵ�е����ͺͺ��������ڴ���ʱ����صĲ���1������C++��׼ģ��⣨STL����һ���֣�������C++11���Ժ�İ汾�С�
        //  chrono�ṩ��������Ҫ��ʱ�ӣ�clock�����ͣ�system_clock, steady_clock, �� high_resolution_clock�����Ƿֱ����ڱ�ʾϵͳʱ�䡢�ȶ�ʱ��͸߾���ʱ�䡣
        //  chrono���ṩ��������Ҫ��ʱ����duration��time_point��duration���ڱ�ʾʱ��������һ���ӡ���Сʱ��ʮ���롣
        //      time_point���ڱ�ʾ�ض���ʱ��㣬�����ʼ���е�ʱ�䡢��ǰ֡��ʱ���ĳ���¼�������ʱ�䡣
        //  chrono���ṩ��һЩ���������ͺͺ��������ڽ���ʱ�䵥λ��ת����ʱ��֮���ת�������ں�ʱ��ĸ�ʽ���Ȳ�����
        /// ����chronoͷ�ļ�
        /*#include <chrono>
        // ����iostreamͷ�ļ����������
        #include <iostream>

        // ����һ�����������ڼ���쳲��������У�Fibonacci sequence���е�n���ֵ
        // ����Ϊn������ֵΪ��n���ֵ
        int fibonacci(int n) {
            // ���nΪ0��1��ֱ�ӷ���n
            if (n == 0 || n == 1) {
                return n;
            }
            // ���򣬵ݹ�ص���fibonacci����������ǰ����֮��
            else {
                return fibonacci(n - 1) + fibonacci(n - 2);
            }
        }

        // ����������
        int main() {
            // ����һ������n����ʾҪ����쳲����������еڼ���
            int n = 40;
            // ����һ������result�����ڴ洢������
            int result = 0;

            // ʹ��system_clock���ȡ��ǰϵͳʱ�䣬���洢��start������
            // system_clock���ʾϵͳʱ�䣬����������ʱ�以��ת��
            auto start = std::chrono::system_clock::now();

            // ����fibonacci�����������n���ֵ������ֵ��result����
            result = fibonacci(n);

            // ʹ��system_clock���ٴλ�ȡ��ǰϵͳʱ�䣬���洢��end������
            auto end = std::chrono::system_clock::now();

            // ʹ��duration_cast��������end��start֮���ʱ���ת��Ϊ���룬���洢��elapsed������
            // duration_cast�������Խ���ͬ���ȵ�duration����ת��Ϊָ�����ȵ�duration����
            // duration�����ʾʱ�����������в�ͬ�ĵ�λ�;���
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            // ʹ��cout��������������ͺ�ʱ
            std::cout << "Fibonacci(" << n << ") = " << result << "\n";
            std::cout << "Time elapsed: " << elapsed.count() << " ms\n";

            // ����0����ʾ������������
            return 0;
        }*/
        // ��һ��ֻ���ڵ�һ�ε����������ʱִ��һ�Σ�Ȼ��startTime�ͻᱣ�ֲ��䡣
        //  ������Ϊstatic�ؼ��ֵ�����֮һ���ñ���ֻ��ʼ��һ�Σ�������ÿ�ν��뺯��ʱ�����³�ʼ����
        static auto startTime = std::chrono::high_resolution_clock::now();  // ����һ����̬����startTime�����ڴ洢����ʼ���е�ʱ���
        auto currentTime = std::chrono::high_resolution_clock::now();       // ����һ������currentTime�����ڴ洢��ǰ֡��ʱ���
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();               // ����һ������time�����ڼ���ӳ���ʼ���е���ǰ֡��ʱ��������λΪ��

        
        UniformBufferObject ubo{};                                          // ����һ��UniformBufferObject�ṹ�����ubo�����ڴ洢Ҫ���ݸ���ɫ����shader�������ݣ���任����transformation matrix����
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));          // ʹ��glm���е�lookAt�������������λ��Ϊ(2, 2, 2)��Ŀ��λ��Ϊ(0, 0, 0)���Ϸ���Ϊz��������
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);   // ʹ��glm���е�perspective�����������ӳ��ǣ�field of view��Ϊ45�ȣ���߱ȣ�aspect ratio��Ϊ��������ȳ��Ը߶ȣ���ƽ�棨near plane��Ϊ0.1��Զƽ�棨far plane��Ϊ10
        ubo.proj[1][1] *= -1;                                               // ����Vulkan��OpenGLʹ�ò�ͬ�Ĳü��ռ䣨clip space������Ҫ��ͶӰ���������������ͶӰ����ڶ��еڶ��е�Ԫ�س���-1������תy�᷽��
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));      // uniformBuffersMapped�����е�ÿ��Ԫ�ض���һ��ָ��ͳһ������ӳ���ڴ棨uniform buffer mapped memory����ָ��
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
        //
        updateUniformBuffer(currentFrame);

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