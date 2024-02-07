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


struct Vertex {             // 定义一个结构体 Vertex，有位置与颜色两个性质
    glm::vec2 pos;          // 定义一个 glm::vec2 类型的成员变量 pos，表示顶点的位置
    glm::vec3 color;        // 定义一个 glm::vec3 类型的成员变量 color，表示顶点的颜色

    //  VkVertexInputBindingDescription结构体是用于描述顶点输入绑定的信息，顶点输入绑定是指从顶点缓冲区中获取顶点数据的方式。
    //      每个顶点缓冲区的绑定号是指使用binding成员变量来标识一个顶点缓冲区或者一个缓冲区内存的不同部分，在创建管线时需要与对应的顶点着色器匹配
    //      一个管线可以有多个顶点绑定描述，每个描述对应一个顶点缓冲区或者一个缓冲区内存的不同部分。
    //      这个结构体包含以下成员：
    //      binding：这个结构体描述的绑定号，用于在顶点着色器中引用顶点数据。
    //      stride：顶点缓冲区的步长是指使用stride成员变量来指定缓冲区中连续元素(如顶点)之间的字节跨距。
    //      inputRate：一个VkVertexInputRate枚举值，指定顶点属性的地址是根据顶点索引还是实例索引来计算的。
    //          它有两个可能的值：
    //          VK_VERTEX_INPUT_RATE_VERTEX：表示顶点属性的地址是根据顶点索引来计算的，即每个顶点都有自己的属性值。
    //          VK_VERTEX_INPUT_RATE_INSTANCE：表示顶点属性的地址是根据实例索引来计算的，即每个实例都有自己的属性值。
    // 定义一个静态成员函数 getBindingDescription，返回一个 VkVertexInputBindingDescription 结构体，表示顶点缓冲区的绑定描述
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // 表示每个顶点使用一个 Vertex 结构体的数据

