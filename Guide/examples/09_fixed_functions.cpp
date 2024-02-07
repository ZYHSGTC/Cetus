#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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

    VkPipelineLayout pipelineLayout;
    //

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
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

        createInfo.oldSwapchain = VK_NULL_HANDLE;

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

        // VkPipelineVertexInputStateCreateInfo结构体是用于描述图形管线中的顶点输入状态的信息，它包含以下字段1：
        // sType：必须为VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO，表示结构体的类型。
        // pNext：可以指向一个扩展结构体，用于提供额外的信息，或者为NULL。
        // flags：保留字段，必须为0。
        // vertexBindingDescriptionCount：指定顶点绑定描述的数量，即pVertexBindingDescriptions数组的长度。
        // pVertexBindingDescriptions：指向一个VkVertexInputBindingDescription结构体数组，用于描述每个顶点缓冲区的绑定号、步长和输入速率。
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
        // vertexAttributeDescriptionCount：指定顶点属性描述的数量，即pVertexAttributeDescriptions数组的长度。
        // pVertexAttributeDescriptions：指向一个VkVertexInputAttributeDescription结构体数组;
        // VkVertexInputAttributeDescription结构体是用于描述顶点输入属性的信息，顶点输入属性是指从顶点缓冲区中获取到的单个数据项，如位置，颜色等6。这个结构体包含以下成员：
        //      location：这个属性在顶点着色器中对应的location编号。layout(position=0) in vec3 aPos
        //      binding：这个属性从哪个绑定描述中获取数据。
        //      format：这个属性数据的大小和类型，使用VkFormat枚举值表示。
        //      offset：这个属性相对于绑定描述中元素起始位置的字节偏移量。
        // Vulkan会根据VkVertexInputAttributeDescription结构体数组中的offset成员变量来计算每个顶点属性的偏移量，
        //      这个偏移量是相对于绑定描述中元素起始位置的字节偏移量。
        //      例如，如果一个顶点缓冲区包含了两个顶点属性，位置和颜色，每个属性占用12个字节，那么第一个属性的偏移量是0，第二个属性的偏移量是12。
        // Vulkan会根据VkVertexInputAttributeDescription结构体数组中的location成员变量来确定每个顶点属性的location编号，
        //      这个编号是用来在顶点着色器中引用顶点数据的2。
        //      例如，如果一个顶点缓冲区包含了两个顶点属性，位置和颜色，那么可以指定第一个属性的location编号为0，第二个属性的location编号为1。
        // 同一绑定点的VkVertexInputAttributeDescription数组的总偏移量应该等于或小于这一绑定点属性描述VkVertexInputBindingDescription里的stride。
        //      这是因为stride表示缓冲区中连续元素之间的字节跨距，而offset表示每个属性相对于元素起始位置的字节偏移量。
        //      如果总偏移量大于stride，那么就会导致数据重叠或跳过
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;      //这里我没有开辟顶点缓冲区所以不需要顶点绑定描述
        vertexInputInfo.vertexAttributeDescriptionCount = 0;    //这里我没有开辟顶点缓冲区所以不需要顶点属性描述


        // VkPipelineInputAssemblyStateCreateInfo结构体是用于指定新创建的管线输入装配状态的参数1。它包含以下成员2：
        //  sType：必须为VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
        //  pNext：NULL或者一个指向扩展这个结构体的指针。
        //  flags：保留未来使用的标志位。
        //  topology：一个VkPrimitiveTopology值，定义了图元拓扑，如下面介绍的。
        //  它有以下枚举值4：
        //      VK_PRIMITIVE_TOPOLOGY_POINT_LIST：表示一系列独立的点图元。
        //      VK_PRIMITIVE_TOPOLOGY_LINE_LIST：表示一系列独立的线图元。
        //      VK_PRIMITIVE_TOPOLOGY_LINE_STRIP：表示一系列连接的线图元，连续的线共享一个顶点。
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST：表示一系列独立的三角形图元。
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP：表示一系列连接的三角形图元，连续的三角形共享一条边。
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN：表示一系列连接的三角形图元，所有的三角形共享一个公共顶点。
        //      VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY：表示一系列独立的线图元，带有邻接信息。
        //      VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY：表示一系列连接的线图元，带有邻接信息，连续的图元共享三个顶点。
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY：表示一系列独立的三角形图元，带有邻接信息。
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY：表示一系列连接的三角形图元，带有邻接信息，连续的三角形共享一条边。
        //      VK_PRIMITIVE_TOPOLOGY_PATCH_LIST：表示一系列独立的补丁图元。
        //  primitiveRestartEnable：控制是否使用一个特殊的顶点索引值来重启图元的装配。
        //      这个选项只适用于索引绘制（vkCmdDrawIndexed，vkCmdDrawMultiIndexedEXT和vkCmdDrawIndexedIndirect）。
        // 图元装配式渲染管线是一种固定功能的渲染方式，它由一组预定义的电路或算法来完成图形处理的各个阶段。
        // 在这种管线中，程序员只能通过设置一些参数来调整渲染效果，而不能自定义各个阶段的处理逻辑。
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // 视口与裁剪矩形可以看《vulkan编程指南》图片理解
        // 管线视口是指在渲染管线中，用来定义图形在屏幕上的显示范围和位置
        // 视口相当于窗口内的子显示窗口，裁剪是对帧缓冲到视口进行裁剪
        // VkPipelineViewportStateCreateInfo结构体是用于指定新创建的管线视口状态的参数1。它包含以下成员2：
        //  sType：必须为VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
        //  pNext：NULL或者一个指向扩展这个结构体的指针。
        //  flags：保留未来使用的标志位。
        //  viewportCount：使用的视口的数量。
        //  pViewports：一个指向VkViewport结构体数组的指针，定义了视口变换。
        //      如果视口状态是动态的，也就是视口在绘制命令之前设置，这个成员会被忽略。
        //      VkViewport结构体用于描述视口区域。它包含以下成员：
        //          x, y：视口左上角的坐标。
        //          width, height：视口的宽度和高度。
        //          minDepth, maxDepth：视口的深度范围。framebuffer深度坐标可以使用固定点或浮点数表示。如果深度 / 模板附件有浮点数深度分量，必须使用浮点数表示。
        //  scissorCount：裁剪矩形的数量，必须和视口数量相同。
        //  pScissors：一个指向VkRect2D结构体数组的指针，定义了对应视口的裁剪矩形范围。如果裁剪状态是动态的，这个成员会被忽略。
        //  VkRect2D结构体用于描述一个二维子区域5。它包含以下成员：
        //          offset：一个VkOffset2D结构体，指定矩形偏移量。
        //          extent：一个VkExtent2D结构体，指定矩形范围。
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        //VkPipelineRasterizationStateCreateInfo结构体是用于指定新创建的管线光栅化状态的参数。它包含以下成员：
        // sType：必须为VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
        // pNext：NULL或者一个指向扩展这个结构体的指针。
        // flags：保留未来使用的标志位。
        // depthClampEnable：控制是否将片段的深度值限制在深度测试范围内。
        //      depthClampEnable为真的话，那么深度值超出[0,1]范围的片段将被丢弃，而不是被裁剪
        // rasterizerDiscardEnable：控制是否在光栅化阶段之前丢弃图元。
        //      光栅化阶段之前丢弃图元意味着在图元被转换为像素之前，根据一些条件判断是否需要绘制它们。
        // polygonMode：polygon是多边形的意思，确定多边形渲染模式，可以是填充模式、线框模式或点模式。
        //      它有以下几个可能的值：
        //      VK_POLYGON_MODE_FILL：多边形的内部和边缘都会被填充颜色。
        //      VK_POLYGON_MODE_LINE：多边形只会绘制边缘，形成线框模式。
        //          在光栅化过程中，只有位于多边形边上的片元/像素会被填充颜色，而多边形内部的片元/像素会被忽略。
        //      VK_POLYGON_MODE_POINT：多边形只会绘制顶点，形成点云模式。
        //          在光栅化过程中，只有位于多边形顶点上的片元/像素会被填充颜色，而其他位置的片元/像素会被忽略。
        //      VK_POLYGON_MODE_FILL_RECTANGLE_NV：多边形会使用修改后的光栅化规则，只要一个采样点在三角形的轴对齐包围盒内，就认为它在多边形内部。这是一个由NVidia提供的扩展模式。
        // cullMode：用于图元剔除的三角形朝向，可以是正面、背面或双面。
        //      它有以下几个可能的值：
        //      VK_CULL_MODE_NONE：表示不进行图元剔除，即保留所有的图元。
        //      VK_CULL_MODE_FRONT_BIT：表示剔除正面朝向的图元，即只保留背面朝向的图元。
        //      VK_CULL_MODE_BACK_BIT：表示剔除背面朝向的图元，即只保留正面朝向的图元。
        //      VK_CULL_MODE_FRONT_AND_BACK：表示剔除所有的图元，即不保留任何图元。
        // frontFace：指定正面三角形的方向，可以是顺时针或逆时针。
        //      它有以下两个可能的值：
        //      VK_FRONT_FACE_COUNTER_CLOCKWISE：表示逆时针方向的三角形为正面朝向。
        //      VK_FRONT_FACE_CLOCKWISE：表示顺时针方向的三角形为正面朝向。
        // depthBiasEnable：Bias有偏移的意思，控制是否对片段的深度值进行偏移。
        //      深度值偏移是一种用于解决深度缓冲区中的深度冲突问题的技术。
        //      深度冲突是指当两个或多个物体在同一位置或非常接近的位置渲染时，
        //      由于深度缓冲区的精度有限，导致物体表面出现闪烁或撕裂的现象。
        //      深度值偏移的原理是对物体的深度值进行一定程度的偏移，
        //      使得不同物体之间有一定的深度差异，从而避免深度冲突
        // depthBiasConstantFactor：一个标量因子，控制每个片段添加的常数深度偏移量。
        // depthBiasClamp：一个最大（或最小）深度偏移量，用于限制片段的深度偏移量。
        // depthBiasSlopeFactor：一个标量因子。它会和片段的斜率相乘，得到一个深度偏移量
        //      片段的斜率是指片段所在平面与近平面的夹角的正切值。
        //      做深度偏移的时候，通常会使用depthBiasConstantFactor和depthBiasSlopeFactor两个参数来控制偏移量的大小。
        //      这两个参数会分别和每个片段（fragment）的深度值相加，得到最终的深度偏移量
        // lineWidth：光栅化线段的宽度。等于一的时候和屏幕上的像素点一样宽;
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.lineWidth = 1.0f;

        // 多重采样是一种使用更多颜色、深度和模板信息（样本）对图元（点，直线，多边形，位图，图像）进行反走样(抗锯齿)处理的技术。
        //      它通过在一个像素上进行多次采样多次计算并最终汇总 (Resolve to single-sample)，可使绘制的图像边缘更加平。
        //      光栅化阶段一个重要的工作就是插值计算(Interpolation），所以多重采样作用到这个阶段主要是多重插值。
        // 多重采样的阶段发生在图形渲染管线的以下部分：
        //      获取可用采样数：这个阶段是在创建图形管线之前进行的。
        //          查询物理设备的属性，获取颜色和深度缓冲区支持的最大采样数，取较小值作为可用采样数。
        //      设置渲染目标：这个阶段是在创建图形管线之前进行的。
        //          创建一个多重采样缓冲区作为附件，用于存储每个像素的多个样本数据。
        //          这个缓冲区需要指定采样数、格式、用途等参数，并分配内存和创建图像视图。
        //          多重采样缓冲区与交换链图像与帧缓冲区的尺寸和格式相同，但是每个像素可以存储多个样本。
        //          多重采样缓冲区可以看作多层帧缓冲，每层缓冲储存一个样本。
        //          在渲染结束后，多重采样缓冲区会被解析为交换链图像，然后呈现到屏幕上。
        //      添加新附件：这个阶段是在创建图形管线时进行的。
        //          在渲染通道中添加一个多重采样附件描述和引用，用于存储多重采样缓冲区的数据。
        //          同时，修改颜色附件的用途为转换源，并添加一个解析子通道，用于将多重采样缓冲区解析为帧缓冲区。
        //      质量提升：这个阶段是在创建图形管线时进行的。
        //          在管线创建信息中指定多重采样状态，包括采样数、遮罩、覆盖测试等参数。这些参数可以影响多重采样的质量和性能。
        //      多重采样测试：这个阶段是在光栅化之后进行的。
        //          需要根据遮罩和覆盖测试来确定哪些样本被保留或丢弃。
        //          遮罩测试：遮罩是一个位掩码，用于指定哪些位平面(那层帧缓冲/多重采样缓冲)参与测试，每个位平面对应一个样本。
        //          遮罩测试是由程序员决定的，而不是设备决定的。程序员可以在创建图形管线时的遮罩码pSampleMask。
        //          遮罩测试的目的是根据遮罩的设置来决定哪些样本被保留或丢弃。
        //              位掩码是一个二进制数，用于表示一组开关的状态。每个二进制位对应一个开关，1表示开，0表示关。
        //              0x0F表示00001111，表示有四个开关都是开的；
        //              如果遮罩码为0x0F，那么表示4个样本都参与测试；
        //              0x03表示00000011，表示有两个开关是开的，后面的都是关的
        //              如果遮罩码为0x03，那么表示只有前两个样本参与测试，后两个样本被丢弃。
        //              如果遮罩码为0x09，也就是00001001。这样，只有第一第三个样本会被保留，其他的都会被丢弃。
        //          覆盖测试：覆盖测试是一种优化技术，用于跳过那些被完全覆盖的像素。
        //          覆盖测试的目的是根据片段的深度值和深度缓冲区中的值来判断一个像素是否被完全覆盖。
        //              如果一个像素被完全覆盖，那么它的所有样本都不需要进行着色，可以节省计算资源。
        //              如果一个像素没有被完全覆盖，那么它的部分或全部样本需要进行着色。
        //      多重采样解析：这个阶段是在片元着色器之后进行的。
        //          多重采样会输出多个颜色值，每个有效的采样点都有一个颜色值。这些颜色值会被写入多重采样缓冲区
        //          需要将多重采样缓冲区中的每个像素的多个样本数据合并为一个单一的颜色值，并写入帧缓冲区中。
        //          解析过程通常是对每个像素中所有有效的采样点颜色值进行平均或加权平均，得到该像素最终的颜色值。
        //          多重采样解析的方法有不同的选择，例如平均法（average）、最大法（max）、最小法（min）等。
        //  结构体VkPipelineMultisampleStateCreateInfo是用于指定新创建的管线多重采样状态的参数。它包含以下成员：
        //  sType：必须为VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
        //  pNext：NULL或者一个指向扩展这个结构体的指针。
        //  flags：保留未来使用的标志位。
        //  rasterizationSamples：指定光栅化阶段使用的采样数。这个值决定了多重采样的质量和性能。
        //      多重采样不是随机的，而是根据一定的规则分布在像素内部。不同的采样数对应不同的采样模式。
        //      4倍采样通常使用正方形网格的四个顶点作为采样点，8倍采样通常使用正方形网格的四个顶点和四个中点作为采样点
        //      VkSampleCountFlagBits是一个枚举类型，用于表示图像或帧缓冲区中每个像素的样本数。它有以下枚举值和含义：
        //      VK_SAMPLE_COUNT_1_BIT：指定一个像素有一个样本
        //      VK_SAMPLE_COUNT_2_BIT：指定一个像素有两个样本
        //      VK_SAMPLE_COUNT_4_BIT：指定一个像素有四个样本
        //      VK_SAMPLE_COUNT_8_BIT：指定一个像素有八个样本
        //      VK_SAMPLE_COUNT_16_BIT：指定一个像素有十六个样本
        //      VK_SAMPLE_COUNT_32_BIT：指定一个像素有三十二个样本
        //      VK_SAMPLE_COUNT_64_BIT：指定一个像素有六十四个样本
        //  sampleShadingEnable：一个VkBool32值，控制是否启用样本着色（sample shading）。
        //  minSampleShading：一个浮点数，指定最小的样本着色比例。
        //      如果sampleShadingEnable为VK_TRUE，那么每个片段中至少有minSampleShading × rasterizationSamples个采样点会被单独着色。
        //      它指定了每个像素内部进行着色的最小采样率。如果它的值为1.0，那么每个采样点都会被独立着色。
        //      如果它的值为0.0，那么只有一个采样点会被着色，然后复制给其他采样点。
        //      如果它的值在0.0和1.0之间，那么只有一部分采样点会被着色，具体哪些采样点由实现决定。
        // pSampleMask：一个指向VkSampleMask值数组的指针，用于在采样遮罩测试（sample mask test）中过滤掉一些采样点。
        //      遮罩测试会将片段着色器输出的SampleMask与光栅化阶段生成的覆盖掩码（coverage mask）进行逻辑与运算，得到最终的覆盖掩码。
        //      只有最终覆盖掩码中为1的位对应的采样点才会被写入颜色缓冲区。
        // alphaToCoverageEnable：用于启用或禁用alpha到覆盖率转换，也就是根据片段着色器输出的alpha值来调整每个样本的覆盖率。
        //      如果启用了这个参数，那么片段着色器输出的alpha值会乘以原始的覆盖率，得到新的覆盖率。
        //      这样可以实现一种类似于透明混合的效果，但是不需要对物体进行排序。
        //      覆盖率是一个概念，用于表示一个像素中有多少个样本被覆盖的比例。
        //          覆盖率的范围是0到1，0表示没有任何样本被覆盖，1表示所有样本都被覆盖。
        //          覆盖率可以由光栅化阶段计算出来，也可以由alpha到覆盖率转换来修改1。
        //      覆盖率可以实现类似透明混合的效果，是因为它可以控制每个样本的颜色输出的权重。
        //          如果一个像素中有多个样本被覆盖，那么这些样本的颜色值会根据它们的覆盖率来进行加权平均，得到最终的像素颜色。
        //          这样就可以模拟出半透明物体的效果，让后面的物体透过前面的物体看到。
        // alphaToOneEnable：用于启用或禁用alpha到一转换，也就是将片段着色器输出的alpha值强制设置为1.0。
        //      如果启用了这个参数，那么片段着色器输出的alpha值会被忽略，不会影响覆盖率。
        //      这样可以避免一些不需要透明度的物体被误认为是半透明的。
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // VkPipelineColorBlendAttachmentState结构体是用于配置颜色混合的一个结构体，它可以对每个绑定的帧缓冲进行单独的颜色混合设置1。它包含以下字段2：
        //  blendEnable：一个布尔值，用于启用或禁用颜色混合。
        //  srcColorBlendFactor：一个VkBlendFactor枚举值，用于指定源颜色因子，也就是新片段的颜色值的权重。
        //  dstColorBlendFactor：一个VkBlendFactor枚举值，用于指定目标颜色因子，也就是帧缓冲中的颜色值的权重。
        //  colorBlendOp：一个VkBlendOp枚举值，用于指定源颜色和目标颜色的混合操作，可以是加法、减法、反向减法、最小值或最大值。
        //  srcAlphaBlendFactor：一个VkBlendFactor枚举值，用于指定源alpha因子，也就是新片段的alpha值的权重。
        //  dstAlphaBlendFactor：一个VkBlendFactor枚举值，用于指定目标alpha因子，也就是帧缓冲中的alpha值的权重。
        //  alphaBlendOp：一个VkBlendOp枚举值，用于指定源alpha和目标alpha的混合操作，可以是加法、减法、反向减法、最小值或最大值。
        //  colorWriteMask：一个四位位掩码，用于指定哪些颜色分量被写入帧缓冲。可以是R、G、B、A或者它们的组合。
        //  
        // VkBlendFactor有以下值：
        //  VK_BLEND_FACTOR_ZERO：表示权重为0，相当于忽略该值。
        //  VK_BLEND_FACTOR_ONE：表示权重为1，相当于保留该值。
        //  VK_BLEND_FACTOR_SRC_COLOR：表示权重为源颜色，也就是新片段的颜色。
        //  VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR：表示权重为1减去源颜色，也就是反转新片段的颜色。
        //  VK_BLEND_FACTOR_DST_COLOR：表示权重为目标颜色，也就是帧缓冲中的颜色。
        //  VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR：表示权重为1减去目标颜色，也就是反转帧缓冲中的颜色。
        //  VK_BLEND_FACTOR_SRC_ALPHA：表示权重为源alpha，也就是新片段的alpha值。
        //  VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA：表示权重为1减去源alpha，也就是反转新片段的alpha值。
        //  VK_BLEND_FACTOR_DST_ALPHA：表示权重为目标alpha，也就是帧缓冲中的alpha值。
        //  VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA：表示权重为1减去目标alpha，也就是反转帧缓冲中的alpha值。
        //  VK_BLEND_FACTOR_CONSTANT_COLOR：表示权重为一个常数颜色，可以通过VkPipelineColorBlendStateCreateInfo结构体中的blendConstants字段来设置。
        //  VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR：表示权重为1减去一个常数颜色。
        //  VK_BLEND_FACTOR_CONSTANT_ALPHA：表示权重为一个常数alpha，可以通过VkPipelineColorBlendStateCreateInfo结构体中的blendConstants字段来设置。
        //  VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA：表示权重为1减去一个常数alpha。
        //  VK_BLEND_FACTOR_SRC_ALPHA_SATURATE：表示权重为源alpha和1减去目标alpha中较小的一个，这可以实现一种预乘alpha混合。

        // VkBlendOp有以下值：
        //  VK_BLEND_OP_ADD：表示将两个值相加，这是最常用的混合操作。
        //  VK_BLEND_OP_SUBTRACT：表示将目标颜色从源颜色中减去。
        //  VK_BLEND_OP_REVERSE_SUBTRACT：表示将源颜色从目标颜色中减去。
        //  VK_BLEND_OP_MIN：表示取两个值中较小的一个。
        //  VK_BLEND_OP_MAX：表示取两个值中较大的一个。
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        // VkPipelineColorBlendStateCreateInfo结构体是用于配置管线颜色混合相关的设置的一个结构体，它可以对每个绑定的帧缓冲进行单独的颜色混合设置1。它包含以下字段2：
        //  sType：一个VkStructureType值，用于标识这个结构体的类型。
        //  pNext：一个指针，用于指向一个扩展这个结构体的其他结构体，或者为NULL。
        //  flags：一个位掩码，用于指定额外的颜色混合信息，目前没有定义任何有效的值。
        //  logicOpEnable：一个布尔值，用于启用或禁用逻辑操作，逻辑操作是一种基于位运算的颜色混合方式。
        //  logicOp：一个VkLogicOp枚举值，用于选择要应用的逻辑操作，只有在logicOpEnable为VK_TRUE时有效。
        //  attachmentCount：一个整数，用于指定pAttachments数组中的VkPipelineColorBlendAttachmentState结构体的数量。它与渲染通道中的颜色附件数量相同。
        //  pAttachments：一个指针，用于指向一个VkPipelineColorBlendAttachmentState结构体数组，定义了每个颜色附件的颜色混合设置。
        //      pAttachments是一个数组，是因为渲染通道中可以有多个帧缓冲附件，每个附件可以有不同的混合方式。这样可以实现更灵活和复杂的渲染效果。
        //      pAttachments的每一个元素对应一个颜色附件。一个帧缓冲可以有多个颜色附件，也就是多个图像视图，用于存储不同的渲染结果。
        //      例如，一个颜色附件可以用来存储漫反射光照，另一个颜色附件可以用来存储镜面高光，最后再把它们混合起来
        //  blendConstants：一个指针，用于指向一个包含四个浮点数的数组，分别表示R、G、B和A分量的混合常数，它们可以作为源因子或目标因子的一部分。
        //      可以作为VkPipelineColorBlendAttachmentState得常数混合权重；
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

        // VkDynamicState可以取以下值之一：
        //  VK_DYNAMIC_STATE_VIEWPORT：表示视口状态是动态的，可以使用vkCmdSetViewport命令来设置视口的数量、位置和尺寸。
        //  VK_DYNAMIC_STATE_SCISSOR：表示裁剪矩形状态是动态的，可以使用vkCmdSetScissor命令来设置裁剪矩形的数量、位置和尺寸。
        //  VK_DYNAMIC_STATE_LINE_WIDTH：表示光栅化线宽状态是动态的，可以使用vkCmdSetLineWidth命令来设置线宽的值。
        //  VK_DYNAMIC_STATE_DEPTH_BIAS：表示深度偏移状态是动态的，可以使用vkCmdSetDepthBias命令来设置深度偏移的常数因子、斜率因子和截断值。
        //  VK_DYNAMIC_STATE_BLEND_CONSTANTS：表示混合常数状态是动态的，可以使用vkCmdSetBlendConstants命令来设置混合常数的R、G、B和A分量。
        //  VK_DYNAMIC_STATE_DEPTH_BOUNDS：表示深度范围状态是动态的，可以使用vkCmdSetDepthBounds命令来设置深度范围的最小值和最大值。
        //  VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK：表示模板比较掩码状态是动态的，可以使用vkCmdSetStencilCompareMask命令来设置模板比较掩码的面部和值。
        //  VK_DYNAMIC_STATE_STENCIL_WRITE_MASK：表示模板写入掩码状态是动态的，可以使用vkCmdSetStencilWriteMask命令来设置模板写入掩码的面部和值。
        //  VK_DYNAMIC_STATE_STENCIL_REFERENCE：表示模板参考值状态是动态的，可以使用vkCmdSetStencilReference命令来设置模板参考值的面部和值。
        // VkPipelineDynamicStateCreateInfo结构体是用于配置管线动态状态相关的设置的一个结构体，它可以指定哪些管线状态是从动态状态命令中获取的，而不是从管线创建信息中获取的1。它包含以下字段2：
        //  sType：一个VkStructureType值，用于标识这个结构体的类型，必须是VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO。
        //  pNext：一个指针，用于指向一个扩展这个结构体的其他结构体，或者为NULL。
        //  flags：一个位掩码，用于指定额外的管线动态状态信息，目前没有定义任何有效的值，必须为0。
        //  dynamicStateCount：一个整数，用于指定pDynamicStates数组中的VkDynamicState值的数量。
        //  pDynamicStates：一个指针，用于指向一个VkDynamicState值数组，指定哪些管线状态是动态的，可以从动态状态命令中获取。
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
        //      VkDescriptorSetLayout创建所需要的信息VkDescriptorSetLayoutCreateInfo结构体:
        //      sType：一个VkStructureType值，用于标识这个结构体的类型，必须是VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO。
        //      pNext：一个指针，用于指向一个扩展这个结构体的其他结构体，或者为NULL。
        //      flags：一个位掩码，用于指定额外的描述符集布局信息，目前没有定义任何有效的值。
        //      bindingCount：一个整数，用于指定pBindings数组中的VkDescriptorSetLayoutBinding结构体的数量。
        //      pBindings：一个指针，用于指向一个VkDescriptorSetLayoutBinding结构体数组，定义了每个描述符绑定点的信息。
        //          VkDescriptorSetLayoutBinding结构体：
        //          binding：一个整数，用于指定这个绑定点的编号，它与着色器中使用的绑定号相对应。
        //          descriptorType：一个VkDescriptorType枚举值，用于指定这个绑定点的描述符类型，如缓冲、图像、采样器等。
        //          descriptorCount：一个整数，用于指定这个绑定点的描述符数量，如果大于1，则表示这个绑定点是一个数组，可以在着色器中使用下标来访问。
        //          stageFlags：一个位掩码，用于指定这个绑定点可以被哪些着色器阶段访问，如顶点、片段、计算等。
        //          pImmutableSamplers：一个指针，用于指定这个绑定点的不可变采样器数组，如果这个绑定点的描述符类型是采样器或者组合图像采样器，则可以使用这个字段来固定采样器对象，而不需要在描述符集中更新。
        //  pushConstantRangeCount：一个整数，用于指定pPushConstantRanges数组中的VkPushConstantRange结构体的数量。
        //  pPushConstantRanges：一个指针，用于指向一个VkPushConstantRange结构体数组，指定管线中使用的推送常量范围。
        //      每个推送常量范围可以定义一段内存空间，用于存储一些常量数据，可以在运行时通过vkCmdPushConstants命令来更新。
        //      VkPushConstantRange结构体:
        //      stageFlags：一个位掩码，用于指定这个推送常量范围可以被哪些着色器阶段访问。
        //      offset：一个整数，用于指定这个推送常量范围在管线布局中的偏移量，单位为字节。
        //      size：一个整数，用于指定这个推送常量范围在管线布局中的大小，单位为字节。
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
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