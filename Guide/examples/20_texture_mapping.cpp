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
    // 有注释
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
        // 用于创建采样器对象时指定是否启用各向异性过滤。
        // 各向异性过滤是一种纹理过滤技术，可以在不同方向上使用不同的纹理级别，从而提高纹理的质量和细节

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
    // 有注释
    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER：用于采样纹理的图像和采样器
        // 位于片段着色器附近
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
        // 定义一个函数，用于创建纹理图像，结构与createindexbuffer相似。创建一个中转缓冲区，然后再把纹理图像复制到gpu；
        // 定义三个变量，用于存储纹理图像的宽度，高度和通道数，STBI_rgb_alpha表示将图片转换为RGBA格式，每个通道占用一个字节
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("examples/Assets/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;              // 计算图片的总字节数，等于宽度乘以高度乘以4（因为每个像素占用4个字节）
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;                                                     // 定义一个void类型的指针，用于映射设备内存到主机内存
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);                                        // 调用stb_image库函数，释放pixels指向的内存空间

        //VkImage textureImage;
        //VkDeviceMemory textureImageMemory;
        //  定义两个变量，分别表示纹理图像对象和纹理图像对象对应的设备内存对象
        //  这两个变量是全局的，因为后面的函数会用到它们
        
        // texWidth和texHeight分别表示图像的宽度和高度
        // VK_FORMAT_R8G8B8A8_SRGB表示图像的格式是RGBA，每个通道占用一个字节，使用sRGB颜色空间
        // VK_IMAGE_TILING_OPTIMAL表示图像的排列方式是最佳的，即由驱动程序选择最合适的方式
        // VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT表示图像的用途是作为传输目标和采样源
        // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT表示设备内存的属性是设备本地的，即只能由GPU访问，不能由主机访问
        // textureImage和textureImageMemory分别接收创建的图像对象和设备内存对象
        createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

        // 调用一个自定义的函数，转换图像为传输目标最佳的布局
        // textureImage是要转换布局的图像对象
        // VK_FORMAT_R8G8B8A8_SRGB表示图像的格式
        // VK_IMAGE_LAYOUT_UNDEFINED表示图像的初始布局是未定义的，即不关心图像的内容
        // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL表示图像的目标布局是传输目标最佳的，即适合作为传输操作的目标
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        // 调用一个自定义的函数，将缓冲区中的数据复制到图像中
        // stagingBuffer是作为传输源的缓冲区对象
        // textureImage是作为传输目标的图像对象
        // static_cast<uint32_t>(texWidth)和static_cast<uint32_t>(texHeight)分别表示图像的宽度和高度，使用static_cast将int类型转换为uint32_t类型
        copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        // 再次调用一个自定义的函数，转换图像的布局
        // textureImage是要转换布局的图像对象
        // VK_FORMAT_R8G8B8A8_SRGB表示图像的格式
        // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL表示图像的初始布局是传输目标最佳的
        // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL表示图像的目标布局是着色器只读最佳的，即适合作为着色器操作的源
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    // Image是Vulkan中表示图像的对象，它可以包含颜色、深度、模板等数据，也可以有不同的维度、格式、平铺方式、使用方式和布局等属性。
    //      Image不能直接被渲染管线访问，它只是一个内存区域，用于存储图像数据。
    // ImageView是Vulkan中表示图像视图的对象，它是对Image的一个引用或子集，用于描述如何访问Image中的数据。
    //      ImageView可以指定Image的一部分子资源，比如mipmap级别和数组层，也可以指定Image的方面和格式，比如颜色或深度方面，以及浮点数或整数格式。
    //      ImageView还可以启用立方体映射和通道混合等功能
    // 一个图像可以包含多个子资源，比如多个mipmap级别或者多个数组层。但是一个图像视图只能引用一个子资源范围，比如一个mipmap级别或者一个数组层

    // 当需要在着色器中访问纹素数据时，需要用到图像视图。因为着色器不能直接访问图像对象，而是通过绑定点来访问与之关联的图像视图。
    // 当需要在命令缓冲区中执行一些操作时，需要用到图像。比如复制、转换、清除等操作都需要指定操作的目标或源图像。

    void createTextureImageView() {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
    }

    // 定义一个用于创建纹理采样器的函数
    void createTextureSampler() {
        // VkPhysicalDeviceProperties是一个结构体，用于描述物理设备的属性。它有以下成员1：
        //  apiVersion：表示设备支持的Vulkan版本，编码方式参见。
        //  driverVersion：表示驱动程序的厂商指定版本。
        //  vendorID：表示物理设备厂商的唯一标识符。
        //  deviceID：表示物理设备在同一厂商下的唯一标识符。
        //  deviceType：表示物理设备的类型，是一个VkPhysicalDeviceType枚举值。
        //  deviceName：表示物理设备的名称，是一个长度为VK_MAX_PHYSICAL_DEVICE_NAME_SIZE的char数组，包含一个以空字符结尾的UTF - 8字符串。
        //  pipelineCacheUUID：表示物理设备的通用唯一标识符，是一个长度为VK_UUID_SIZE的uint8_t数组。
        //  limits：表示物理设备的特定限制，是一个VkPhysicalDeviceLimits结构体。
        //      VkPhysicalDeviceLimits结构体用于描述物理设备的特定限制，它有很多成员，我只列举一些常用的1：
        //          maxImageDimension1D：表示一维图像的最大宽度。
        //          maxImageDimension2D：表示二维图像的最大宽度和高度。
        //          maxImageDimension3D：表示三维图像的最大宽度、高度和深度。
        //          maxImageDimensionCube：表示立方体图像的最大宽度和高度。
        //          maxImageArrayLayers：表示图像数组的最大层数。
        //          maxTexelBufferElements：表示纹素缓冲区的最大元素数。
        //          maxUniformBufferRange：表示统一缓冲区的最大字节范围。
        //          maxStorageBufferRange：表示存储缓冲区的最大字节范围。
        //          maxPushConstantsSize：表示推送常量的最大字节大小。
        //          maxMemoryAllocationCount：表示一个逻辑设备可以同时分配的最大内存数。
        //          maxSamplerAllocationCount：表示一个逻辑设备可以同时分配的最大采样器数。
        //          bufferImageGranularity：表示缓冲区和图像之间的内存对齐粒度。
        //          sparseAddressSpaceSize：表示稀疏资源的地址空间大小。
        //          maxBoundDescriptorSets：表示一个管线布局可以绑定的最大描述符集数。
        //          maxPerStageDescriptorSamplers：表示一个着色器阶段可以访问的最大采样器数。
        //          maxPerStageDescriptorUniformBuffers：表示一个着色器阶段可以访问的最大统一缓冲区数。
        //          maxPerStageDescriptorStorageBuffers：表示一个着色器阶段可以访问的最大存储缓冲区数。
        //          maxPerStageDescriptorSampledImages：表示一个着色器阶段可以访问的最大采样图像数。
        //          maxPerStageDescriptorStorageImages：表示一个着色器阶段可以访问的最大存储图像数。
        //          maxPerStageDescriptorInputAttachments：表示一个着色器阶段可以访问的最大输入附件数。
        //          maxPerStageResources：表示一个着色器阶段可以访问的最大资源总数。
        //          maxDescriptorSetSamplers：表示一个描述符集可以包含的最大采样器数。
        //          maxDescriptorSetUniformBuffers：表示一个描述符集可以包含的最大统一缓冲区数。
        //          maxDescriptorSetUniformBuffersDynamic：表示一个描述符集可以包含的最大动态统一缓冲区数。
        //          maxDescriptorSetStorageBuffers：表示一个描述符集可以包含的最大存储缓冲区数。
        //          maxDescriptorSetStorageBuffersDynamic：表示一个描述符集可以包含的最大动态存储缓冲区数。
        //          maxDescriptorSetSampledImages：表示一个描述符集可以包含的最大采样图像数。
        //          maxDescriptorSetStorageImages：表示一个描述符集可以包含的最大存储图像数。
        //          maxDescriptorSetInputAttachments：表示一个描述符集可以包含的最大输入附件数。
        //          maxVertexInputAttributes：表示顶点输入属性的最大数量。
        //          maxVertexInputBindings：表示顶点输入绑定的最大数量。
        //          maxVertexInputAttributeOffset：表示顶点输入属性偏移量的最大值。
        //          maxVertexInputBindingStride：表示顶点输入绑定步幅的最大值。
        //          maxVertexOutputComponents：表示顶点着色器输出组件的最大数量。
        //  sparseProperties：表示物理设备的稀疏相关属性，是一个VkPhysicalDeviceSparseProperties结构体。
        VkPhysicalDeviceProperties properties{};// 定义一个用于存储物理设备属性的结构体
        vkGetPhysicalDeviceProperties(physicalDevice, &properties); // 获取物理设备的属性，比如最大各向异性（anisotropy）值

        // 结构体VkSamplerCreateInfo是用来描述采样器对象的创建信息的，它有以下成员变量1：
        //  sType：表示结构体的类型，必须是VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO。
        //  pNext：表示指向扩展结构体的指针，或者是NULL。
        //  flags：表示采样器的额外参数的位掩码，目前没有定义任何标志位，所以必须是0。
        //  magFilter：表示放大过滤方式，可以是VK_FILTER_NEAREST或VK_FILTER_LINEAR，表示当纹理被放大时，使用最近的纹素或相邻纹素的加权平均值。
        //  minFilter：表示缩小过滤方式，可以是VK_FILTER_NEAREST或VK_FILTER_LINEAR，表示当纹理被缩小时，使用最近的纹素或相邻纹素的加权平均值。
        //      VkFilter是一个枚举类型，用于指定纹理采样时的过滤方式。它有三种可能的值1：
        //      VK_FILTER_NEAREST：使用最近邻插值，即选择最接近纹理坐标的纹素。
        //      VK_FILTER_LINEAR：使用线性插值，即根据纹理坐标周围的四个纹素的加权平均值。
        //      VK_FILTER_CUBIC_IMG：使用三次样条插值，即根据纹理坐标周围的十六个纹素的加权平均值。
        //          这个值是一个可选的扩展，只有在VK_IMG_filter_cubic扩展被支持时才有效。
        //  mipmapMode：表示多级渐远纹理过滤方式，可以是VK_SAMPLER_MIPMAP_MODE_NEAREST或VK_SAMPLER_MIPMAP_MODE_LINEAR，表示当从多个mipmap级别中采样时，使用最近的级别或相邻级别的加权平均值。
        //      VkSamplerMipmapMode是一个枚举类型，用于指定多级渐远纹理采样时的过滤方式。它有两种可能的值1：
        //      VK_SAMPLER_MIPMAP_MODE_NEAREST：使用最近邻插值，即选择最接近LOD值的mipmap级别。
        //      VK_SAMPLER_MIPMAP_MODE_LINEAR：使用线性插值，即根据LOD值在两个最接近的mipmap级别之间进行混合。
        //  addressModeU：表示水平方向的寻址模式，可以是VK_SAMPLER_ADDRESS_MODE_REPEAT、VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT、VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE、VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER、VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE等，表示当纹理坐标超出[0, 1]范围时，如何处理纹理边界。
        //  addressModeV：表示垂直方向的寻址模式，与addressModeU类似。
        //  addressModeW：表示深度方向的寻址模式，与addressModeU类似。
        //      VkSamplerAddressMode是一个枚举类型，用于指定纹理采样时的寻址模式。它有六种可能的值1：
        //      VK_SAMPLER_ADDRESS_MODE_REPEAT：重复纹理，即当纹理坐标超出[0, 1)范围时，将其映射到该范围内的相应位置。
        //      VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT：镜像重复纹理，即当纹理坐标超出[0, 1)范围时，将其映射到该范围内的相反位置。
        //      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE：截取到边缘，即当纹理坐标超出[0, 1)范围时，将其设置为该范围内的最近边缘值。
        //      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER：截取到边界，即当纹理坐标超出[0, 1)范围时，将其设置为预定义的边界颜色。
        //      VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE：镜像截取到边缘，即当纹理坐标超出[0, 1)范围时，将其设置为该范围内的最近边缘值的相反值。
        //      VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE_KHR：与VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE相同，但是这个值是一个可选的扩展，只有在VK_KHR_sampler_mirror_clamp_to_edge扩展被支持时才有效。
        //  mipLodBias：表示用于添加到mipmap LOD（细节层次）计算中的偏置值，可以使纹理看起来更清晰或更模糊。
        //  anisotropyEnable：表示是否启用各向异性过滤，可以是VK_TRUE或VK_FALSE，各向异性过滤是一种高质量的过滤方式，可以减少倾斜视角下的模糊和锯齿。
        //  maxAnisotropy：表示在启用各向异性过滤时，采样器使用的最大各向异性值，必须在1.0到物理设备支持的最大值之间。
        //  compareEnable：表示是否启用比较操作，可以是VK_TRUE或VK_FALSE，比较操作可以对采样结果进行深度比较，用于实现阴影等效果。
        //  compareOp：表示在启用比较操作时，采样器使用的比较操作符，表示如何比较采样结果和参考值。
        //      VkCompareOp是一个枚举类型，用于指定深度或模板测试时的比较操作。它有八种可能的值：
        //      VK_COMPARE_OP_NEVER：永远返回false。
        //      VK_COMPARE_OP_LESS：当输入值小于参考值时返回true。
        //      VK_COMPARE_OP_EQUAL：当输入值等于参考值时返回true。
        //      VK_COMPARE_OP_LESS_OR_EQUAL：当输入值小于或等于参考值时返回true。
        //      VK_COMPARE_OP_GREATER：当输入值大于参考值时返回true。
        //      VK_COMPARE_OP_NOT_EQUAL：当输入值不等于参考值时返回true。
        //      VK_COMPARE_OP_GREATER_OR_EQUAL：当输入值大于或等于参考值时返回true。
        //      VK_COMPARE_OP_ALWAYS：永远返回true。
        //  minLod：表示用于对计算得到的LOD值的最小值进行限制，可以避免纹理过于模糊。
        //  maxLod：表示用于对计算得到的LOD值的最大值进行限制，可以避免纹理过于清晰。为了避免限制最大值，可以把maxLod设置为常量VK_LOD_CLAMP_NONE。
        //  borderColor：表示当寻址模式为边界模式时，采样器使用的预定义边界颜色，
        //      VkBorderColor是一个枚举类型，用于指定采样器截取到边界时的预定义颜色。它有八种可能的值2：
        //      VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK：表示(0.0, 0.0, 0.0, 0.0)的浮点颜色。
        //      VK_BORDER_COLOR_INT_TRANSPARENT_BLACK：表示(0, 0, 0, 0)的整数颜色。
        //      VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK：表示(0.0, 0.0, 0.0, 1.0)的浮点颜色。
        //      VK_BORDER_COLOR_INT_OPAQUE_BLACK：表示(0, 0, 0, 1)的整数颜色。
        //      VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE：表示(1.0, 1.0, 1.0, 1.0)的浮点颜色。
        //      VK_BORDER_COLOR_INT_OPAQUE_WHITE：表示(1, 1, 1, 1)的整数颜色。
        //      VK_BORDER_COLOR_FLOAT_CUSTOM_EXT：表示一个自定义的浮点颜色，需要使用VkSamplerCustomBorderColorCreateInfoEXT结构来指定。这个值是一个可选的扩展，只有在VK_EXT_custom_border_color扩展被支持时才有效。
        //      VK_BORDER_COLOR_INT_CUSTOM_EXT：表示一个自定义的整数颜色，需要使用VkSamplerCustomBorderColorCreateInfoEXT结构来指定。这个值是一个可选的扩展，只有在VK_EXT_custom_border_color扩展被支持时才有效。
        //  unnormalizedCoordinates：表示是否使用非归一化坐标来寻址纹理，可以是VK_TRUE或VK_FALSE，非归一化坐标的范围是0到图像的大小，而不是0到1。
        // 定义一个用于描述纹理采样器创建信息的结构体
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;          // 设置结构体的类型，必须是VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
        samplerInfo.magFilter = VK_FILTER_LINEAR;                           // 设置放大过滤方式（magFilter）为线性过滤（linear filtering），表示当纹理被放大时，使用相邻像素的加权平均值
        samplerInfo.minFilter = VK_FILTER_LINEAR;                           // 设置缩小过滤方式（minFilter）为线性过滤，表示当纹理被缩小时，使用相邻像素的加权平均值
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;             // 设置多级渐远纹理过滤方式（mipmap mode）为线性过滤，表示当从多个mipmap级别中采样时，使用相邻级别的加权平均值
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;          // 设置水平方向的寻址模式（addressModeU）为重复模式（repeat mode），表示当纹理坐标超出[0, 1]范围时，重复纹理图案
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;          // 设置垂直方向的寻址模式（addressModeV）为重复模式，表示当纹理坐标超出[0, 1]范围时，重复纹理图案
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;          // 设置深度方向的寻址模式（addressModeW）为重复模式，表示当纹理坐标超出[0, 1]范围时，重复纹理图案
        samplerInfo.anisotropyEnable = VK_TRUE;                             // 设置各向异性过滤（anisotropic filtering）为启用，表示使用一种更高质量的过滤方式，可以减少倾斜视角下的模糊和锯齿
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // 设置最大各向异性值为物理设备支持的最大值，表示使用最高程度的各向异性过滤
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;         // 设置边界颜色（borderColor）为不透明黑色（opaque black），表示当寻址模式为边界模式（border mode）时，使用黑色填充超出范围的像素
        samplerInfo.compareEnable = VK_FALSE;                               // 设置是否启用比较操作（compare operation）为否，表示不对采样结果进行深度比较
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;                       // 设置比较操作符（compare operator）为始终通过（always pass），表示不对采样结果进行深度比较
        samplerInfo.unnormalizedCoordinates = VK_FALSE;                     // 设置是否使用非归一化坐标（unnormalized coordinates）为否，表示使用[0, 1]范围内的归一化坐标来寻址纹理

        // 调用创建纹理采样器的函数，传入创建信息结构体和空指针作为分配回调函数，返回一个纹理采样器对象
        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            // 如果创建失败，抛出一个运行时错误
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

    // 定义一个函数，用于创建一个图像对象和一个设备内存对象。函数的参数分别是：
    // width：图像的宽度，height：图像的高度，format：图像的格式，tiling：图像的排列方式
    // usage：图像的用途，properties：设备内存的属性
    // image：引用类型的参数，接收创建的图像对象，imageMemory：引用类型的参数，接收创建的设备内存对象
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        // 定义一个结构体变量VkImageCreateInfo，用于指定图像对象的创建信息，各个成员的含义如下：
        //  sType是一个VkStructureType值，表示这个结构体的类型。必为VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO。
        //  pNext是一个指针，指向一个扩展结构体，用于提供额外的参数。
        //  flags是一个位域（bit field），表示图像对象的附加属性，例如是否立方体兼容（cube compatible）、是否稀疏绑定（sparse binding）等。
        //  imageType是一个VkImageType值，表示图像对象的基本维度，例如一维（1D）、二维（2D）或三维（3D）。
        //  format是一个VkFormat值，表示图像对象的格式和类型，例如RGBA、深度模板（depth stencil）等。
        //  extent是一个VkExtent3D结构体，表示图像对象的尺寸，包括宽度、高度和深度。
        //  mipLevels是一个整数，表示图像对象的多级渐远纹理（mipmap）层数。
        //  arrayLayers是一个整数，表示图像对象的“数组纹理”的层数。
        //  samples是一个VkSampleCountFlagBits值，表示图像对象的每个纹素（texel）的采样点数，用于实现多重采样抗锯齿，图形管线那里有介绍。
        //  tiling是一个VkImageTiling值，表示图像对象在内存中的排列方式，例如线性（linear）或最佳（optimal）。
        //      VK_IMAGE_TILING_OPTIMAL表示图像数据按照最佳方式排列，方便设备访问
        //          适用于那些需要高效地被设备（GPU）访问的图像对象，例如用于渲染、采样、存储等操作的图像对象。
        //          通常可以支持更多的图像用途和格式，但是不能直接被主机（CPU）映射或复制。
        //      VK_IMAGE_TILING_LINEAR表示图像数据按照线性方式排列，方便主机访问
        //          适用于那些需要直接被主机访问或复制的图像对象，例如用于从主机读取或写入数据的图像对象。
        //          通常只能支持传输源或传输目标的用途，而且对格式有限制。
        //      VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT：表示图像的排列方式由一个Linux DRM格式修饰符定义，这是一个扩展值，需要支持VK_EXT_image_drm_format_modifier扩展
        //  usage是一个位域，表示图像对象的用途，例如传输源（transfer source）、传输目标（transfer destination）、采样源（sampled source）等。顶点缓冲区有介绍
        //  sharingMode是一个VkSharingMode值，表示图像对象在多个队列族（queue family）之间的共享模式，例如独占（exclusive）或并发（concurrent）。交换链创建那有介绍
        //  queueFamilyIndexCount是一个整数，表示共享图像对象的队列族的数量。
        //  pQueueFamilyIndices是一个指针，指向一个整数数组，表示共享图像对象的队列族的索引。
        //  initialLayout是一个VkImageLayout值，表示图像对象的初始布局，例如未定义（undefined）、传输目标最佳（transfer destination optimal）等。
        //      VkImageLayout是一个枚举类型，用于指定图像对象和图像子资源的布局1。布局决定了图像数据在内存中的排列方式，以及图像可以被哪些操作访问2。VkImageLayout有以下可能的值：
        //          VK_IMAGE_LAYOUT_UNDEFINED：表示图像的布局是未定义的，即不关心图像的内容。
        //              这个值通常用于创建图像时指定初始布局，或者转换图像时指定源布局。
        //          VK_IMAGE_LAYOUT_GENERAL：表示图像的布局是通用的，
        //              即可以被任何操作访问，但是性能可能不是最佳的。这个值通常用于那些没有特定用途或格式的图像。
        //          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL：表示图像的布局是颜色附件最佳的，即适合作为渲染通道（render pass）的颜色附件。
        //              这个值通常用于那些用于渲染输出的图像。
        //          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL：表示图像的布局是深度模板附件最佳的，即适合作为渲染通道的深度模板附件。
        //              这个值通常用于那些用于深度测试或模板测试的图像。
        //          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL：表示图像的布局是深度模板只读最佳的，即适合作为只读深度模板附件或采样源。
        //              这个值通常用于那些用于阴影贴图或其他只读深度模板操作的图像。
        //          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL：表示图像的布局是着色器只读最佳的，即适合作为着色器操作或输入附件操作的源。
        //              这个值通常用于那些用于采样、存储或其他只读着色器操作的图像。
        //          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL：表示图像的布局是传输源最佳的，即适合作为传输操作（transfer operation）的源。
        //              这个值通常用于那些用于复制、填充或清除操作的源图像。
        //          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL：表示图像的布局是传输目标最佳的，即适合作为传输操作的目标。
        //              这个值通常用于那些用于复制、填充或清除操作的目标图像。
        //          VK_IMAGE_LAYOUT_PREINITIALIZED：表示图像已经被主机初始化过，即主机已经写入了一些数据到图像中。
        //              这个值通常用于创建图像时指定初始布局，以避免不必要地清除图像内容。
        //          VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL：表示图像的布局是深度只读模板附件最佳的，
        //              即适合作为只读深度附件和可写模板附件。这个值通常用于那些需要同时访问深度和模板数据的渲染通道。
        //          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL：表示图像的布局是深度附件模板只读最佳的，
        //              即适合作为可写深度附件和只读模板附件。这个值通常用于那些需要同时访问深度和模板数据的渲染通道。
        //          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL：表示图像的布局是深度附件最佳的，即适合作为可写深度附件。
        //              这个值通常用于那些只需要访问深度数据而不需要访问模板数据的渲染通道。
        //          VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL：表示图像的布局是深度只读最佳的，即适合作为只读深度附件或采样源。
        //              这个值通常用于那些只需要访问深度数据而不需要访问模板数据的渲染通道或着色器操作。
        //          VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL：表示图像的布局是模板附件最佳的，即适合作为可写模板附件。
        //              这个值通常用于那些只需要访问模板数据而不需要访问深度数据的渲染通道。
        //          VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL：表示图像的布局是模板只读最佳的，即适合作为只读模板附件或采样源。
        //              这个值通常用于那些只需要访问模板数据而不需要访问深度数据的渲染通道或着色器操作。
        //          VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL：表示图像的布局是只读最佳的，即适合作为只读附件或采样源。
        //              这个值通常用于那些不需要写入图像数据的渲染通道或着色器操作。
        //          VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL：表示图像的布局是附件最佳的，即适合作为可写附件。
        //              这个值通常用于那些需要写入图像数据的渲染通道。
        //          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR：表示图像的布局是呈现源的，即适合作为交换链（swapchain）的呈现源。
        //              这个值通常用于那些用于显示输出的图像。
        //          VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR：表示图像的布局是视频解码目标的，即适合作为视频解码操作（video decode operation）的目标。
        //              这个值通常用于那些用于视频解码输出的图像。
        //          VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR：表示图像的布局是视频解码源的，即适合作为视频解码操作的源。
        //              这个值通常用于那些用于视频解码输入的图像。
        //          VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR：表示图像的布局是视频解码DPB（decoded picture buffer）的，即适合作为视频解码操作的DPB。
        //              这个值通常用于那些用于存储视频解码过程中参考帧（reference frame）的图像。
        //          VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR：表示图像可以被多个设备共享呈现，即适合作为共享呈现操作（shared present operation）的源。
        //              这个值通常用于那些用于跨设备显示输出的图像。
        //          VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT：表示图像的布局是片段密度贴图最佳（fragment density map optimal）的，即适合作为片段密度贴图（fragment density map）操作的源。
        //              这个值通常用于那些用于存储片段密度信息（fragment density information）的图像。
        //          VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR：表示图像的布局是片段着色率附件最佳（fragment shading rate attachment optimal）的，即适合作为片段着色率附件（fragment shading rate attachment）操作的源。
        //              这个值通常用于那些用于存储片段着色率信息（fragment shading rate information）的图像。
        //          VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR：表示图像的布局是视频编码目标（video encode destination）的，即适合作为视频编码操作（video encode operation）的目标。
        //              这个值通常用于那些用于视频编码输出的图像。
        //          VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR：表示图像的布局是视频编码源（video encode source）的，即适合作为视频编码操作（video encode operation）的源。
        //              这个值通常用于那些用于视频编码输入或参考帧（reference frame）的图像。
        //          VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR：表示图像的布局是视频编码DPB（decoded picture buffer） 的，即适合作为视频编码操作（video encode operation） 的DPB。
        // 
        //          采样是一种从图像对象中获取像素值的操作，通常在着色器中使用图像采样器（image sampler）来进行。
        //              采样的目的是为了将图像数据转换为着色器可以使用的颜色值，以便于渲染或计算。
        //          传输是一种将数据从一个内存对象复制到另一个内存对象的操作，通常在命令缓冲区中使用复制命令（copy command）来进行。
        //              传输的目的是为了将数据从CPU内存移动到GPU内存，或者在GPU内存中进行重新排列或转换。
        //              ...
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = format;                              // 设置结构体变量中format成员为函数参数中的format，表示图像的格式
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;                                // 设置结构体变量中mipLevels成员为1，表示只有一级多级渐远纹理（mipmap）
        imageInfo.arrayLayers = 1;                              
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = tiling;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;      // 设置结构体变量中sharingMode成员为VK_SHARING_MODE_EXCLUSIVE，表示图像只能被一个队列族（queue family）拥有
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;    // 设置结构体变量中initialLayout成员为VK_IMAGE_LAYOUT_UNDEFINED，表示图像的初始布局是未定义的，即不关心图像的内容
        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) { // 创建与纹理对应的一个图像对象。
            throw std::runtime_error("failed to create image!");
        }
        
        VkMemoryRequirements memRequirements;                   // 定义一个结构体变量，用于存储图像对象的内存需求信息
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;        // 设置结构体变量中allocationSize成员为memRequirements.size，表示要分配的设备内存的大小等于图像对象的内存需求大小
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);// findMemoryType函数是一个自定义的函数，用于根据给定的内存类型位域和属性找到合适的内存类型索引

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);       // 调用Vulkan API函数，将设备内存对象绑定到图像对象上
    }

    // 定义一个改变图像布局的函数
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();      // 创建一个临时的命令缓冲区

        // 结构体VkImageMemoryBarrier是用于指定一个图像内存屏障（image memory barrier）的参数。
        //  图像内存屏障是一种用于同步图像对象的访问和布局的机制。结构体VkImageMemoryBarrier包含以下成员：
        //      sType：表示结构体的类型，必须是VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER。
        //      pNext：表示指向扩展结构体的指针，如果没有扩展，可以为NULL。
        //      srcAccessMask：表示源访问掩码（source access mask），是一个位掩码，指定在屏障之前，哪些类型的操作可以访问图像对象。
        //      dstAccessMask：表示目标访问掩码（destination access mask），是一个位掩码，指定在屏障之后，哪些类型的操作可以访问图像对象。
        //          VK_ACCESS_INDIRECT_COMMAND_READ_BIT：表示可以读取间接命令（indirect command）的缓冲区（buffer）。
        //          VK_ACCESS_INDEX_READ_BIT：表示可以读取索引（index）的缓冲区。
        //          VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT：表示可以读取顶点属性（vertex attribute）的缓冲区。
        //          VK_ACCESS_UNIFORM_READ_BIT：表示可以读取统一变量（uniform）的缓冲区或图像（image）。
        //          VK_ACCESS_INPUT_ATTACHMENT_READ_BIT：表示可以读取输入附件（input attachment）的图像。
        //          VK_ACCESS_SHADER_READ_BIT：表示可以读取着色器（shader）的缓冲区或图像。
        //          VK_ACCESS_SHADER_WRITE_BIT：表示可以写入着色器的缓冲区或图像。
        //          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT：表示可以读取颜色附件（color attachment）的图像。
        //          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT：表示可以写入颜色附件的图像。
        //          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT：表示可以读取深度模板附件（depth stencil attachment）的图像。
        //          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT：表示可以写入深度模板附件的图像。
        //          VK_ACCESS_TRANSFER_READ_BIT：表示可以读取传输操作（transfer operation）的缓冲区或图像。
        //          VK_ACCESS_TRANSFER_WRITE_BIT：表示可以写入传输操作的缓冲区或图像。
        //          VK_ACCESS_HOST_READ_BIT：表示可以由主机（host）读取的内存对象。
        //          VK_ACCESS_HOST_WRITE_BIT：表示可以由主机写入的内存对象。
        //          VK_ACCESS_MEMORY_READ_BIT：表示可以由任何操作读取的内存对象。
        //          VK_ACCESS_MEMORY_WRITE_BIT：表示可以由任何操作写入的内存对象。
        //      oldLayout：表示源布局（old layout），是一个枚举值，指定在屏障之前，图像对象的布局。。
        //      newLayout：表示目标布局（new layout），是一个枚举值，指定在屏障之后，图像对象的布局。
        //      srcQueueFamilyIndex：表示源队列族索引（source queue family index），是一个整数，指定在屏障之前，哪个队列族（queue family）拥有图像对象的访问权。如果不需要队列族所有权转移（queue family ownership transfer），可以设置为VK_QUEUE_FAMILY_IGNORED。
        //      dstQueueFamilyIndex：表示目标队列族索引（destination queue family index），是一个整数，指定在屏障之后，哪个队列族拥有图像对象的访问权。如果不需要队列族所有权转移，可以设置为VK_QUEUE_FAMILY_IGNORED。
        //      image：表示受到屏障影响的图像对象（image object），是一个句柄（handle）。
        //      subresourceRange：表示受到屏障影响的图像子资源范围（image subresource range），是一个结构体，指定图像对象中的哪些方面(多级渐远纹理层级、数组层级)需要同步。
        VkImageMemoryBarrier barrier{};                                 // 定义一个图像内存屏障结构体
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;         // 设置结构体类型
        barrier.oldLayout = oldLayout;                                  // 设置源布局
        barrier.newLayout = newLayout;                                  // 设置目标布局
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;          // 设置源队列族索引，忽略表示不需要队列族所有权转移
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;          // 设置目标队列族索引，忽略表示不需要队列族所有权转移
        barrier.image = image;                                          // 设置要改变布局的图像对象
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;                      // 设置要改变布局的子资源范围，这里只改变颜色方面，第0层级，第0数组层
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        // VkPipelineStageFlags有28种值。它们是用来表示管线阶段掩码（pipeline stage mask）的
        //    指定同步范围（synchronization scope）包含的管线阶段（pipeline stage）。它们可以是以下枚举值（enum values）的组合：
        //    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT：表示管线开始阶段，不执行任何操作。
        //    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT：表示绘制间接阶段，执行间接绘制命令。
        //    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT：表示顶点输入阶段，读取顶点和索引数据。
        //    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT：表示顶点着色器阶段，执行顶点着色器程序。
        //    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT：表示曲面细分控制着色器阶段，执行曲面细分控制着色器程序。
        //    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT：表示曲面细分评估着色器阶段，执行曲面细分评估着色器程序。
        //    VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT：表示几何着色器阶段，执行几何着色器程序。
        //    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT：表示片段着色器阶段，执行片段着色器程序。
        //    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT：表示早期片段测试阶段，执行深度和模板测试。
        //    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT：表示晚期片段测试阶段，执行深度和模板测试。
        //    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT：表示颜色附件输出阶段，写入颜色附件。
        //    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT：表示计算着色器阶段，执行计算着色器程序。
        //    VK_PIPELINE_STAGE_TRANSFER_BIT：表示传输阶段，执行传输操作。
        //    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT：表示管线结束阶段，不执行任何操作。
        //    VK_PIPELINE_STAGE_HOST_BIT：表示主机阶段，由主机访问内存对象。
        //    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT：表示所有图形阶段，包括顶点输入、顶点着色器、曲面细分着色器、几何着色器、片段着色器、片段测试和颜色附件输出等。
        //    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT：表示所有命令阶段，包括所有图形阶段、计算着色器阶段和传输阶段等。
        //    除了这些基本的管线阶段掩码值，还有一些扩展的管线阶段掩码值，它们是由一些特定的扩展或功能提供的。它们可以与基本的管线阶段掩码值一起使用，以实现更细粒度的同步。它们有以下枚举值：
        //
        //    VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT：表示条件渲染阶段，由VK_EXT_conditional_rendering扩展提供，执行条件渲染操作。
        //    VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT：表示变换反馈阶段，由VK_EXT_transform_feedback扩展提供，执行变换反馈操作。
        //    VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV：表示命令预处理阶段，由VK_NV_device_generated_commands扩展提供，执行命令预处理操作。
        //    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR：表示加速结构构建阶段，由VK_KHR_acceleration_structure扩展提供，执行加速结构构建操作。
        //    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR：表示光线追踪着色器阶段，由VK_KHR_ray_tracing_pipeline扩展提供，执行光线追踪着色器程序。
        //    VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV：表示着色率图像阶段，由VK_NV_shading_rate_image扩展提供，读取着色率图像数据。
        //    VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV：表示任务着色器阶段，由VK_NV_mesh_shader扩展提供，执行任务着色器程序。
        //    VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV：表示网格着色器阶段，由VK_NV_mesh_shader扩展提供，执行网格着色器程序。
        //    VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT：表示片段密度处理阶段，由VK_EXT_fragment_density_map扩展提供，读取片段密度图像数据。
        VkPipelineStageFlags sourceStage;  // 定义源管线阶段和目标管线阶段的标志
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            // 如果源布局是未定义的，目标布局是传输目标最佳的，那么在屏障之前不需要任何访问掩码，之后需要传输写入访问掩码
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;        // 源管线阶段是管线开始阶段
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;      // 目标管线阶段是传输阶段
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            // 如果源布局是传输目标最佳的，目标布局是着色器只读最佳的，那么在屏障之前需要传输写入访问掩码，之后需要着色器读取访问掩码
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;   // 源管线阶段是传输阶段
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;      // 目标管线阶段是片段着色器阶段

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            // 如果源布局和目标布局不是上述两种情况，那么抛出一个异常，表示不支持这种布局转换
            throw std::invalid_argument("unsupported layout transition!");
        }

        // 函数vkCmdPipelineBarrier是用于在命令缓冲区（command buffer）中记录一个管线屏障（pipeline barrier）命令的。
        //      管线屏障命令是一种用于定义内存依赖性（memory dependency）和布局转换（layout transition）的命令5。函数vkCmdPipelineBarrier包含以下参数：
        //  commandBuffer：表示要记录命令的命令缓冲区。
        //      srcStageMask：表示源管线阶段掩码（source pipeline stage mask），是一个位掩码，指定第一个同步范围（synchronization scope）包含的管线阶段（pipeline stage）。
        //      dstStageMask：表示目标管线阶段掩码（destination pipeline stage mask），是一个位掩码，指定第二个同步范围包含的管线阶段。
        //      dependencyFlags：表示依赖性标志（dependency flags），是一个位掩码，指定如何形成执行依赖性（execution dependency）和内存依赖性。
        //      memoryBarrierCount：表示内存屏障数组的长度。
        //      pMemoryBarriers：表示指向内存屏障数组的指针。内存屏障用于同步任意类型数据的访问6。
        //      bufferMemoryBarrierCount：表示缓冲区内存屏障数组的长度。
        //      pBufferMemoryBarriers：表示指向缓冲区内存屏障数组的指针。缓冲区内存屏障用于同步缓冲区对象（buffer object）的访问7。
        //      imageMemoryBarrierCount：表示图像内存屏障数组的长度。
        //      pImageMemoryBarriers：表示指向图像内存屏障数组的指针。图像内存屏障用于同步图像对象的访问和布局8。
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

    // 定义一个用于复制数据的函数，参数包括源缓冲区对象，目标图像对象，以及图像的宽度和高度
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        // 结构体VkBufferImageCopy是用于描述缓冲区和图像之间复制区域的参数。它包含以下字段：
        //     bufferOffset：表示缓冲区中数据的偏移量，单位是字节。
        //     bufferRowLength：表示缓冲区中每行数据的长度，单位是像素。如果为0，表示使用图像的宽度。
        //     bufferImageHeight：表示缓冲区中每个图像层的高度，单位是像素。如果为0，表示使用图像的高度。
        //     imageSubresource：表示图像子资源的方面、mipmap级别和数组层。
        //     imageOffset：表示图像中数据的偏移量，单位是像素。
        //     imageExtent：表示图像中数据的范围，单位是像素。
        // 定义一个用于描述缓冲区和图像之间复制区域的结构体
        VkBufferImageCopy region{};
        region.bufferOffset = 0;                                        // 设置缓冲区中数据的偏移量为0
        region.bufferRowLength = 0;                                     // 设置缓冲区中每行数据的长度为0，表示使用图像的宽度
        region.bufferImageHeight = 0;                                   // 设置缓冲区中每个图像层的高度为0，表示使用图像的高度

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // 设置图像子资源的方面掩码为颜色方面
        region.imageSubresource.mipLevel = 0;                           // 设置图像子资源的mipmap级别为0，表示最高分辨率的级别
        region.imageSubresource.baseArrayLayer = 0;                     // 设置图像子资源的最低数组层为0
        region.imageSubresource.layerCount = 1;                         // 设置图像子资源的数组层数为1，表示只有一层

        region.imageOffset = { 0, 0, 0 };                               // 设置图像中数据的偏移量为(0, 0, 0)，表示从左上角开始复制
        region.imageExtent = {                                          // 设置图像中数据的范围为(width, height, 1)，表示复制整个图像
            width,
            height,
            1
        };

        // 在命令缓冲区中记录一个复制命令，将缓冲区对象中的数据复制到图像对象中，假设图像对象已经处于传输目标优化布局
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
    // 有注释
    void createDescriptorPool() {
        // 描述符池有两个描述符，一个是变换矩阵的缓冲区，一个是纹理采样的采样器，数量分别是两个定义不同帧的uniform缓冲区与采样器
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

            // 更新描述对应的采样器，采样描述集为当前帧的描述集
            // 更新描述符为采样器类型，再描述集种绑定点为1，不是描述符数组，描述符数量为1；
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