        return bindingDescription;
    }

    // VkVertexInputAttributeDescription结构体是用于描述顶点输入属性的信息，顶点输入属性是指从顶点缓冲区中获取到的单个数据项，如位置，颜色等6。这个结构体包含以下成员：
    //      location：这个属性在顶点着色器中对应的location编号。layout(position=0) in vec3 aPos
    //      binding：这个属性从哪个绑定描述中获取数据。
    //          一个顶点缓冲区有一个 binding，它表示顶点缓冲区在 vkCmdBindVertexBuffers 函数中的绑定号。
    //          每个顶点属性也有一个 binding，它表示该属性从哪个顶点缓冲区获取数据。
    //      format：这个属性数据的大小和类型，使用VkFormat枚举值表示。
    //      offset：这个属性相对于绑定描述中元素起始位置的字节偏移量。
    // Vulkan会根据VkVertexInputAttributeDescription结构体数组中的offset成员变量来计算每个顶点属性的偏移量，
    //      这个偏移量是相对于绑定描述中元素起始位置的字节偏移量。
    //      例如，如果一个顶点缓冲区包含了两个顶点属性，位置和颜色，每个属性占用12个字节，那么第一个属性的偏移量是0，第二个属性的偏移量是12。
    // Vulkan会根据VkVertexInputAttributeDescription结构体数组中的location成员变量来确定每个顶点属性的location编号，
    //      这个编号是用来在顶点着色器中引用顶点数据的。
    //      例如，如果一个顶点缓冲区包含了两个顶点属性，位置和颜色，那么可以指定第一个属性的location编号为0，第二个属性的location编号为1。
    // 同一绑定点的VkVertexInputAttributeDescription数组的总偏移量应该等于或小于这一绑定点属性描述VkVertexInputBindingDescription里的stride。
    //      这是因为stride表示缓冲区中连续元素之间的字节跨距，而offset表示每个属性相对于元素起始位置的字节偏移量。
    //      如果总偏移量大于stride，那么就会导致数据重叠或跳过
    // 定义一个静态成员函数 getAttributeDescriptions，返回一个包含两个元素的 std::array，表示顶点缓冲区中每个属性的描述
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        // 设置 attributeDescriptions 的第一个元素，表示 pos 属性的描述
        attributeDescriptions[0].binding = 0;                       // 表示 pos 属性所属的顶点缓冲区的绑定号
        attributeDescriptions[0].location = 0;                      // 表示 pos 属性在着色器中的位置号为0
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;  // 表示 pos 属性的数据格式是两个32位浮点数 (float)
        attributeDescriptions[0].offset = offsetof(Vertex, pos);    // 表示 pos 属性在Vertex结构体中的字节偏移量

        // 设置 attributeDescriptions 的第二个元素，表示 color 属性的描述
        attributeDescriptions[1].binding = 0;                       // 表示 color 属性所属的顶点缓冲区的绑定号
        attributeDescriptions[1].location = 1;                      // 表示 color 属性在着色器中的位置号为1
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;// 表示 color 属性的数据格式是三个32位浮点数 (float)
        attributeDescriptions[1].offset = offsetof(Vertex, color);  // 表示 color 属性在 Vertex 结构体中的字节偏移量

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

    // VkBuffer 是一个代表缓冲区对象的不透明句柄。
    //  缓冲区对象是一种线性数组的数据结构，它可以用于多种目的，例如作为顶点数据、索引数据、统一缓冲区、存储缓冲区等。
    // VkDeviceMemory 是一个代表设备内存对象的不透明句柄。
    //  设备内存对象是一种分配在设备上的内存块，它可以被缓冲区对象或图像对象使用。
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

        // 调用vkDestroyBuffer函数，传入设备句柄、vertexBuffer句柄和空指针，销毁vertexBuffer对象
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        // 调用vkFreeMemory函数，传入设备句柄、vertexBufferMemory句柄和空指针，释放vertexBufferMemory对象
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
        
        // 下面是对着色器进行基本的设置与指定
        // VkPipelineVertexInputStateCreateInfo结构体是用于描述图形管线中的顶点输入状态的信息，它包含以下字段：
        // sType：必须为VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO，表示结构体的类型。
        // pNext：可以指向一个扩展结构体，用于提供额外的信息，或者为NULL。
        // flags：保留字段，必须为0。
        // vertexBindingDescriptionCount：指定顶点绑定描述的数量，即pVertexBindingDescriptions数组的长度。
        // pVertexBindingDescriptions：指向一个VkVertexInputBindingDescription结构体数组，用于描述每个顶点缓冲区的绑定号、步长和输入速率。
        // vertexAttributeDescriptionCount：指定顶点属性描述的数量，即pVertexAttributeDescriptions数组的长度。
        // pVertexAttributeDescriptions：指向一个VkVertexInputAttributeDescription结构体数组; 
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

    // 定义一个函数 createVertexBuffer，用于创建顶点缓冲区
    void createVertexBuffer() {
        // VkBufferCreateInfo 结构体是用来创建缓冲区对象的参数。它包含以下字段：
        //  sType:必为VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO。
        //  pNext:NULL或者一个指向扩展结构体的指针。
        //  flags:一个VkBufferCreateFlags位掩码，表示缓冲区对象的额外参数，例如是否支持稀疏绑定、稀疏重排或稀疏别名等。
        //      目前，Vulkan API 只定义了一种有效的 VkBufferCreateFlags 值，即 VK_BUFFER_CREATE_SPARSE_BINDING_BIT，它表示缓冲区对象支持稀疏绑定。
        //      稀疏绑定是一种允许缓冲区对象只绑定部分内存的技术，它可以提高内存利用率和性能。
        //  size:一个 VkDeviceSize 值，表示缓冲区对象的字节大小。
        //  usage:一个 VkBufferUsageFlags 位掩码，表示缓冲区对象的用途，例如是否用作顶点缓冲区、索引缓冲区、统一缓冲区、存储缓冲区等。
        //      VkBufferUsageFlags有这些值：
        //      VK_BUFFER_USAGE_TRANSFER_SRC_BIT：表示缓冲区对象可以作为传输操作的源。
        //      VK_BUFFER_USAGE_TRANSFER_DST_BIT：表示缓冲区对象可以作为传输操作的目标。
        //      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT：表示缓冲区对象可以作为统一缓冲区，用于存储着色器中的常量数据。
        //      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT：表示缓冲区对象可以作为存储缓冲区，用于存储着色器中的可变数据。
        //      VK_BUFFER_USAGE_INDEX_BUFFER_BIT：表示缓冲区对象可以作为索引缓冲区，用于存储绘制命令中的索引数据。
        //      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT：表示缓冲区对象可以作为顶点缓冲区，用于存储顶点输入状态中的顶点数据。
        //  sharingMode：一个 VkSharingMode 值，表示缓冲区对象的共享模式，即它是否可以被多个队列族访问。
        //      VK_SHARING_MODE_EXCLUSIVE：表示缓冲区对象只能被一个队列族使用，在使用前必须进行所有权转移。
        //      VK_SHARING_MODE_CONCURRENT：表示缓冲区对象可以被多个队列族同时使用，无需进行所有权转移。
        //  queueFamilyIndexCount：一个 uint32_t 值，表示 pQueueFamilyIndices 数组的元素个数。
        //  pQueueFamilyIndices：一个指向 uint32_t 值的数组，表示可以访问该缓冲区对象的队列族索引。
        //      它只在共享模式为 VK_SHARING_MODE_CONCURRENT 时有效。它可以用来实现跨队列族或跨设备的资源共享。
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(vertices[0]) * vertices.size();
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }


        // VkMemoryRequirements 结构体是用来表示缓冲区对象或图像对象的内存需求的。它包含以下字段：
        //  size：表示所需的内存块的字节大小。
        //  alignment：表示所需的内存块的字节对齐值。
        //  memoryTypeBits：表示支持该对象的内存类型的位掩码。
        //      位i被设置1时，意味着物理设备内存属性中(我电脑有3个)的第i个内存类型支持该缓冲区对象
        VkMemoryRequirements memRequirements;   // 函数是用来获取缓冲区对象的内存需求的
        vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

        // VkMemoryAllocateInfo 结构体是用来分配设备内存对象的参数。它包含以下字段：
        //  sType：必为VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO。
        //  pNext：NULL 或者一个指向扩展结构体的指针。
        //  allocationSize：一个 VkDeviceSize 值，表示要分配的内存块的字节大小。
        //  memoryTypeIndex：一个 uint32_t 值，表示从物理设备内存属性中选择的内存类型的索引。
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;    //下一句找出第一个支持该缓冲区对象的内存类型的索引，这里找到索引1，然后作为缓冲区内存类型
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        // 顶点缓冲区绑定内存意味着将一个 VkBuffer 对象和一个 VkDeviceMemory 对象关联起来。
        // 使得顶点缓冲区可以访问设备内存中的数据。这需要调用 vkBindBufferMemory 函数，并提供两者的句柄以及一个字节偏移量
        vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

        void* data;
        // vkMapMemory 函数是用来将一个设备内存对象映射到主机地址空间的1。它接受以下参数：
        //  device：指定拥有该内存对象的逻辑设备句柄。
        //  memory：指定要映射的设备内存对象句柄。
        //  offset：指定从内存对象开始的字节偏移量。
        //  size：指定要映射的内存范围的字节大小，或者 VK_WHOLE_SIZE 表示从 offset 到内存对象末尾。
        //  flags：保留为未来使用，必须为 0。
        //  ppData：指向一个 void* 变量的地址，用于返回映射范围的主机可访问指针。
        vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferInfo.size); // 将 vertices 数组中的数据复制到映射内存区域中
        vkUnmapMemory(device, vertexBufferMemory);              // 取消映射 vertexBufferMemory 对象
    }

    // 第一个参数为缓冲区适合的内存类型(有多个)，第二参数是我们需要的类型。函数目的是找到第一个及适合当前缓冲区的，同时在这些适合的缓冲区选择拥有特性MemoryProperty的内存类型索引。
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        // Vulkan中的内存分为两大类：Host Memory 和 Device Memory。
        //  Host Memory 是指 CPU 可以直接访问的系统内存，Device Memory 是指 GPU 可以直接访问的显卡内存。
        // 内存堆是一种内存管理的机制，它可以动态地分配和释放内存块。内存堆通常由操作系统或运行时库提供，也可以由程序员自己实现。它包含以下字段：

        // VkPhysicalDeviceMemoryProperties 是一个结构体，它用来描述物理设备的内存属性2。它包含以下字段：
        //  memoryTypeCount：表示memoryTypes数组的有效元素个数。我电脑有3个；
        //  memoryTypes：一个包含 VK_MAX_MEMORY_TYPES 个 VkMemoryType 结构体的数组，表示可以用来访问内存堆的内存类型。
        //      propertyFlags：一个 VkMemoryPropertyFlags 位掩码，表示该内存类型的属性，例如是否可以被主机映射访问、是否在主机上是缓存的等。
        //          VkMemoryPropertyFlags 是一个 VkFlags 类型的位掩码，它用来指定内存类型的属性。
        //          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT：这个属性表示这块内存是分配在设备本地的，也就是显卡的显存。
        //              这种内存对于显卡访问来说是最快的，但是对于 CPU 访问来说可能很慢或者不可用。使用这种内存可以提高图形渲染的性能，但是需要注意数据的传输和同步。
        //          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT：这个属性表示这块内存可以被 CPU 访问，也就是可以通过 vkMapMemory 函数映射到 CPU 的地址空间。
        //              这种内存可以用来存储 CPU 生成或修改的数据，然后传递给显卡使用。使用这种内存可以方便 CPU 和 GPU 的数据交互，但是需要注意内存的一致性和缓存问题。
        //          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT：这个属性表示这块内存在 CPU 和 GPU 之间是一致的，也就是说不需要调用 vkFlushMappedMemoryRanges 或 vkInvalidateMappedMemoryRanges 函数来刷新或失效 CPU 的缓存。
        //              这种内存可以简化 CPU 和 GPU 的数据同步，但是可能会牺牲一些性能或者消耗更多的电力。
        //          VK_MEMORY_PROPERTY_HOST_CACHED_BIT：这个属性表示这块内存在 CPU 端是有缓存的，也就是说 CPU 访问这块内存会比没有缓存的快。
        //              但是，有缓存的内存可能不是一致的，也就是说需要调用 vkFlushMappedMemoryRanges 或 vkInvalidateMappedMemoryRanges 函数来保证 CPU 和 GPU 之间的数据正确性。
        //              使用这种内存可以提高 CPU 的访问速度，但是需要注意缓存管理和同步问题。
        //          VK_MEMORY_PROPERTY_HOST_CACHED_BIT，表示这种内存类型分配的内存在主机上有缓存。主机访问未缓存的内存比访问缓存的内存慢，但未缓存的内存总是主机一致的1。
        //          VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT，表示这种内存类型只允许设备访问内存。内存类型不能同时具有VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT和VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT。另外，对象的后备内存可能由实现按需提供，如延迟分配的内存中所述1。
        //          VK_MEMORY_PROPERTY_PROTECTED_BIT，表示这种内存类型只允许设备访问内存，并允许受保护的队列操作访问内存。内存类型不能同时具有VK_MEMORY_PROPERTY_PROTECTED_BIT和任何VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT、VK_MEMORY_PROPERTY_HOST_COHERENT_BIT或VK_MEMORY_PROPERTY_HOST_CACHED_BIT1。
        //          VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD，表示这种内存类型分配的内存对设备访问是自动可用和可见的。这是一个由AMD提供的扩展2。
        //          VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD，表示这种内存类型分配的内存在设备上没有缓存。未缓存的设备内存总是设备一致的。这也是一个由AMD提供的扩展2。
        //          VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV，表示外部设备可以直接访问这种内存类型分配的内存。这是一个由NVIDIA提供的扩展3。
        //          VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM，表示VkMemoryPropertyFlagBits枚举中有效值的最大数量。它不是一个真正的标志位，而是用于错误检查和范围验证4。
        //          我电脑有三个组合：
        //          1:VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        //          2:VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT&VK_MEMORY_PROPERTY_HOST_CACHED_BIT
        //          3:VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        //      heapIndex：表示该内存类型对应的内存堆的索引，必须小于memoryHeapCount字段。
        //  memoryHeapCount：表示 memoryHeaps 数组的有效元素个数。我电脑有1个；
        //  memoryHeaps：一个包含 VK_MAX_MEMORY_HEAPS 个 VkMemoryHeap 结构体的数组，表示可以用来分配内存的内存堆。
        //      它包含以下字段：
        //      size：一个 VkDeviceSize 值，表示该内存堆的字节大小。
        //      flags：一个 VkMemoryHeapFlags 位掩码，表示该内存堆的属性，例如是否是设备本地的、是否支持多实例等。
        //          VK_MEMORY_HEAP_DEVICE_LOCAL_BIT 它表示该内存堆对应于设备本地内存，即GPU可以直接访问的显存。
        //              设备本地内存可能有不同于主机本地内存（即 CPU 可以直接访问的系统内存）的性能特征和支持的内存属性。
        //          VK_MEMORY_HEAP_MULTI_INSTANCE_BIT 它表示在一个代表多个物理设备的逻辑设备中，该内存堆有每个物理设备实例的副本。
        //              默认情况下，从这样的内存堆分配的内存会被复制到每个物理设备实例的内存堆中。这种标志位通常用于支持 SLI 或 Crossfire 等多 GPU 技术2。
        //          VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR 它是由 VK_KHR_device_group_creation 扩展提供的，用于兼容旧版本的 Vulkan API3。
        //          VK_MEMORY_HEAP_FLAG_BITS_MAX_ENUM 它表示 VkMemoryHeapFlagBits 枚举中有效值的最大数量。它不是一个真正的标志位，而是用于错误检查和范围验证。
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties); //获取物理设备的内存属性，并将其存储在memProperties中

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            // typeFilter & (1 << i)：1 << i 是一个位运算表达式，它表示将数字 1 左移 i 位
            //      这个部分是用来检查 typeFilter 的第 i 位是否为 1，如果为 1，表示第 i 个内存类型是可用的，否则不可用。  
            // (memProperties.memoryTypes[i].propertyFlags & properties) == properties：
            //      这个部分是用来检查第 i 个内存类型的属性是否包含了 properties 中指定的所有属性。
            //      这个程序需要VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            //      第一个内存类型组合就可以适用
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;   // 如果满足条件，返回当前内存类型的索引
            }
        }

        // 如果没有找到合适的内存类型，抛出一个运行时错误异常，并输出错误信息
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

            // 定义一个 VkBuffer 类型的数组 vertexBuffers，并初始化为包含 vertexBuffer 句柄的单元素数组，表示要绑定的顶点缓冲区列表
            VkBuffer vertexBuffers[] = { vertexBuffer };
            // 定义一个 VkDeviceSize 类型的数组 offsets，并初始化为包含 0 的单元素数组，表示每个顶点缓冲区的字节偏移量列表
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            // vkCmdBindVertexBuffers 函数是用来绑定顶点缓冲区到一个命令缓冲区的，以便在后续的绘制命令中使用。它接受以下参数：
            //  commandBuffer：指定要记录命令的命令缓冲区句柄。
            //  firstBinding：指定第一个顶点输入绑定的索引，它的状态会被该命令更新。
            //  bindingCount：指定要更新状态的顶点输入绑定的数量。
            //  pBuffers：指向一个包含缓冲区句柄的数组。
            //  pOffsets：指向一个包含缓冲区偏移量的数组。
            // 该函数的作用是将 pBuffers 和 pOffsets 中的元素 i 替换为当前状态的顶点输入绑定 firstBinding + i，对于 i 在[0, bindingCount) 范围内。
            // 顶点输入绑定会更新为从缓冲区 pBuffers[i] 的开始处加上偏移量 pOffsets[i] 开始。
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