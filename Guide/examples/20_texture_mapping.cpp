#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
//

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
    glm::vec2 texCoord;//

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        //
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
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
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    //
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

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
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createTextureImage();//
        createTextureImageView();
        createTextureSampler();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
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

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);

        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

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
    // ��ע��
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
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        // ���ڴ�������������ʱָ���Ƿ����ø������Թ��ˡ�
        // �������Թ�����һ��������˼����������ڲ�ͬ������ʹ�ò�ͬ�������𣬴Ӷ���������������ϸ��

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
            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
        }//
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
    // ��ע��
    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER�����ڲ��������ͼ��Ͳ�����
        // λ��Ƭ����ɫ������
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
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

    void createTextureImage() {                                         
        // ����һ�����������ڴ�������ͼ�񣬽ṹ��createindexbuffer���ơ�����һ����ת��������Ȼ���ٰ�����ͼ���Ƶ�gpu��
        // �����������������ڴ洢����ͼ��Ŀ�ȣ��߶Ⱥ�ͨ������STBI_rgb_alpha��ʾ��ͼƬת��ΪRGBA��ʽ��ÿ��ͨ��ռ��һ���ֽ�
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("examples/Assets/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;              // ����ͼƬ�����ֽ��������ڿ�ȳ��Ը߶ȳ���4����Ϊÿ������ռ��4���ֽڣ�
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;                                                     // ����һ��void���͵�ָ�룬����ӳ���豸�ڴ浽�����ڴ�
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);                                        // ����stb_image�⺯�����ͷ�pixelsָ����ڴ�ռ�

        //VkImage textureImage;
        //VkDeviceMemory textureImageMemory;
        //  ���������������ֱ��ʾ����ͼ����������ͼ������Ӧ���豸�ڴ����
        //  ������������ȫ�ֵģ���Ϊ����ĺ������õ�����
        
        // texWidth��texHeight�ֱ��ʾͼ��Ŀ�Ⱥ͸߶�
        // VK_FORMAT_R8G8B8A8_SRGB��ʾͼ��ĸ�ʽ��RGBA��ÿ��ͨ��ռ��һ���ֽڣ�ʹ��sRGB��ɫ�ռ�
        // VK_IMAGE_TILING_OPTIMAL��ʾͼ������з�ʽ����ѵģ�������������ѡ������ʵķ�ʽ
        // VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT��ʾͼ�����;����Ϊ����Ŀ��Ͳ���Դ
        // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT��ʾ�豸�ڴ���������豸���صģ���ֻ����GPU���ʣ���������������
        // textureImage��textureImageMemory�ֱ���մ�����ͼ�������豸�ڴ����
        createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

        // ����һ���Զ���ĺ�����ת��ͼ��Ϊ����Ŀ����ѵĲ���
        // textureImage��Ҫת�����ֵ�ͼ�����
        // VK_FORMAT_R8G8B8A8_SRGB��ʾͼ��ĸ�ʽ
        // VK_IMAGE_LAYOUT_UNDEFINED��ʾͼ��ĳ�ʼ������δ����ģ���������ͼ�������
        // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL��ʾͼ���Ŀ�겼���Ǵ���Ŀ����ѵģ����ʺ���Ϊ���������Ŀ��
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        // ����һ���Զ���ĺ��������������е����ݸ��Ƶ�ͼ����
        // stagingBuffer����Ϊ����Դ�Ļ���������
        // textureImage����Ϊ����Ŀ���ͼ�����
        // static_cast<uint32_t>(texWidth)��static_cast<uint32_t>(texHeight)�ֱ��ʾͼ��Ŀ�Ⱥ͸߶ȣ�ʹ��static_cast��int����ת��Ϊuint32_t����
        copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        // �ٴε���һ���Զ���ĺ�����ת��ͼ��Ĳ���
        // textureImage��Ҫת�����ֵ�ͼ�����
        // VK_FORMAT_R8G8B8A8_SRGB��ʾͼ��ĸ�ʽ
        // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL��ʾͼ��ĳ�ʼ�����Ǵ���Ŀ����ѵ�
        // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL��ʾͼ���Ŀ�겼������ɫ��ֻ����ѵģ����ʺ���Ϊ��ɫ��������Դ
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    // Image��Vulkan�б�ʾͼ��Ķ��������԰�����ɫ����ȡ�ģ������ݣ�Ҳ�����в�ͬ��ά�ȡ���ʽ��ƽ�̷�ʽ��ʹ�÷�ʽ�Ͳ��ֵ����ԡ�
    //      Image����ֱ�ӱ���Ⱦ���߷��ʣ���ֻ��һ���ڴ��������ڴ洢ͼ�����ݡ�
    // ImageView��Vulkan�б�ʾͼ����ͼ�Ķ������Ƕ�Image��һ�����û��Ӽ�������������η���Image�е����ݡ�
    //      ImageView����ָ��Image��һ��������Դ������mipmap���������㣬Ҳ����ָ��Image�ķ���͸�ʽ��������ɫ����ȷ��棬�Լ���������������ʽ��
    //      ImageView����������������ӳ���ͨ����ϵȹ���
    // һ��ͼ����԰����������Դ��������mipmap������߶������㡣����һ��ͼ����ͼֻ������һ������Դ��Χ������һ��mipmap�������һ�������

    // ����Ҫ����ɫ���з�����������ʱ����Ҫ�õ�ͼ����ͼ����Ϊ��ɫ������ֱ�ӷ���ͼ����󣬶���ͨ���󶨵���������֮������ͼ����ͼ��
    // ����Ҫ�����������ִ��һЩ����ʱ����Ҫ�õ�ͼ�񡣱��縴�ơ�ת��������Ȳ�������Ҫָ��������Ŀ���Դͼ��

    void createTextureImageView() {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
    }

    // ����һ�����ڴ�������������ĺ���
    void createTextureSampler() {
        // VkPhysicalDeviceProperties��һ���ṹ�壬�������������豸�����ԡ��������³�Ա1��
        //  apiVersion����ʾ�豸֧�ֵ�Vulkan�汾�����뷽ʽ�μ���
        //  driverVersion����ʾ��������ĳ���ָ���汾��
        //  vendorID����ʾ�����豸���̵�Ψһ��ʶ����
        //  deviceID����ʾ�����豸��ͬһ�����µ�Ψһ��ʶ����
        //  deviceType����ʾ�����豸�����ͣ���һ��VkPhysicalDeviceTypeö��ֵ��
        //  deviceName����ʾ�����豸�����ƣ���һ������ΪVK_MAX_PHYSICAL_DEVICE_NAME_SIZE��char���飬����һ���Կ��ַ���β��UTF - 8�ַ�����
        //  pipelineCacheUUID����ʾ�����豸��ͨ��Ψһ��ʶ������һ������ΪVK_UUID_SIZE��uint8_t���顣
        //  limits����ʾ�����豸���ض����ƣ���һ��VkPhysicalDeviceLimits�ṹ�塣
        //      VkPhysicalDeviceLimits�ṹ���������������豸���ض����ƣ����кܶ��Ա����ֻ�о�һЩ���õ�1��
        //          maxImageDimension1D����ʾһάͼ�������ȡ�
        //          maxImageDimension2D����ʾ��άͼ�������Ⱥ͸߶ȡ�
        //          maxImageDimension3D����ʾ��άͼ�������ȡ��߶Ⱥ���ȡ�
        //          maxImageDimensionCube����ʾ������ͼ�������Ⱥ͸߶ȡ�
        //          maxImageArrayLayers����ʾͼ���������������
        //          maxTexelBufferElements����ʾ���ػ����������Ԫ������
        //          maxUniformBufferRange����ʾͳһ������������ֽڷ�Χ��
        //          maxStorageBufferRange����ʾ�洢������������ֽڷ�Χ��
        //          maxPushConstantsSize����ʾ���ͳ���������ֽڴ�С��
        //          maxMemoryAllocationCount����ʾһ���߼��豸����ͬʱ���������ڴ�����
        //          maxSamplerAllocationCount����ʾһ���߼��豸����ͬʱ�����������������
        //          bufferImageGranularity����ʾ��������ͼ��֮����ڴ�������ȡ�
        //          sparseAddressSpaceSize����ʾϡ����Դ�ĵ�ַ�ռ��С��
        //          maxBoundDescriptorSets����ʾһ�����߲��ֿ��԰󶨵����������������
        //          maxPerStageDescriptorSamplers����ʾһ����ɫ���׶ο��Է��ʵ�������������
        //          maxPerStageDescriptorUniformBuffers����ʾһ����ɫ���׶ο��Է��ʵ����ͳһ����������
        //          maxPerStageDescriptorStorageBuffers����ʾһ����ɫ���׶ο��Է��ʵ����洢����������
        //          maxPerStageDescriptorSampledImages����ʾһ����ɫ���׶ο��Է��ʵ�������ͼ������
        //          maxPerStageDescriptorStorageImages����ʾһ����ɫ���׶ο��Է��ʵ����洢ͼ������
        //          maxPerStageDescriptorInputAttachments����ʾһ����ɫ���׶ο��Է��ʵ�������븽������
        //          maxPerStageResources����ʾһ����ɫ���׶ο��Է��ʵ������Դ������
        //          maxDescriptorSetSamplers����ʾһ�������������԰�����������������
        //          maxDescriptorSetUniformBuffers����ʾһ�������������԰��������ͳһ����������
        //          maxDescriptorSetUniformBuffersDynamic����ʾһ�������������԰��������̬ͳһ����������
        //          maxDescriptorSetStorageBuffers����ʾһ�������������԰��������洢����������
        //          maxDescriptorSetStorageBuffersDynamic����ʾһ�������������԰��������̬�洢����������
        //          maxDescriptorSetSampledImages����ʾһ�������������԰�����������ͼ������
        //          maxDescriptorSetStorageImages����ʾһ�������������԰��������洢ͼ������
        //          maxDescriptorSetInputAttachments����ʾһ�������������԰�����������븽������
        //          maxVertexInputAttributes����ʾ�����������Ե����������
        //          maxVertexInputBindings����ʾ��������󶨵����������
        //          maxVertexInputAttributeOffset����ʾ������������ƫ���������ֵ��
        //          maxVertexInputBindingStride����ʾ��������󶨲��������ֵ��
        //          maxVertexOutputComponents����ʾ������ɫ�������������������
        //  sparseProperties����ʾ�����豸��ϡ��������ԣ���һ��VkPhysicalDeviceSparseProperties�ṹ�塣
        VkPhysicalDeviceProperties properties{};// ����һ�����ڴ洢�����豸���ԵĽṹ��
        vkGetPhysicalDeviceProperties(physicalDevice, &properties); // ��ȡ�����豸�����ԣ��������������ԣ�anisotropy��ֵ

        // �ṹ��VkSamplerCreateInfo��������������������Ĵ�����Ϣ�ģ��������³�Ա����1��
        //  sType����ʾ�ṹ������ͣ�������VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO��
        //  pNext����ʾָ����չ�ṹ���ָ�룬������NULL��
        //  flags����ʾ�������Ķ��������λ���룬Ŀǰû�ж����κα�־λ�����Ա�����0��
        //  magFilter����ʾ�Ŵ���˷�ʽ��������VK_FILTER_NEAREST��VK_FILTER_LINEAR����ʾ�������Ŵ�ʱ��ʹ����������ػ��������صļ�Ȩƽ��ֵ��
        //  minFilter����ʾ��С���˷�ʽ��������VK_FILTER_NEAREST��VK_FILTER_LINEAR����ʾ��������Сʱ��ʹ����������ػ��������صļ�Ȩƽ��ֵ��
        //      VkFilter��һ��ö�����ͣ�����ָ���������ʱ�Ĺ��˷�ʽ���������ֿ��ܵ�ֵ1��
        //      VK_FILTER_NEAREST��ʹ������ڲ�ֵ����ѡ����ӽ�������������ء�
        //      VK_FILTER_LINEAR��ʹ�����Բ�ֵ������������������Χ���ĸ����صļ�Ȩƽ��ֵ��
        //      VK_FILTER_CUBIC_IMG��ʹ������������ֵ������������������Χ��ʮ�������صļ�Ȩƽ��ֵ��
        //          ���ֵ��һ����ѡ����չ��ֻ����VK_IMG_filter_cubic��չ��֧��ʱ����Ч��
        //  mipmapMode����ʾ�༶��Զ������˷�ʽ��������VK_SAMPLER_MIPMAP_MODE_NEAREST��VK_SAMPLER_MIPMAP_MODE_LINEAR����ʾ���Ӷ��mipmap�����в���ʱ��ʹ������ļ�������ڼ���ļ�Ȩƽ��ֵ��
        //      VkSamplerMipmapMode��һ��ö�����ͣ�����ָ���༶��Զ�������ʱ�Ĺ��˷�ʽ���������ֿ��ܵ�ֵ1��
        //      VK_SAMPLER_MIPMAP_MODE_NEAREST��ʹ������ڲ�ֵ����ѡ����ӽ�LODֵ��mipmap����
        //      VK_SAMPLER_MIPMAP_MODE_LINEAR��ʹ�����Բ�ֵ��������LODֵ��������ӽ���mipmap����֮����л�ϡ�
        //  addressModeU����ʾˮƽ�����Ѱַģʽ��������VK_SAMPLER_ADDRESS_MODE_REPEAT��VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT��VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE��VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER��VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE�ȣ���ʾ���������곬��[0, 1]��Χʱ����δ�������߽硣
        //  addressModeV����ʾ��ֱ�����Ѱַģʽ����addressModeU���ơ�
        //  addressModeW����ʾ��ȷ����Ѱַģʽ����addressModeU���ơ�
        //      VkSamplerAddressMode��һ��ö�����ͣ�����ָ���������ʱ��Ѱַģʽ���������ֿ��ܵ�ֵ1��
        //      VK_SAMPLER_ADDRESS_MODE_REPEAT���ظ����������������곬��[0, 1)��Χʱ������ӳ�䵽�÷�Χ�ڵ���Ӧλ�á�
        //      VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT�������ظ����������������곬��[0, 1)��Χʱ������ӳ�䵽�÷�Χ�ڵ��෴λ�á�
        //      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE����ȡ����Ե�������������곬��[0, 1)��Χʱ����������Ϊ�÷�Χ�ڵ������Եֵ��
        //      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER����ȡ���߽磬�����������곬��[0, 1)��Χʱ����������ΪԤ����ı߽���ɫ��
        //      VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE�������ȡ����Ե�������������곬��[0, 1)��Χʱ����������Ϊ�÷�Χ�ڵ������Եֵ���෴ֵ��
        //      VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE_KHR����VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE��ͬ���������ֵ��һ����ѡ����չ��ֻ����VK_KHR_sampler_mirror_clamp_to_edge��չ��֧��ʱ����Ч��
        //  mipLodBias����ʾ������ӵ�mipmap LOD��ϸ�ڲ�Σ������е�ƫ��ֵ������ʹ�����������������ģ����
        //  anisotropyEnable����ʾ�Ƿ����ø������Թ��ˣ�������VK_TRUE��VK_FALSE���������Թ�����һ�ָ������Ĺ��˷�ʽ�����Լ�����б�ӽ��µ�ģ���;�ݡ�
        //  maxAnisotropy����ʾ�����ø������Թ���ʱ��������ʹ�õ�����������ֵ��������1.0�������豸֧�ֵ����ֵ֮�䡣
        //  compareEnable����ʾ�Ƿ����ñȽϲ�����������VK_TRUE��VK_FALSE���Ƚϲ������ԶԲ������������ȱȽϣ�����ʵ����Ӱ��Ч����
        //  compareOp����ʾ�����ñȽϲ���ʱ��������ʹ�õıȽϲ���������ʾ��αȽϲ�������Ͳο�ֵ��
        //      VkCompareOp��һ��ö�����ͣ�����ָ����Ȼ�ģ�����ʱ�ıȽϲ��������а��ֿ��ܵ�ֵ��
        //      VK_COMPARE_OP_NEVER����Զ����false��
        //      VK_COMPARE_OP_LESS��������ֵС�ڲο�ֵʱ����true��
        //      VK_COMPARE_OP_EQUAL��������ֵ���ڲο�ֵʱ����true��
        //      VK_COMPARE_OP_LESS_OR_EQUAL��������ֵС�ڻ���ڲο�ֵʱ����true��
        //      VK_COMPARE_OP_GREATER��������ֵ���ڲο�ֵʱ����true��
        //      VK_COMPARE_OP_NOT_EQUAL��������ֵ�����ڲο�ֵʱ����true��
        //      VK_COMPARE_OP_GREATER_OR_EQUAL��������ֵ���ڻ���ڲο�ֵʱ����true��
        //      VK_COMPARE_OP_ALWAYS����Զ����true��
        //  minLod����ʾ���ڶԼ���õ���LODֵ����Сֵ�������ƣ����Ա����������ģ����
        //  maxLod����ʾ���ڶԼ���õ���LODֵ�����ֵ�������ƣ����Ա����������������Ϊ�˱����������ֵ�����԰�maxLod����Ϊ����VK_LOD_CLAMP_NONE��
        //  borderColor����ʾ��ѰַģʽΪ�߽�ģʽʱ��������ʹ�õ�Ԥ����߽���ɫ��
        //      VkBorderColor��һ��ö�����ͣ�����ָ����������ȡ���߽�ʱ��Ԥ������ɫ�����а��ֿ��ܵ�ֵ2��
        //      VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK����ʾ(0.0, 0.0, 0.0, 0.0)�ĸ�����ɫ��
        //      VK_BORDER_COLOR_INT_TRANSPARENT_BLACK����ʾ(0, 0, 0, 0)��������ɫ��
        //      VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK����ʾ(0.0, 0.0, 0.0, 1.0)�ĸ�����ɫ��
        //      VK_BORDER_COLOR_INT_OPAQUE_BLACK����ʾ(0, 0, 0, 1)��������ɫ��
        //      VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE����ʾ(1.0, 1.0, 1.0, 1.0)�ĸ�����ɫ��
        //      VK_BORDER_COLOR_INT_OPAQUE_WHITE����ʾ(1, 1, 1, 1)��������ɫ��
        //      VK_BORDER_COLOR_FLOAT_CUSTOM_EXT����ʾһ���Զ���ĸ�����ɫ����Ҫʹ��VkSamplerCustomBorderColorCreateInfoEXT�ṹ��ָ�������ֵ��һ����ѡ����չ��ֻ����VK_EXT_custom_border_color��չ��֧��ʱ����Ч��
        //      VK_BORDER_COLOR_INT_CUSTOM_EXT����ʾһ���Զ����������ɫ����Ҫʹ��VkSamplerCustomBorderColorCreateInfoEXT�ṹ��ָ�������ֵ��һ����ѡ����չ��ֻ����VK_EXT_custom_border_color��չ��֧��ʱ����Ч��
        //  unnormalizedCoordinates����ʾ�Ƿ�ʹ�÷ǹ�һ��������Ѱַ����������VK_TRUE��VK_FALSE���ǹ�һ������ķ�Χ��0��ͼ��Ĵ�С��������0��1��
        // ����һ�������������������������Ϣ�Ľṹ��
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;          // ���ýṹ������ͣ�������VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
        samplerInfo.magFilter = VK_FILTER_LINEAR;                           // ���÷Ŵ���˷�ʽ��magFilter��Ϊ���Թ��ˣ�linear filtering������ʾ�������Ŵ�ʱ��ʹ���������صļ�Ȩƽ��ֵ
        samplerInfo.minFilter = VK_FILTER_LINEAR;                           // ������С���˷�ʽ��minFilter��Ϊ���Թ��ˣ���ʾ��������Сʱ��ʹ���������صļ�Ȩƽ��ֵ
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;             // ���ö༶��Զ������˷�ʽ��mipmap mode��Ϊ���Թ��ˣ���ʾ���Ӷ��mipmap�����в���ʱ��ʹ�����ڼ���ļ�Ȩƽ��ֵ
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;          // ����ˮƽ�����Ѱַģʽ��addressModeU��Ϊ�ظ�ģʽ��repeat mode������ʾ���������곬��[0, 1]��Χʱ���ظ�����ͼ��
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;          // ���ô�ֱ�����Ѱַģʽ��addressModeV��Ϊ�ظ�ģʽ����ʾ���������곬��[0, 1]��Χʱ���ظ�����ͼ��
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;          // ������ȷ����Ѱַģʽ��addressModeW��Ϊ�ظ�ģʽ����ʾ���������곬��[0, 1]��Χʱ���ظ�����ͼ��
        samplerInfo.anisotropyEnable = VK_TRUE;                             // ���ø������Թ��ˣ�anisotropic filtering��Ϊ���ã���ʾʹ��һ�ָ��������Ĺ��˷�ʽ�����Լ�����б�ӽ��µ�ģ���;��
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // ��������������ֵΪ�����豸֧�ֵ����ֵ����ʾʹ����̶߳ȵĸ������Թ���
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;         // ���ñ߽���ɫ��borderColor��Ϊ��͸����ɫ��opaque black������ʾ��ѰַģʽΪ�߽�ģʽ��border mode��ʱ��ʹ�ú�ɫ��䳬����Χ������
        samplerInfo.compareEnable = VK_FALSE;                               // �����Ƿ����ñȽϲ�����compare operation��Ϊ�񣬱�ʾ���Բ������������ȱȽ�
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;                       // ���ñȽϲ�������compare operator��Ϊʼ��ͨ����always pass������ʾ���Բ������������ȱȽ�
        samplerInfo.unnormalizedCoordinates = VK_FALSE;                     // �����Ƿ�ʹ�÷ǹ�һ�����꣨unnormalized coordinates��Ϊ�񣬱�ʾʹ��[0, 1]��Χ�ڵĹ�һ��������Ѱַ����

        // ���ô�������������ĺ��������봴����Ϣ�ṹ��Ϳ�ָ����Ϊ����ص�����������һ���������������
        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            // �������ʧ�ܣ��׳�һ������ʱ����
            throw std::runtime_error("failed to create texture sampler!");
        }
    }


    VkImageView createImageView(VkImage image, VkFormat format) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view!");
        }

        return imageView;
    }

    // ����һ�����������ڴ���һ��ͼ������һ���豸�ڴ���󡣺����Ĳ����ֱ��ǣ�
    // width��ͼ��Ŀ�ȣ�height��ͼ��ĸ߶ȣ�format��ͼ��ĸ�ʽ��tiling��ͼ������з�ʽ
    // usage��ͼ�����;��properties���豸�ڴ������
    // image���������͵Ĳ��������մ�����ͼ�����imageMemory���������͵Ĳ��������մ������豸�ڴ����
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        // ����һ���ṹ�����VkImageCreateInfo������ָ��ͼ�����Ĵ�����Ϣ��������Ա�ĺ������£�
        //  sType��һ��VkStructureTypeֵ����ʾ����ṹ������͡���ΪVK_STRUCTURE_TYPE_IMAGE_CREATE_INFO��
        //  pNext��һ��ָ�룬ָ��һ����չ�ṹ�壬�����ṩ����Ĳ�����
        //  flags��һ��λ��bit field������ʾͼ�����ĸ������ԣ������Ƿ���������ݣ�cube compatible�����Ƿ�ϡ��󶨣�sparse binding���ȡ�
        //  imageType��һ��VkImageTypeֵ����ʾͼ�����Ļ���ά�ȣ�����һά��1D������ά��2D������ά��3D����
        //  format��һ��VkFormatֵ����ʾͼ�����ĸ�ʽ�����ͣ�����RGBA�����ģ�壨depth stencil���ȡ�
        //  extent��һ��VkExtent3D�ṹ�壬��ʾͼ�����ĳߴ磬������ȡ��߶Ⱥ���ȡ�
        //  mipLevels��һ����������ʾͼ�����Ķ༶��Զ����mipmap��������
        //  arrayLayers��һ����������ʾͼ�����ġ����������Ĳ�����
        //  samples��һ��VkSampleCountFlagBitsֵ����ʾͼ������ÿ�����أ�texel���Ĳ�������������ʵ�ֶ��ز�������ݣ�ͼ�ι��������н��ܡ�
        //  tiling��һ��VkImageTilingֵ����ʾͼ��������ڴ��е����з�ʽ���������ԣ�linear������ѣ�optimal����
        //      VK_IMAGE_TILING_OPTIMAL��ʾͼ�����ݰ�����ѷ�ʽ���У������豸����
        //          ��������Щ��Ҫ��Ч�ر��豸��GPU�����ʵ�ͼ���������������Ⱦ���������洢�Ȳ�����ͼ�����
        //          ͨ������֧�ָ����ͼ����;�͸�ʽ�����ǲ���ֱ�ӱ�������CPU��ӳ����ơ�
        //      VK_IMAGE_TILING_LINEAR��ʾͼ�����ݰ������Է�ʽ���У�������������
        //          ��������Щ��Ҫֱ�ӱ��������ʻ��Ƶ�ͼ������������ڴ�������ȡ��д�����ݵ�ͼ�����
        //          ͨ��ֻ��֧�ִ���Դ����Ŀ�����;�����ҶԸ�ʽ�����ơ�
        //      VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT����ʾͼ������з�ʽ��һ��Linux DRM��ʽ���η����壬����һ����չֵ����Ҫ֧��VK_EXT_image_drm_format_modifier��չ
        //  usage��һ��λ�򣬱�ʾͼ��������;�����紫��Դ��transfer source��������Ŀ�꣨transfer destination��������Դ��sampled source���ȡ����㻺�����н���
        //  sharingMode��һ��VkSharingModeֵ����ʾͼ������ڶ�������壨queue family��֮��Ĺ���ģʽ�������ռ��exclusive���򲢷���concurrent�����������������н���
        //  queueFamilyIndexCount��һ����������ʾ����ͼ�����Ķ������������
        //  pQueueFamilyIndices��һ��ָ�룬ָ��һ���������飬��ʾ����ͼ�����Ķ������������
        //  initialLayout��һ��VkImageLayoutֵ����ʾͼ�����ĳ�ʼ���֣�����δ���壨undefined��������Ŀ����ѣ�transfer destination optimal���ȡ�
        //      VkImageLayout��һ��ö�����ͣ�����ָ��ͼ������ͼ������Դ�Ĳ���1�����־�����ͼ���������ڴ��е����з�ʽ���Լ�ͼ����Ա���Щ��������2��VkImageLayout�����¿��ܵ�ֵ��
        //          VK_IMAGE_LAYOUT_UNDEFINED����ʾͼ��Ĳ�����δ����ģ���������ͼ������ݡ�
        //              ���ֵͨ�����ڴ���ͼ��ʱָ����ʼ���֣�����ת��ͼ��ʱָ��Դ���֡�
        //          VK_IMAGE_LAYOUT_GENERAL����ʾͼ��Ĳ�����ͨ�õģ�
        //              �����Ա��κβ������ʣ��������ܿ��ܲ�����ѵġ����ֵͨ��������Щû���ض���;���ʽ��ͼ��
        //          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL����ʾͼ��Ĳ�������ɫ������ѵģ����ʺ���Ϊ��Ⱦͨ����render pass������ɫ������
        //              ���ֵͨ��������Щ������Ⱦ�����ͼ��
        //          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL����ʾͼ��Ĳ��������ģ�帽����ѵģ����ʺ���Ϊ��Ⱦͨ�������ģ�帽����
        //              ���ֵͨ��������Щ������Ȳ��Ի�ģ����Ե�ͼ��
        //          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL����ʾͼ��Ĳ��������ģ��ֻ����ѵģ����ʺ���Ϊֻ�����ģ�帽�������Դ��
        //              ���ֵͨ��������Щ������Ӱ��ͼ������ֻ�����ģ�������ͼ��
        //          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL����ʾͼ��Ĳ�������ɫ��ֻ����ѵģ����ʺ���Ϊ��ɫ�����������븽��������Դ��
        //              ���ֵͨ��������Щ���ڲ������洢������ֻ����ɫ��������ͼ��
        //          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL����ʾͼ��Ĳ����Ǵ���Դ��ѵģ����ʺ���Ϊ���������transfer operation����Դ��
        //              ���ֵͨ��������Щ���ڸ��ơ��������������Դͼ��
        //          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL����ʾͼ��Ĳ����Ǵ���Ŀ����ѵģ����ʺ���Ϊ���������Ŀ�ꡣ
        //              ���ֵͨ��������Щ���ڸ��ơ��������������Ŀ��ͼ��
        //          VK_IMAGE_LAYOUT_PREINITIALIZED����ʾͼ���Ѿ���������ʼ�������������Ѿ�д����һЩ���ݵ�ͼ���С�
        //              ���ֵͨ�����ڴ���ͼ��ʱָ����ʼ���֣��Ա��ⲻ��Ҫ�����ͼ�����ݡ�
        //          VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL����ʾͼ��Ĳ��������ֻ��ģ�帽����ѵģ�
        //              ���ʺ���Ϊֻ����ȸ����Ϳ�дģ�帽�������ֵͨ��������Щ��Ҫͬʱ������Ⱥ�ģ�����ݵ���Ⱦͨ����
        //          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL����ʾͼ��Ĳ�������ȸ���ģ��ֻ����ѵģ�
        //              ���ʺ���Ϊ��д��ȸ�����ֻ��ģ�帽�������ֵͨ��������Щ��Ҫͬʱ������Ⱥ�ģ�����ݵ���Ⱦͨ����
        //          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL����ʾͼ��Ĳ�������ȸ�����ѵģ����ʺ���Ϊ��д��ȸ�����
        //              ���ֵͨ��������Щֻ��Ҫ����������ݶ�����Ҫ����ģ�����ݵ���Ⱦͨ����
        //          VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL����ʾͼ��Ĳ��������ֻ����ѵģ����ʺ���Ϊֻ����ȸ��������Դ��
        //              ���ֵͨ��������Щֻ��Ҫ����������ݶ�����Ҫ����ģ�����ݵ���Ⱦͨ������ɫ��������
        //          VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL����ʾͼ��Ĳ�����ģ�帽����ѵģ����ʺ���Ϊ��дģ�帽����
        //              ���ֵͨ��������Щֻ��Ҫ����ģ�����ݶ�����Ҫ����������ݵ���Ⱦͨ����
        //          VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL����ʾͼ��Ĳ�����ģ��ֻ����ѵģ����ʺ���Ϊֻ��ģ�帽�������Դ��
        //              ���ֵͨ��������Щֻ��Ҫ����ģ�����ݶ�����Ҫ����������ݵ���Ⱦͨ������ɫ��������
        //          VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL����ʾͼ��Ĳ�����ֻ����ѵģ����ʺ���Ϊֻ�����������Դ��
        //              ���ֵͨ��������Щ����Ҫд��ͼ�����ݵ���Ⱦͨ������ɫ��������
        //          VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL����ʾͼ��Ĳ����Ǹ�����ѵģ����ʺ���Ϊ��д������
        //              ���ֵͨ��������Щ��Ҫд��ͼ�����ݵ���Ⱦͨ����
        //          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR����ʾͼ��Ĳ����ǳ���Դ�ģ����ʺ���Ϊ��������swapchain���ĳ���Դ��
        //              ���ֵͨ��������Щ������ʾ�����ͼ��
        //          VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR����ʾͼ��Ĳ�������Ƶ����Ŀ��ģ����ʺ���Ϊ��Ƶ���������video decode operation����Ŀ�ꡣ
        //              ���ֵͨ��������Щ������Ƶ���������ͼ��
        //          VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR����ʾͼ��Ĳ�������Ƶ����Դ�ģ����ʺ���Ϊ��Ƶ���������Դ��
        //              ���ֵͨ��������Щ������Ƶ���������ͼ��
        //          VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR����ʾͼ��Ĳ�������Ƶ����DPB��decoded picture buffer���ģ����ʺ���Ϊ��Ƶ���������DPB��
        //              ���ֵͨ��������Щ���ڴ洢��Ƶ��������вο�֡��reference frame����ͼ��
        //          VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR����ʾͼ����Ա�����豸������֣����ʺ���Ϊ������ֲ�����shared present operation����Դ��
        //              ���ֵͨ��������Щ���ڿ��豸��ʾ�����ͼ��
        //          VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT����ʾͼ��Ĳ�����Ƭ���ܶ���ͼ��ѣ�fragment density map optimal���ģ����ʺ���ΪƬ���ܶ���ͼ��fragment density map��������Դ��
        //              ���ֵͨ��������Щ���ڴ洢Ƭ���ܶ���Ϣ��fragment density information����ͼ��
        //          VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR����ʾͼ��Ĳ�����Ƭ����ɫ�ʸ�����ѣ�fragment shading rate attachment optimal���ģ����ʺ���ΪƬ����ɫ�ʸ�����fragment shading rate attachment��������Դ��
        //              ���ֵͨ��������Щ���ڴ洢Ƭ����ɫ����Ϣ��fragment shading rate information����ͼ��
        //          VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR����ʾͼ��Ĳ�������Ƶ����Ŀ�꣨video encode destination���ģ����ʺ���Ϊ��Ƶ���������video encode operation����Ŀ�ꡣ
        //              ���ֵͨ��������Щ������Ƶ���������ͼ��
        //          VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR����ʾͼ��Ĳ�������Ƶ����Դ��video encode source���ģ����ʺ���Ϊ��Ƶ���������video encode operation����Դ��
        //              ���ֵͨ��������Щ������Ƶ���������ο�֡��reference frame����ͼ��
        //          VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR����ʾͼ��Ĳ�������Ƶ����DPB��decoded picture buffer�� �ģ����ʺ���Ϊ��Ƶ���������video encode operation�� ��DPB��
        // 
        //          ������һ�ִ�ͼ������л�ȡ����ֵ�Ĳ�����ͨ������ɫ����ʹ��ͼ���������image sampler�������С�
        //              ������Ŀ����Ϊ�˽�ͼ������ת��Ϊ��ɫ������ʹ�õ���ɫֵ���Ա�����Ⱦ����㡣
        //          ������һ�ֽ����ݴ�һ���ڴ�����Ƶ���һ���ڴ����Ĳ�����ͨ�������������ʹ�ø������copy command�������С�
        //              �����Ŀ����Ϊ�˽����ݴ�CPU�ڴ��ƶ���GPU�ڴ棬������GPU�ڴ��н����������л�ת����
        //              ...
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = format;                              // ���ýṹ�������format��ԱΪ���������е�format����ʾͼ��ĸ�ʽ
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;                                // ���ýṹ�������mipLevels��ԱΪ1����ʾֻ��һ���༶��Զ����mipmap��
        imageInfo.arrayLayers = 1;                              
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = tiling;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;      // ���ýṹ�������sharingMode��ԱΪVK_SHARING_MODE_EXCLUSIVE����ʾͼ��ֻ�ܱ�һ�������壨queue family��ӵ��
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;    // ���ýṹ�������initialLayout��ԱΪVK_IMAGE_LAYOUT_UNDEFINED����ʾͼ��ĳ�ʼ������δ����ģ���������ͼ�������
        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) { // �����������Ӧ��һ��ͼ�����
            throw std::runtime_error("failed to create image!");
        }
        
        VkMemoryRequirements memRequirements;                   // ����һ���ṹ����������ڴ洢ͼ�������ڴ�������Ϣ
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;        // ���ýṹ�������allocationSize��ԱΪmemRequirements.size����ʾҪ������豸�ڴ�Ĵ�С����ͼ�������ڴ������С
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);// findMemoryType������һ���Զ���ĺ��������ڸ��ݸ������ڴ�����λ��������ҵ����ʵ��ڴ���������

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);       // ����Vulkan API���������豸�ڴ����󶨵�ͼ�������
    }

    // ����һ���ı�ͼ�񲼾ֵĺ���
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();      // ����һ����ʱ���������

        // �ṹ��VkImageMemoryBarrier������ָ��һ��ͼ���ڴ����ϣ�image memory barrier���Ĳ�����
        //  ͼ���ڴ�������һ������ͬ��ͼ�����ķ��ʺͲ��ֵĻ��ơ��ṹ��VkImageMemoryBarrier�������³�Ա��
        //      sType����ʾ�ṹ������ͣ�������VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER��
        //      pNext����ʾָ����չ�ṹ���ָ�룬���û����չ������ΪNULL��
        //      srcAccessMask����ʾԴ�������루source access mask������һ��λ���룬ָ��������֮ǰ����Щ���͵Ĳ������Է���ͼ�����
        //      dstAccessMask����ʾĿ��������루destination access mask������һ��λ���룬ָ��������֮����Щ���͵Ĳ������Է���ͼ�����
        //          VK_ACCESS_INDIRECT_COMMAND_READ_BIT����ʾ���Զ�ȡ������indirect command���Ļ�������buffer����
        //          VK_ACCESS_INDEX_READ_BIT����ʾ���Զ�ȡ������index���Ļ�������
        //          VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT����ʾ���Զ�ȡ�������ԣ�vertex attribute���Ļ�������
        //          VK_ACCESS_UNIFORM_READ_BIT����ʾ���Զ�ȡͳһ������uniform���Ļ�������ͼ��image����
        //          VK_ACCESS_INPUT_ATTACHMENT_READ_BIT����ʾ���Զ�ȡ���븽����input attachment����ͼ��
        //          VK_ACCESS_SHADER_READ_BIT����ʾ���Զ�ȡ��ɫ����shader���Ļ�������ͼ��
        //          VK_ACCESS_SHADER_WRITE_BIT����ʾ����д����ɫ���Ļ�������ͼ��
        //          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT����ʾ���Զ�ȡ��ɫ������color attachment����ͼ��
        //          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT����ʾ����д����ɫ������ͼ��
        //          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT����ʾ���Զ�ȡ���ģ�帽����depth stencil attachment����ͼ��
        //          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT����ʾ����д�����ģ�帽����ͼ��
        //          VK_ACCESS_TRANSFER_READ_BIT����ʾ���Զ�ȡ���������transfer operation���Ļ�������ͼ��
        //          VK_ACCESS_TRANSFER_WRITE_BIT����ʾ����д�봫������Ļ�������ͼ��
        //          VK_ACCESS_HOST_READ_BIT����ʾ������������host����ȡ���ڴ����
        //          VK_ACCESS_HOST_WRITE_BIT����ʾ����������д����ڴ����
        //          VK_ACCESS_MEMORY_READ_BIT����ʾ�������κβ�����ȡ���ڴ����
        //          VK_ACCESS_MEMORY_WRITE_BIT����ʾ�������κβ���д����ڴ����
        //      oldLayout����ʾԴ���֣�old layout������һ��ö��ֵ��ָ��������֮ǰ��ͼ�����Ĳ��֡���
        //      newLayout����ʾĿ�겼�֣�new layout������һ��ö��ֵ��ָ��������֮��ͼ�����Ĳ��֡�
        //      srcQueueFamilyIndex����ʾԴ������������source queue family index������һ��������ָ��������֮ǰ���ĸ������壨queue family��ӵ��ͼ�����ķ���Ȩ���������Ҫ����������Ȩת�ƣ�queue family ownership transfer������������ΪVK_QUEUE_FAMILY_IGNORED��
        //      dstQueueFamilyIndex����ʾĿ�������������destination queue family index������һ��������ָ��������֮���ĸ�������ӵ��ͼ�����ķ���Ȩ���������Ҫ����������Ȩת�ƣ���������ΪVK_QUEUE_FAMILY_IGNORED��
        //      image����ʾ�ܵ�����Ӱ���ͼ�����image object������һ�������handle����
        //      subresourceRange����ʾ�ܵ�����Ӱ���ͼ������Դ��Χ��image subresource range������һ���ṹ�壬ָ��ͼ������е���Щ����(�༶��Զ����㼶������㼶)��Ҫͬ����
        VkImageMemoryBarrier barrier{};                                 // ����һ��ͼ���ڴ����Ͻṹ��
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;         // ���ýṹ������
        barrier.oldLayout = oldLayout;                                  // ����Դ����
        barrier.newLayout = newLayout;                                  // ����Ŀ�겼��
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;          // ����Դ���������������Ա�ʾ����Ҫ����������Ȩת��
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;          // ����Ŀ����������������Ա�ʾ����Ҫ����������Ȩת��
        barrier.image = image;                                          // ����Ҫ�ı䲼�ֵ�ͼ�����
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;                      // ����Ҫ�ı䲼�ֵ�����Դ��Χ������ֻ�ı���ɫ���棬��0�㼶����0�����
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        // VkPipelineStageFlags��28��ֵ��������������ʾ���߽׶����루pipeline stage mask����
        //    ָ��ͬ����Χ��synchronization scope�������Ĺ��߽׶Σ�pipeline stage�������ǿ���������ö��ֵ��enum values������ϣ�
        //    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT����ʾ���߿�ʼ�׶Σ���ִ���κβ�����
        //    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT����ʾ���Ƽ�ӽ׶Σ�ִ�м�ӻ������
        //    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT����ʾ��������׶Σ���ȡ������������ݡ�
        //    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT����ʾ������ɫ���׶Σ�ִ�ж�����ɫ������
        //    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT����ʾ����ϸ�ֿ�����ɫ���׶Σ�ִ������ϸ�ֿ�����ɫ������
        //    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT����ʾ����ϸ��������ɫ���׶Σ�ִ������ϸ��������ɫ������
        //    VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT����ʾ������ɫ���׶Σ�ִ�м�����ɫ������
        //    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT����ʾƬ����ɫ���׶Σ�ִ��Ƭ����ɫ������
        //    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT����ʾ����Ƭ�β��Խ׶Σ�ִ����Ⱥ�ģ����ԡ�
        //    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT����ʾ����Ƭ�β��Խ׶Σ�ִ����Ⱥ�ģ����ԡ�
        //    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT����ʾ��ɫ��������׶Σ�д����ɫ������
        //    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT����ʾ������ɫ���׶Σ�ִ�м�����ɫ������
        //    VK_PIPELINE_STAGE_TRANSFER_BIT����ʾ����׶Σ�ִ�д��������
        //    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT����ʾ���߽����׶Σ���ִ���κβ�����
        //    VK_PIPELINE_STAGE_HOST_BIT����ʾ�����׶Σ������������ڴ����
        //    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT����ʾ����ͼ�ν׶Σ������������롢������ɫ��������ϸ����ɫ����������ɫ����Ƭ����ɫ����Ƭ�β��Ժ���ɫ��������ȡ�
        //    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT����ʾ��������׶Σ���������ͼ�ν׶Ρ�������ɫ���׶κʹ���׶εȡ�
        //    ������Щ�����Ĺ��߽׶�����ֵ������һЩ��չ�Ĺ��߽׶�����ֵ����������һЩ�ض�����չ�����ṩ�ġ����ǿ���������Ĺ��߽׶�����ֵһ��ʹ�ã���ʵ�ָ�ϸ���ȵ�ͬ��������������ö��ֵ��
        //
        //    VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT����ʾ������Ⱦ�׶Σ���VK_EXT_conditional_rendering��չ�ṩ��ִ��������Ⱦ������
        //    VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT����ʾ�任�����׶Σ���VK_EXT_transform_feedback��չ�ṩ��ִ�б任����������
        //    VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV����ʾ����Ԥ����׶Σ���VK_NV_device_generated_commands��չ�ṩ��ִ������Ԥ���������
        //    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR����ʾ���ٽṹ�����׶Σ���VK_KHR_acceleration_structure��չ�ṩ��ִ�м��ٽṹ����������
        //    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR����ʾ����׷����ɫ���׶Σ���VK_KHR_ray_tracing_pipeline��չ�ṩ��ִ�й���׷����ɫ������
        //    VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV����ʾ��ɫ��ͼ��׶Σ���VK_NV_shading_rate_image��չ�ṩ����ȡ��ɫ��ͼ�����ݡ�
        //    VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV����ʾ������ɫ���׶Σ���VK_NV_mesh_shader��չ�ṩ��ִ��������ɫ������
        //    VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV����ʾ������ɫ���׶Σ���VK_NV_mesh_shader��չ�ṩ��ִ��������ɫ������
        //    VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT����ʾƬ���ܶȴ���׶Σ���VK_EXT_fragment_density_map��չ�ṩ����ȡƬ���ܶ�ͼ�����ݡ�
        VkPipelineStageFlags sourceStage;  // ����Դ���߽׶κ�Ŀ����߽׶εı�־
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            // ���Դ������δ����ģ�Ŀ�겼���Ǵ���Ŀ����ѵģ���ô������֮ǰ����Ҫ�κη������룬֮����Ҫ����д���������
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;        // Դ���߽׶��ǹ��߿�ʼ�׶�
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;      // Ŀ����߽׶��Ǵ���׶�
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            // ���Դ�����Ǵ���Ŀ����ѵģ�Ŀ�겼������ɫ��ֻ����ѵģ���ô������֮ǰ��Ҫ����д��������룬֮����Ҫ��ɫ����ȡ��������
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;   // Դ���߽׶��Ǵ���׶�
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;      // Ŀ����߽׶���Ƭ����ɫ���׶�

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            // ���Դ���ֺ�Ŀ�겼�ֲ������������������ô�׳�һ���쳣����ʾ��֧�����ֲ���ת��
            throw std::invalid_argument("unsupported layout transition!");
        }

        // ����vkCmdPipelineBarrier�����������������command buffer���м�¼һ���������ϣ�pipeline barrier������ġ�
        //      ��������������һ�����ڶ����ڴ������ԣ�memory dependency���Ͳ���ת����layout transition��������5������vkCmdPipelineBarrier�������²�����
        //  commandBuffer����ʾҪ��¼��������������
        //      srcStageMask����ʾԴ���߽׶����루source pipeline stage mask������һ��λ���룬ָ����һ��ͬ����Χ��synchronization scope�������Ĺ��߽׶Σ�pipeline stage����
        //      dstStageMask����ʾĿ����߽׶����루destination pipeline stage mask������һ��λ���룬ָ���ڶ���ͬ����Χ�����Ĺ��߽׶Ρ�
        //      dependencyFlags����ʾ�����Ա�־��dependency flags������һ��λ���룬ָ������γ�ִ�������ԣ�execution dependency�����ڴ������ԡ�
        //      memoryBarrierCount����ʾ�ڴ���������ĳ��ȡ�
        //      pMemoryBarriers����ʾָ���ڴ����������ָ�롣�ڴ���������ͬ�������������ݵķ���6��
        //      bufferMemoryBarrierCount����ʾ�������ڴ���������ĳ��ȡ�
        //      pBufferMemoryBarriers����ʾָ�򻺳����ڴ����������ָ�롣�������ڴ���������ͬ������������buffer object���ķ���7��
        //      imageMemoryBarrierCount����ʾͼ���ڴ���������ĳ��ȡ�
        //      pImageMemoryBarriers����ʾָ��ͼ���ڴ����������ָ�롣ͼ���ڴ���������ͬ��ͼ�����ķ��ʺͲ���8��
        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    // ����һ�����ڸ������ݵĺ�������������Դ����������Ŀ��ͼ������Լ�ͼ��Ŀ�Ⱥ͸߶�
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        // �ṹ��VkBufferImageCopy������������������ͼ��֮�临������Ĳ����������������ֶΣ�
        //     bufferOffset����ʾ�����������ݵ�ƫ��������λ���ֽڡ�
        //     bufferRowLength����ʾ��������ÿ�����ݵĳ��ȣ���λ�����ء����Ϊ0����ʾʹ��ͼ��Ŀ�ȡ�
        //     bufferImageHeight����ʾ��������ÿ��ͼ���ĸ߶ȣ���λ�����ء����Ϊ0����ʾʹ��ͼ��ĸ߶ȡ�
        //     imageSubresource����ʾͼ������Դ�ķ��桢mipmap���������㡣
        //     imageOffset����ʾͼ�������ݵ�ƫ��������λ�����ء�
        //     imageExtent����ʾͼ�������ݵķ�Χ����λ�����ء�
        // ����һ������������������ͼ��֮�临������Ľṹ��
        VkBufferImageCopy region{};
        region.bufferOffset = 0;                                        // ���û����������ݵ�ƫ����Ϊ0
        region.bufferRowLength = 0;                                     // ���û�������ÿ�����ݵĳ���Ϊ0����ʾʹ��ͼ��Ŀ��
        region.bufferImageHeight = 0;                                   // ���û�������ÿ��ͼ���ĸ߶�Ϊ0����ʾʹ��ͼ��ĸ߶�

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // ����ͼ������Դ�ķ�������Ϊ��ɫ����
        region.imageSubresource.mipLevel = 0;                           // ����ͼ������Դ��mipmap����Ϊ0����ʾ��߷ֱ��ʵļ���
        region.imageSubresource.baseArrayLayer = 0;                     // ����ͼ������Դ����������Ϊ0
        region.imageSubresource.layerCount = 1;                         // ����ͼ������Դ���������Ϊ1����ʾֻ��һ��

        region.imageOffset = { 0, 0, 0 };                               // ����ͼ�������ݵ�ƫ����Ϊ(0, 0, 0)����ʾ�����Ͻǿ�ʼ����
        region.imageExtent = {                                          // ����ͼ�������ݵķ�ΧΪ(width, height, 1)����ʾ��������ͼ��
            width,
            height,
            1
        };

        // ����������м�¼һ����������������������е����ݸ��Ƶ�ͼ������У�����ͼ������Ѿ����ڴ���Ŀ���Ż�����
        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
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

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }
    // ��ע��
    void createDescriptorPool() {
        // ����������������������һ���Ǳ任����Ļ�������һ������������Ĳ������������ֱ����������岻ͬ֡��uniform�������������
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            // typedef struct VkDescriptorImageInfo {
            //     VkSampler        sampler;
            //     VkImageView      imageView;
            //     VkImageLayout    imageLayout;
            // } VkDescriptorImageInfo;
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            // ����������Ӧ�Ĳ�����������������Ϊ��ǰ֡��������
            // ����������Ϊ���������ͣ����������ְ󶨵�Ϊ1���������������飬����������Ϊ1��
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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
    //
    VkCommandBuffer beginSingleTimeCommands() {
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

        return commandBuffer;
    }
    //
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    //
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
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

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
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

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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