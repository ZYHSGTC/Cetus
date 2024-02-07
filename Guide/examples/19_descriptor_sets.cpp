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

    // 描述符布局、表述符集与描述符之间的关系可以用下面的图示来表示：(Set 0有3个描述符，Set 1 有4个描述符
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
    //  描述符池储存着描述符，包含所有需要的描述符，描述符集是描述符池一些描述符的集合。
    //      这句话没有错，但是描述符池不一定包含所有需要的描述符，它只是一个分配描述符集的容器。
    //      描述符池的大小和类型需要在创建时指定，如果分配完了就不能再分配新的描述符集，除非清空或销毁描述符池。
    //  描述符集将描述符池的描述符抽出来按描述符布局组成一个描述符集合。
    //      描述符池初始化，分别指定每种描述符的数量，再指定这些所有的描述符会构成多少个描述符集。描述符集的并集就等于描述池中所有的描述符
    //      同一个描述符集不能有两种描述符对应同一个绑定点，不然着色器上会混乱。
    //      这句话也没有错，但是描述符集不是直接抽出来的，而是通过vkAllocateDescriptorSets函数从描述符池中分配出来的。
    //      描述符布局定义了每个描述符集中包含哪些类型和数量的描述符，以及它们在着色器中的绑定点。
    //  然后描述符集用于调用vkUpdateDescriptorSets，将描述符集中的一个绑定点(对应一个描述符(数组))的几个同类描述符与对应数量的统一缓冲区关联起来，使得着色器中可以使用统一缓冲区的数据
    //      这句话也没有错，但是vkUpdateDescriptorSets函数不仅可以关联统一缓冲区，还可以关联其他类型的资源，比如采样器、图像视图等。
    //      vkUpdateDescriptorSets函数通过VkWriteDescriptorSet结构体来指定要更新哪个描述符集中的哪个绑定点，以及要关联哪个资源对象和数据范围。
    //  描述符布局descriptorSetLayout包含了图形管线如何使用统一缓冲区，采样器、图像视图的所有信息。
    //      每个描述符集都对应着一个描述符布局，相同描述符布局描述符集可以有多个。
    // **************************************************************************************
    //  创建VkDescriptorSetLayout句柄的数组，指定管线中使用的描述符集布局。
    //      每个描述符集布局可以包含多个描述符绑定点，用于存储不同类型和数量的资源。
    //      VkDescriptorSetLayout创建所需要的信息VkDescriptorSetLayoutCreateInfo结构体:
    //      sType：一个VkStructureType值，用于标识这个结构体的类型，必须是VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO。
    //      pNext：一个指针，用于指向一个扩展这个结构体的其他结构体，或者为NULL。
    //      flags：一个位掩码，用于指定额外的描述符集布局信息，目前没有定义任何有效的值。
    //      bindingCount：一个整数，用于指定pBindings数组中的VkDescriptorSetLayoutBinding结构体的数量。
    //      pBindings：一个指针，用于指向一个VkDescriptorSetLayoutBinding结构体数组，定义了每个描述符绑定点的信息。
    //          VkDescriptorSetLayoutBinding结构体：
    //          binding：一个整数，用于指定这个绑定点的编号，它与着色器中使用的绑定号相对应。
    //          descriptorType：一个VkDescriptorType枚举值，用于指定这个绑定点的描述符类型，如缓冲、图像、采样器等。
    //              VkDescriptorType是一个枚举类型，用于指定描述符的类型。它有以下几种值3：
    //              VK_DESCRIPTOR_TYPE_SAMPLER：用于采样纹理的采样器
    //              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER：用于采样纹理的图像和采样器
    //              VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE：用于采样纹理的图像
    //              VK_DESCRIPTOR_TYPE_STORAGE_IMAGE：用于读写纹理的图像
    //              VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER：用于访问缓冲区中的纹素（texel）的缓冲区视图
    //              VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER：用于读写缓冲区中的纹素（texel）的缓冲区视图
    //              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER：用于访问缓冲区中的统一数据（uniform data）的缓冲区
    //              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER：用于读写缓冲区中的存储数据（storage data）的缓冲区
    //              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC：用于访问缓冲区中的统一数据（uniform data）的缓冲区，但是可以在绘制时动态地改变偏移量
    //              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC：用于读写缓冲区中的存储数据（storage data）的缓冲区，但是可以在绘制时动态地改变偏移量
    //              VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT：用于访问子通道输入附件（subpass input attachment）的图像
    //          descriptorCount：一个整数，用于指定这个绑定点的描述符数量，如果大于1，则表示这个绑定点是一个数组，可以在着色器中使用下标来访问。
    //              descriptorCount大于一表示一个绑定点对应多个描述符，这通常用于实现数组或向量等数据结构。
    //              在着色器中使用时，需要在layout修饰符中指定数组的大小，并用下标访问对应的元素。例如：
    //              在顶点着色器中定义一个统一缓冲区数组
    //              layout(binding = 0) uniform UniformBufferObject {
    //                  mat4 model;
    //                  mat4 view;
    //                  mat4 proj;
    //              } ubos[3];
    //              void main() {
    //                  gl_Position = ubos[1].proj * ubos[1].view * ubos[1].model * vec4(aPos, 1.0);
    //              }
    //          stageFlags：一个位掩码，用于指定这个绑定点可以被哪些着色器阶段访问，如顶点、片段、计算等。
    //              stageFlags是一个位域类型，用于指定描述符在哪些着色器阶段可用。它有以下几种值3：
    //              VK_SHADER_STAGE_VERTEX_BIT：顶点着色器阶段
    //              VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT：曲面细分控制着色器阶段
    //              VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT：曲面细分评估着色器阶段
    //              VK_SHADER_STAGE_GEOMETRY_BIT：几何着色器阶段
    //              VK_SHADER_STAGE_FRAGMENT_BIT：片段着色器阶段
    //              VK_SHADER_STAGE_COMPUTE_BIT：计算着色器阶段
    //              VK_SHADER_STAGE_ALL_GRAPHICS：所有图形管线相关的着色器阶段
    //              VK_SHADER_STAGE_ALL：所有着色器阶段
    //          pImmutableSamplers：一个指针，用于指定这个绑定点的不可变采样器数组，
    //              如果这个绑定点的描述符类型是采样器或者组合图像采样器，则可以使用这个字段来固定采样器对象，而不需要在描述符集中更新。
    // 创建描述符集布局的函数
    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};                    // 创建一个描述符集布局绑定结构体
        uboLayoutBinding.binding = 0;                                       // 设置绑定点为0
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;// 设置描述符类型为统一缓冲区
        uboLayoutBinding.descriptorCount = 1;                               // 设置描述符数量为1
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;           // 设置着色器阶段标志为顶点阶段
        uboLayoutBinding.pImmutableSamplers = nullptr;                      // 设置不可变采样器为空指针

        VkDescriptorSetLayoutCreateInfo layoutInfo{};                       // 创建一个描述符集布局创建信息结构体
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;                                        // 设置绑定数量为1
        layoutInfo.pBindings = &uboLayoutBinding;                           // 设置绑定指针为上面创建的绑定结构体

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            // 调用Vulkan API函数，创建描述符集布局，如果创建失败，抛出运行时错误异常
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }


    // 有注释
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

        // VkPipelineLayoutCreateInfo结构体是用于配置管线布局相关的设置的一个结构体，它可以指定管线中使用的描述符集布局和推送常量范围。
        //  sType：一个VkStructureType值，用于标识这个结构体的类型，必须是VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO。
        //  pNext：一个指针，用于指向一个扩展这个结构体的其他结构体，或者为NULL。
        //  flags：一个位掩码，用于指定额外的管线布局信息，目前没有定义任何有效的值，必须为0。
        //  setLayoutCount：一个整数，用于指定pSetLayouts数组中的VkDescriptorSetLayout值的数量。
        //  pSetLayouts：一个指针，用于指向一个VkDescriptorSetLayout句柄的数组，指定管线中使用的描述符集布局。
        //      每个描述符集布局可以包含多个描述符绑定点，用于存储不同类型和数量的资源。
        //  pushConstantRangeCount：一个整数，用于指定VkPushConstantRange结构体的数量。这里没有设定；
        //  pPushConstantRanges：一个指针，用于指向一个VkPushConstantRange结构体数组，指定管线中使用的推送常量范围。
        //      每个推送常量范围可以定义一段内存空间，用于存储一些常量数据，可以在运行时通过vkCmdPushConstants命令来更新。
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

    void createUniformBuffers() {                               // 创建统一缓冲区的函数
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);  // 获取统一缓冲区对象的大小
        // 如果只有一个统一缓冲区，那么每一帧都需要等待上一帧使用该缓冲区的命令(渲染过程-->)执行完毕，才能更新该同一缓冲区。
        // 也就是变成了跟单缓冲一样的速度：
        // 双帧数：(上下交替，双缓冲)(渲染过程-->)(更新缓冲呈递命令过程->)
        //  ->-->t      ->-->t...
        //        ->-->t      ->-->t...
        // 为了避免这种情况，每个缓冲区对应一个帧，这样可以实现双缓冲。
        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);      // 统一缓冲区的内存数据指针

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {     // 遍历每一帧
            // 定义主机可见与主机同步一致的同一缓冲区
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
            // 获得同一缓冲区的指针uniformBuffersMapped[i]
            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }
    // 创建一个描述符池（descriptor pool），用于分配描述符集（descriptor set）
    // 描述符池（descriptor pool）是一种用于分配和管理描述符集（descriptor set）的对象。
    // 描述符集是一种用于存储一组描述符（descriptor）的对象，描述符是一种用于表示缓冲区（buffer）或图像（image）等资源的对象。
    void createDescriptorPool() {   
        // VkDescriptorPoolSize结构体用于指定描述符池（descriptor pool）中每种类型的描述符的数量。它包含以下成员：
        //  type：一个VkDescriptorType枚举值，表示描述符的类型，
        //      VkDescriptorType是一个枚举类型，用于指定描述符的类型。它有以下几种值3：
        //      VK_DESCRIPTOR_TYPE_SAMPLER：用于采样纹理的采样器
        //      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER：用于采样纹理的图像和采样器
        //      VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE：用于采样纹理的图像
        //      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE：用于读写纹理的图像
        //      VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER：用于访问缓冲区中的纹素（texel）的缓冲区视图
        //      VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER：用于读写缓冲区中的纹素（texel）的缓冲区视图
        //      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER：用于访问缓冲区中的统一数据（uniform data）的缓冲区
        //      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER：用于读写缓冲区中的存储数据（storage data）的缓冲区
        //      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC：用于访问缓冲区中的统一数据（uniform data）的缓冲区，但是可以在绘制时动态地改变偏移量
        //      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC：用于读写缓冲区中的存储数据（storage data）的缓冲区，但是可以在绘制时动态地改变偏移量
        //      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT：用于访问子通道输入附件（subpass input attachment）的图像
        //  descriptorCount：一个uint32_t值，表示该类型的描述符的数量。
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        // VkDescriptorPoolCreateInfo结构体用于指定描述符池（descriptor pool）的属性2。它包含以下成员：
        //  sType：必须为VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO。
        //  pNext：一个void* 指针，表示指向扩展结构体的指针，或者为NULL。
        //  flags：一个VkDescriptorPoolCreateFlags位掩码，表示描述符池的创建标志。
        //      这些值是指VkDescriptorPoolCreateFlags位掩码，表示描述符池（descriptor pool）的创建标志1。它们有以下含义和作用：
        //      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT：
        //          表示允许从描述符池中单独释放描述符集（descriptor set），而不是只能重置整个描述符池。
        //      VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT：
        //          表示允许在绑定描述符集后更新描述符集中的描述符（descriptor），而不是只能在绑定前更新。
        //      VK_DESCRIPTOR_POOL_CREATE_HOST_ONLY_BIT_EXT：
        //          表示描述符池中的描述符集只能用于主机（host）可见的内存类型，即CPU可以直接访问或映射（map）的内存。
        //      VK_DESCRIPTOR_POOL_CREATE_ALLOW_OVERALLOCATION_SETS_BIT_NV：
        //          表示允许从描述符池中分配超过maxSets数量的描述符集，但是不保证分配成功。
        //      VK_DESCRIPTOR_POOL_CREATE_ALLOW_OVERALLOCATION_POOLS_BIT_NV：
        //          表示允许从描述符池中分配超过pPoolSizes指定数量的描述符，但是不保证分配成功。
        //  maxSets：一个uint32_t值，表示描述符池中可以分配的最大描述符集（descriptor set）数量。
        //  poolSizeCount：一个uint32_t值，表示描述符池大小（descriptor pool size）结构体的数量。
        //  pPoolSizes：一个VkDescriptorPoolSize* 指针，表示指向描述符池大小结构体数组的指针。
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);// 设置描述符池中可以分配的最大描述符集数量为MAX_FRAMES_IN_FLIGHT
        poolInfo.poolSizeCount = 1;     // 设置描述符池大小结构体的数量为1，即只有一种类型的描述符
        poolInfo.pPoolSizes = &poolSize;// 设置描述符池大小结构体的指针

        // 调用vkCreateDescriptorPool函数，创建描述符池，并将句柄存储在descriptorPool变量中
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    
    void createDescriptorSets() {       // 创建描述符集（descriptor set），用于绑定统一缓冲区（uniform buffer）到着色器中
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        // 它使用MAX_FRAMES_IN_FLIGHT作为向量的初始大小，并使用descriptorSetLayout作为每个元素的初始值。
        // 这样就可以创建一个包含MAX_FRAMES_IN_FLIGHT个相同的描述符集布局（descriptor set layout）的向量，用于分配描述符集（descriptor set）。

        // VkDescriptorSetAllocateInfo结构体用于指定从descriptor pool分配descriptor set的参数。它包含以下成员：
        //  sType：一个VkStructureType枚举值，表示结构体的类型，必须为VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO。
        //  pNext：一个void* 指针，表示指向扩展结构体的指针，或者为NULL。
        //  descriptorPool：一个VkDescriptorPool句柄，表示要从中分配描述符集的描述符池。
        //  descriptorSetCount：一个uint32_t值，表示要分配的描述符集数量。
        //  pSetLayouts：一个VkDescriptorSetLayout* 指针，表示指向descriptor set layout数组的指针，每个元素指定了对应描述符集的布局。
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;          
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); 
        allocInfo.pSetLayouts = layouts.data();            
        
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);        // 调整descriptorSets向量的大小为MAX_FRAMES_IN_FLIGHT，用于存储每一帧对应的描述符集句柄
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        // 调用vkAllocateDescriptorSets函数，从描述符池中分配描述符集，并将句柄存储在descriptorSets向量中。如果分配失败，抛出运行时异常
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) { // 遍历每一帧对应的索引i
            // 定义一个描述符缓冲区信息（descriptor buffer info）结构体，用于指定要绑定到描述符集中的缓冲区资源
            // VkDescriptorBufferInfo结构体用于指定要绑定到descriptor set中的缓冲区buffer资源。它包含以下成员：
            //  buffer：一个VkBuffer句柄，表示要绑定的缓冲区。
            //  offset：一个VkDeviceSize值，表示要绑定的缓冲区的偏移量，即从缓冲区的起始位置开始绑定的字节位置。
            //  range：一个VkDeviceSize值，表示要绑定的缓冲区的范围，即绑定多少字节的缓冲区数据。如果设置为VK_WHOLE_SIZE，则表示绑定从偏移量开始到缓冲区末尾的所有数据。
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];          // 设置要绑定的缓冲区句柄为uniformBuffers向量中的第i个元素
            bufferInfo.offset = 0;                          // 设置要绑定的缓冲区的偏移量为0，即从缓冲区的起始位置开始绑定
            bufferInfo.range = sizeof(UniformBufferObject); // 设置要绑定的缓冲区的范围为UniformBufferObject结构体的大小，即绑定整个缓冲区

            // VkWriteDescriptorSet结构体用于指定更新描述符集descriptor set中某个描述符descriptor的操作。它包含以下成员：
            //  sType：一个VkStructureType枚举值，表示结构体的类型，必须为VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET。
            //  pNext：一个void* 指针，表示指向扩展结构体的指针，或者为NULL。
            //  dstSet：一个VkDescriptorSet句柄，表示要更新的目标描述符集。
            //  dstBinding：一个uint32_t值，表示要更新的目标描述符在描述符集中的绑定点（binding point），即与着色器（shader）中layout(binding = …)对应的值。
            //  dstArrayElement：一个uint32_t值，表示要更新的目标描述符在描述符数组中的起始位置。如果dstBinding对应的描述符类型是VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK，则表示起始字节偏移量。
            //  descriptorCount：一个uint32_t值，表示要更新多少个描述符数组的元素。descriptorCount有多少，pBufferInfo数组就有多少个元素
            //      如果dstBinding对应的描述符类型是VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK，则表示更新多少字节的数据。
            //  descriptorType：一个VkDescriptorType枚举值，表示要更新的描述符的类型，如统一缓冲区（uniform buffer）、采样器（sampler）、图像（image）等。
            //  pImageInfo：一个VkDescriptorImageInfo* 指针，表示指向图像相关描述符的信息结构体数组的指针，或者为NULL。
            //      只有当descriptorType是VK_DESCRIPTOR_TYPE_SAMPLER、VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER、VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE、VK_DESCRIPTOR_TYPE_STORAGE_IMAGE或VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT时，该指针才有效。
            //  pBufferInfo：一个VkDescriptorBufferInfo* 指针，表示指向缓冲区相关描述符的信息结构体数组的指针，或者为NULL。
            //      只有当descriptorType是VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER、VK_DESCRIPTOR_TYPE_STORAGE_BUFFER、VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC或VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC时，该指针才有效。
            //  pTexelBufferView：一个VkBufferView* 指针，表示指向缓冲区视图（buffer view）句柄数组的指针，或者为NULL。
            //      只有当descriptorType是VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER或VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER时，该指针才有效。
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];                         // 设置要更新的描述符集句柄为descriptorSets向量中的第i个元素
            descriptorWrite.dstBinding = 0;                                     // 设置要更新的描述符在描述符集中的绑定点为0，即与着色器中layout(binding = 0)对应
            descriptorWrite.dstArrayElement = 0;                                // 设置要更新的描述符在描述符数组中的起始位置为0，即只更新一个描述符
            descriptorWrite.descriptorCount = 1;                                // 设置要更新的描述符的数量为1，即只更新一个描述符
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // 设置要更新的描述符的类型为统一缓冲区（uniform buffer）
            descriptorWrite.pBufferInfo = &bufferInfo;                          // 设置要更新的描述符对应的缓冲区信息结构体的指针
            
            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);    // 调用vkUpdateDescriptorSets函数，更新描述符集中的描述符，将缓冲区资源绑定到描述符上
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
            // 函数vkCmdBindDescriptorSets的功能
            //  vkCmdBindDescriptorSets函数用于将一个或多个描述符集（descriptor set）绑定到一个命令缓冲区（command buffer）。
            // 函数vkCmdBindDescriptorSets的参数
            //  commandBuffer：一个VkCommandBuffer句柄，表示要绑定描述符集的命令缓冲区。
            //  pipelineBindPoint：一个VkPipelineBindPoint枚举值，表示要使用描述符集的管线（pipeline）类型。
            //      有两种类型：VK_PIPELINE_BIND_POINT_GRAPHICS和VK_PIPELINE_BIND_POINT_COMPUTE，分别表示图形管线和计算管线。
            //  layout：一个VkPipelineLayout句柄，表示用于指定绑定点（binding point）的管线布局（pipeline layout）。
            //      管线布局是一种用于定义描述符集布局（descriptor set layout）和推送常量（push constant）范围的对象。
            //  firstSet：一个uint32_t值，表示要绑定的第一个描述符集在管线布局中的集合号（set number）。集合号是一种用于区分不同描述符集的编号，从0开始。
            //  descriptorSetCount：一个uint32_t值，表示要绑定的描述符集数量。
            //  pDescriptorSets：一个VkDescriptorSet* 指针，表示指向要绑定的描述符集句柄数组的指针。
            //  dynamicOffsetCount：一个uint32_t值，表示动态偏移量（dynamic offset）数组中的元素数量。动态偏移量是一种用于修改动态缓冲区（dynamic buffer）绑定时的偏移量的值。
            //  pDynamicOffsets：一个uint32_t* 指针，表示指向动态偏移量数组的指针。
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

    
    void updateUniformBuffer(uint32_t currentImage) {                       // 定义一个更新统一缓冲区（uniform buffer）的函数，参数为当前图像的索引（image index）
        // chrono的基本功能
        //  chrono是一个C++头文件，提供了一系列的类型和函数，用于处理时间相关的操作1。它是C++标准模板库（STL）的一部分，包含在C++11及以后的版本中。
        //  chrono提供了三种主要的时钟（clock）类型：system_clock, steady_clock, 和 high_resolution_clock。它们分别用于表示系统时间、稳定时间和高精度时间。
        //  chrono还提供了两种主要的时间概念：duration和time_point。duration用于表示时间间隔，如一分钟、两小时或十毫秒。
        //      time_point用于表示特定的时间点，如程序开始运行的时间、当前帧的时间或某个事件发生的时间。
        //  chrono还提供了一些辅助的类型和函数，用于进行时间单位的转换、时钟之间的转换、日期和时间的格式化等操作。
        /// 引入chrono头文件
        /*#include <chrono>
        // 引入iostream头文件，用于输出
        #include <iostream>

        // 定义一个函数，用于计算斐波那契数列（Fibonacci sequence）中第n项的值
        // 参数为n，返回值为第n项的值
        int fibonacci(int n) {
            // 如果n为0或1，直接返回n
            if (n == 0 || n == 1) {
                return n;
            }
            // 否则，递归地调用fibonacci函数，计算前两项之和
            else {
                return fibonacci(n - 1) + fibonacci(n - 2);
            }
        }

        // 定义主函数
        int main() {
            // 定义一个变量n，表示要计算斐波那契数列中第几项
            int n = 40;
            // 定义一个变量result，用于存储计算结果
            int result = 0;

            // 使用system_clock类获取当前系统时间，并存储在start变量中
            // system_clock类表示系统时间，可以与日历时间互相转换
            auto start = std::chrono::system_clock::now();

            // 调用fibonacci函数，计算第n项的值，并赋值给result变量
            result = fibonacci(n);

            // 使用system_clock类再次获取当前系统时间，并存储在end变量中
            auto end = std::chrono::system_clock::now();

            // 使用duration_cast函数，将end和start之间的时间差转换为毫秒，并存储在elapsed变量中
            // duration_cast函数可以将不同精度的duration对象转换为指定精度的duration对象
            // duration对象表示时间间隔，可以有不同的单位和精度
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            // 使用cout对象输出计算结果和耗时
            std::cout << "Fibonacci(" << n << ") = " << result << "\n";
            std::cout << "Time elapsed: " << elapsed.count() << " ms\n";

            // 返回0，表示程序正常结束
            return 0;
        }*/
        // 这一行只会在第一次调用这个函数时执行一次，然后startTime就会保持不变。
        //  这是因为static关键字的作用之一是让变量只初始化一次，而不是每次进入函数时都重新初始化。
        static auto startTime = std::chrono::high_resolution_clock::now();  // 定义一个静态变量startTime，用于存储程序开始运行的时间点
        auto currentTime = std::chrono::high_resolution_clock::now();       // 定义一个变量currentTime，用于存储当前帧的时间点
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();               // 定义一个变量time，用于计算从程序开始运行到当前帧的时间间隔，单位为秒

        
        UniformBufferObject ubo{};                                          // 定义一个UniformBufferObject结构体变量ubo，用于存储要传递给着色器（shader）的数据，如变换矩阵（transformation matrix）等
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));          // 使用glm库中的lookAt函数，设置相机位置为(2, 2, 2)，目标位置为(0, 0, 0)，上方向为z轴正方向
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);   // 使用glm库中的perspective函数，设置视场角（field of view）为45度，宽高比（aspect ratio）为交换链宽度除以高度，近平面（near plane）为0.1，远平面（far plane）为10
        ubo.proj[1][1] *= -1;                                               // 由于Vulkan和OpenGL使用不同的裁剪空间（clip space），需要对投影矩阵进行修正，将投影矩阵第二行第二列的元素乘以-1，即反转y轴方向
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));      // uniformBuffersMapped数组中的每个元素都是一个指向统一缓冲区映射内存（uniform buffer mapped memory）的指针
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