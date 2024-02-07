#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
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
    // 图像视图（image view）是一个对象，它描述了如何访问一个图像（image）的特定部分和格式。
    // 图像视图被渲染路径（render pass）和描述符集（descriptor set）使用，以便在渲染操作和着色器中使用图像
    // 一个图像视图规定了交换链中一个图像如何被使用与显示。
    // 图像视图可以决定交换链中图像的维度、格式、颜色通道、子资源范围等
    
    // 一个图像的子资源（subresource）是指图像的一部分，它具有一定的用途（aspect）、细化级别（mip level）和数组层（array layer）。一个图像可以有多个子资源，它们共享同一个图像数据存储空间1。
    
    // 一个图像是由颜色（color）、深度（depth）、模板（stencil）、细化级别（mip level）和图像层（array layer）组成的
    // 颜色（color）是指图像的每个像素的颜色值，它可以由不同的颜色通道（color channel）组成，如RGB、HSV、CMYK等。
    // 深度（depth）是指图像中每个像素到相机的距离，它可以用于实现深度测试（depth test），即判断哪些像素是可见的，哪些是被遮挡的4。
    // 模板（stencil）是指图像中每个像素的模板值，它可以用于实现模板测试（stencil test），即根据一些条件决定哪些像素可以被绘制或修改5。
    // 细化级别（mip level）是指图像的不同分辨率版本，它可以用于实现多级渐进纹理映射（mipmap），即根据观察距离选择合适的分辨率来渲染图像，从而提高性能和质量。
    // 图像层（array layer）是指图像的不同层数，它可以用于实现立方体贴图（cube map）、三维纹理（3D texture）或数组纹理（array texture），即将多个二维图像堆叠起来形成一个更复杂的图像结构。
    
    // VkImageView是一个非分派句柄VK_DEFINE_NON_DISPATCHABLE_HANDLE，它与可分派句柄VK_DEFINE_HANDLE有所不同
    // 非分派句柄（non-dispatchable handle）是一种用于标识Vulkan对象的数据类型，它是一个64位的整数，用于唯一标识一个对象。
    // 非分派句柄与可分派句柄有以下区别：
    //  非分派句柄定义的句柄是一个64位整数，不需要通过函数指针表来调用对象相关的函数，而是直接传递给函数作为参数，不用调用系统底层api，只需要vulkan内部api。
    //  非分派句柄通常用于表示不需要跨设备或跨实例共享的对象；如图像视图（image view）、缓冲区（buffer）等。
    //  可分派句柄定义的句柄是一个指针类型，它指向一个包含对象信息和函数指针表的结构体，可能需要调用系统底层api。
    //  可分派句柄通常用于表示需要跨设备或跨实例共享的对象，如设备（device）、实例（instance）等。

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
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
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
        // 为交换链中的每个图像分配一个图像视图对象的空间
        swapChainImageViews.resize(swapChainImages.size());
        // 图像视图创建需要一个VkImageViewCreateInfo结构体
        // 它有以下成员变量4：
        //  sType：用于指定结构体的类型，必须为VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO。
        //  pNext：用于指定扩展结构体的指针，一般为nullptr。
        //  flags：用于指定创建标志，目前保留为0。
        //  image：用于指定要创建视图的图像对象。
        //  viewType：用于指定视图的类型，可以是一维、二维、三维或立方体贴图等。
        //      它一个枚举类型，它用于指定图像视图（image view）的类型。它有以下八个值：
        //      VK_IMAGE_VIEW_TYPE_1D：表示一维图像视图，只有宽度一个维度。
        //      VK_IMAGE_VIEW_TYPE_2D：表示二维图像视图，有宽度和高度两个维度。
        //      VK_IMAGE_VIEW_TYPE_3D：表示三维图像视图，有宽度、高度和深度三个维度。
        //      VK_IMAGE_VIEW_TYPE_CUBE：表示立方体贴图（cube map）图像视图，由六个二维图像组成，每个图像对应立方体的一个面。
        //      VK_IMAGE_VIEW_TYPE_1D_ARRAY：表示一维数组图像视图，由多个一维图像组成，每个图像对应数组的一个层（layer）。
        //      VK_IMAGE_VIEW_TYPE_2D_ARRAY：表示二维数组图像视图，由多个二维图像组成，每个图像对应数组的一个层。
        //      VK_IMAGE_VIEW_TYPE_CUBE_ARRAY：表示立方体贴图数组图像视图，由多个立方体贴图组成，每个立方体贴图对应数组的一个层。
        //      VK_IMAGE_VIEW_TYPE_MAX_ENUM：表示枚举类型的最大值，不是一个有效的值。
        //  format：用于指定视图颜色的格式，可以和图像的格式一致或兼容。
        //  components：用于指定颜色通道的映射方式，可以对每个通道进行恒等、零、一或反转等操作。
        //      它有四个分量的VkComponentSwizzle(rbga)，VkComponentSwizzle是一个枚举类型，它用于指定颜色通道（color channel）的映射方式。
        //      VkComponentSwizzle有以下六个值：
        //      VK_COMPONENT_SWIZZLE_IDENTITY：表示恒等映射，即保持原来的分量不变。例如，R->R, G->G, B->B, A->A。
        //      VK_COMPONENT_SWIZZLE_ZERO：表示零映射，即将原来的分量替换为0。例如，R -> 0, G -> 0, B -> 0, A -> 0。
        //      VK_COMPONENT_SWIZZLE_ONE：表示一映射，即将原来的分量替换为1。例如，R -> 1, G -> 1, B -> 1, A -> 1。
        //      VK_COMPONENT_SWIZZLE_R：表示红映射，即将原来的分量替换为红分量。例如，R->R, G->R, B->R, A->R。
        //      VK_COMPONENT_SWIZZLE_G：表示绿映射，即将原来的分量替换为绿分量。例如，R->G, G->G, B->G, A->G。
        //      VK_COMPONENT_SWIZZLE_B：表示蓝映射，即将原来的分量替换为蓝分量。例如，R->B, G->B, B->B, A->B。
        //      VK_COMPONENT_SWIZZLE_A：表示透明度映射，即将原来的分量替换为透明度分量。例如，R->A, G->A, B->A, A->A。
        //  subresourceRange：用于指定子资源范围，包括要访问的图像的哪些分量（如颜色、深度、模板等）、哪些细化级别和哪些数组层。
        //      它是一个VkImageSubresourceRange结构体，它用于指定子资源范围（subresource range）。VkImageSubresourceRange有以下四个成员变量：
        //      aspectMask：用于指定要访问的图像的哪些分量，可以是颜色（color）、深度（depth）、模板（stencil）或者深度模板（depth stencil）。
        //          aspectMask是一个位域（bit field），它用于指定要访问的图像（image）的哪些分量;
        //          aspectMask可以是这八个值中的一些和，也就是可以同时访问图像的多个分量。比如，如果aspectMask为VK_IMAGE_ASPECT_COLOR_BIT |
        //          aspectMask有以下八个值2：
        //          VK_IMAGE_ASPECT_COLOR_BIT：表示要访问图像的颜色分量，即每个像素的颜色值。
        //          VK_IMAGE_ASPECT_DEPTH_BIT：表示要访问图像的深度分量，即每个像素到相机的距离。
        //          VK_IMAGE_ASPECT_STENCIL_BIT：表示要访问图像的模板分量，即每个像素的模板值。
        //          VK_IMAGE_ASPECT_METADATA_BIT：表示要访问图像的元数据分量，即一些额外的信息，如压缩格式等。
        //          VK_IMAGE_ASPECT_PLANE_0_BIT：表示要访问图像的第一个平面分量，用于多平面格式的图像，如YUV等。
        //          VK_IMAGE_ASPECT_PLANE_1_BIT：表示要访问图像的第二个平面分量，用于多平面格式的图像，如YUV等。
        //          VK_IMAGE_ASPECT_PLANE_2_BIT：表示要访问图像的第三个平面分量，用于多平面格式的图像，如YUV等。
        //          VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT：表示要访问图像的第一个内存平面分量，用于多内存格式的图像。
        //      baseMipLevel：用于指定要访问的图像的最低细化级别，从0开始计数。
        //      levelCount：用于指定要访问的图像的细化级别的数量，如果为VK_REMAINING_MIP_LEVELS，表示从baseMipLevel开始到最高细化级别结束。
        //      baseArrayLayer：用于指定要访问的图像的最低数组层，从0开始计数。
        //      layerCount：用于指定要访问的图像的数组层的数量，如果为VK_REMAINING_ARRAY_LAYERS，表示从baseArrayLayer开始到最高数组层结束。
        
        // 遍历交换链中的每个图像
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};// 创建一个图像视图创建信息结构体，用于指定图像视图的参数
            
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;// 设置结构体的类型
            createInfo.image = swapChainImages[i];                      // 设置要创建视图的图像对象
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;                // 设置视图的类型，这里是二维图像
            createInfo.format = swapChainImageFormat;                   // 设置视图的格式，这里和交换链的格式一致
            // 设置颜色通道的映射方式，这里是默认的恒等映射
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            // 设置子资源范围，用于指定要访问的图像的哪一部分和用途
            // 这里只访问整个图像的颜色分量，没有细化级别和数组层
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;     // 不开启mipmap
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;     // 图像只有一层

            // 调用vkCreateImageView函数，使用创建信息结构体和设备对象，创建一个图像视图对象，并存储到数组中
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
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