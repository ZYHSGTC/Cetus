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

    VkRenderPass renderPass;
    // VkRenderPass是Vulkan中的一个对象，用于描述一个渲染流程中的附件（attachment）、子通道（subpass）和依赖（dependency），以及它们之间的关系和转换。
    // VkRenderPass可以让驱动程序知道渲染的输入、输出和中间结果，从而进行优化和调度，提高性能和节省带宽。

    // 在硬件内存中，VkRenderPass的形式取决于GPU的架构类型。一般来说，有两种主要的GPU架构类型：立即模式渲染器（IMR）和基于图块的渲染器（TBR）。
    // 在IMR中，GPU会立即执行命令，并将结果写入帧缓冲区中。
    //      这种方式对内存带宽要求较高，但是对渲染顺序要求较低。
    //      在这种情况下，VkRenderPass可以帮助驱动程序重新调度命令，以避免不必要的同步和图像布局转换。
    // 在TBR中，GPU会将屏幕分割成多个小块（tile），并将每个tile的几何数据缓存到片上内存（on - chip memory）中。
    //      然后GPU会逐个处理每个tile，并将结果写入帧缓冲区中。这种方式对内存带宽要求较低，但是对渲染顺序要求较高。
    //      在这种情况下，VkRenderPass可以帮助驱动程序确定何时将数据进入或离开片上内存，并且判断是否需要将数据放置到内存或丢弃tile内的全部内容。
    VkPipelineLayout pipelineLayout;

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
        createRenderPass();
        createGraphicsPipeline();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

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

    void createRenderPass() {
        // 结构体VkAttachmentDescription用于描述一个渲染过程中使用的附件的属性，例如格式，采样数，加载和存储操作，布局转换等.
        //  有以下成员变量：
        //  flags：一个VkAttachmentDescriptionFlagBits枚举值，指定附件的额外属性。
        //  format：一个VkFormat枚举值，指定附件使用的图像视图的格式。
        //  samples：一个VkSampleCountFlagBits枚举值，指定附件使用的图像的采样数。
        //  loadOp：加载操作，指定了渲染开始时，附件的数据应该如何处理，对附件的加载操作是对于subpass而言的，只有subpass加载路径
        //      是一个VkAttachmentLoadOp枚举值，有以下可能的值：
        //      VK_ATTACHMENT_LOAD_OP_LOAD：加载操作，渲染开始时从内存中读取附件数据。
        //      VK_ATTACHMENT_LOAD_OP_CLEAR：清除操作，渲染开始时用一个常数值清除附件数据。
        //      VK_ATTACHMENT_LOAD_OP_DONT_CARE：不关心操作，渲染开始时不管附件数据，内容未定义。
        //      VK_ATTACHMENT_LOAD_OP_NONE_EXT：不访问图像在渲染区域内的内容，图像的内容是未定义的。
        //  storeOp：存储操作,指定了在渲染通道结束时对附件的处理方式。这决定了渲染过程中生成的像素数据是否保留到内存，以及保留到哪个内存区域。
        //      是一个VkAttachmentStoreOp枚举值,有以下几种可能的值：
        //      VK_ATTACHMENT_STORE_OP_STORE：保留图像在渲染区域内的内容。
        //      VK_ATTACHMENT_STORE_OP_DONT_CARE：不保留图像在渲染区域内的内容，图像的内容是未定义的。
        //      VK_ATTACHMENT_STORE_OP_NONE_EXT：不访问图像在渲染区域内的内容，图像的内容是未定义的。
        //  stencilLoadOp：一个VkAttachmentLoadOp枚举值，指定附件的模板分量在第一个使用它的子通道开始时的处理方式。
        //  stencilStoreOp：一个VkAttachmentStoreOp枚举值，指定附件的模板分量在最后一个使用它的子通道结束时的处理方式。
        //  
        //  initialLayout：一个VkImageLayout枚举值，初始布局，指定了randerpass渲染开始前，如何从内存中读取附件的图像布局
        //  finalLayout：一个VkImageLayout枚举值，最终布局，指定了randerpass渲染结束后，如何从内存中写入附件的图像布局。
        //      图像布局（VkImageLayout）指定了图像子资源在内存中的存储方式，一个图像子资源可以作为一个附件，但一个附件只能包含一个图像子资源。
        //      VkImageLayout：用于指定图像在渲染过程中或者其他操作中的布局转换，有以下几种可能的值：
        //      VK_IMAGE_LAYOUT_UNDEFINED：图像没有特定的布局，可以从任何其他布局转换到这个布局。
        //      VK_IMAGE_LAYOUT_GENERAL：图像可以被任何操作访问，但性能可能不佳。
        //      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL：图像作为颜色附件使用，只能被颜色附件访问。
        //      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL：图像作为深度模板附件使用，只能被深度模板附件访问。
        //      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL：图像作为只读深度模板附件使用，只能被深度模板附件或着色器访问。
        //      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL：图像作为只读纹理使用，只能被着色器访问。
        //      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL：图像作为传输源使用，只能被传输操作访问。
        //      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL：图像作为传输目标使用，只能被传输操作访问。
        //      VK_IMAGE_LAYOUT_PREINITIALIZED：图像已经被主机初始化，可以从这个布局转换到其他布局。
        //      VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL：图像作为只读深度和可写模板附件使用，只能被深度模板附件或着色器访问。
        //      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL：图像作为可写深度和只读模板附件使用，只能被深度模板附件或着色器访问。
        //      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR：图像作为交换链呈现源使用。
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;                      // 设置附件的格式，与交换链图像的格式相同
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                    // 设置附件的采样数，这里为单重采样
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               // 设置附件的加载操作，这里为清除操作，表示渲染开始时用一个常数值清除附件数据
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             // 设置附件的存储操作，这里为存储操作，表示渲染结束时将附件数据写入内存中
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    // 设置附件的模板加载操作，这里为不关心操作，表示渲染开始时不管模板数据，内容未定义
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // 设置附件的模板存储操作，这里为不关心操作，表示渲染结束时不管模板数据，内容未定义
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // 设置附件的初始布局，这里为未定义的布局，表示不关心之前的布局，也不保留数据
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      // 设置附件的最终布局，这里为呈现源优化的布局，表示用于呈现到屏幕上

        // 定义了附件还需要定义附件引用的原因是：
        //  附件描述只是定义了附件的一般属性，而不是指定了附件在渲染流程中的具体用途 。
        //  附件引用是指定了附件在子流程中的具体用途，例如作为颜色附件、深度模板附件、输入附件等 。
        //  附件引用还指定了附件在子流程中的具体布局，例如颜色附件最优、深度模板附件最优等 。
        //  通过定义附件引用，可以让Vulkan知道如何在子流程中使用和转换附件 。
        // 结构体VkAttachmentReference用于设定renderpass的subpass的引用附件，这些附件是renderpass所有附件中的一个；
        //  attachment：一个整数值，表示在VkRenderPassCreateInfo::pAttachments中对应索引处的附件
        //      值为VK_ATTACHMENT_UNUSED表示该附件未被使用。
        //  layout：一个VkImageLayout枚举值，指定附件在子通道中使用的布局。
        //      一般来说，VkAttachmentReference中的layout布局应该与VkAttachmentDescription中的initialLayout或者finalLayout中的一个一致，具体取决于附件是在渲染通道中第一次使用还是最后一次使用。
        //      如果一个附件在渲染通道中只有一个子通道使用，那么它的layout布局应该与finalLayout一致，这样就可以避免额外的布局转换3。
        //      如果一个附件在渲染通道中有多个子通道使用，那么它的layout布局应该与initialLayout一致，这样就可以保证在第一个子通道开始时已经完成了布局转换3
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // 子流程是渲染流程中的一个阶段，它定义了一组渲染操作和使用的附件。子流程可以相互连接，形成一个完整的渲染流程。
        //  子流程与渲染流程有密切的关系，因为一个渲染流程由一个或多个子流程组成。子流程可以共享同一个渲染通道对象和帧缓冲对象，以及同一组附件。
        //  子流程还可以在同一张图像上进行多次渲染，实现一些高级的渲染技术，例如延迟着色。
        //  每个子流程都会使用一次渲染管线，因为每个子流程都需要对输入的数据进行处理和输出。不同的子流程可以使用不同的渲染管线设置，例如管线类型、着色器、混合模式等。
        // 
        // 子流程有以下必要的组成部分 :
        //  绑定点：指定了子流程使用的管线类型，例如图形管线或计算管线。
        //  颜色附件引用：指定了子流程使用的颜色附件，以及它们在子流程中的布局。
        //  深度模板附件引用：指定了子流程使用的深度模板附件，以及它在子流程中的布局。
        //  输入附件引用：指定了子流程使用的输入附件，以及它们在子流程中的布局。
        //  保留附件引用：指定了子流程不使用但需要保留的附件，以及它们在子流程中的布局。
        //  管线布局：指定了子流程使用的管线布局，包括描述符集布局和推送常量范围。
        // 
        // 子流程在gpu中具体做了以下几件事：
        //  从帧缓冲对象（framebuffer object）中获取附件作为输入，或者从前一个子流程的输出中获取输入附件（input attachment）。
        //  根据子流程描述（subpass description）中指定的绑定点（binding point），选择使用图形管线（graphics pipeline）或者计算管线（compute pipeline）。
        //  根据子流程描述中指定的管线布局（pipeline layout），绑定相应的描述符集（descriptor set）和推送常量（push constant），以提供着色器所需的数据。
        //  根据子流程描述中指定的颜色附件引用（color attachment reference）和深度模板附件引用（depth stencil attachment reference），设置渲染目标（render target），即输出的图像位置。
        //  执行渲染管线或者计算管线中的各个阶段，对输入的数据进行处理，生成输出的图像数据。
        //  根据子流程描述中指定的保留附件引用（preserve attachment reference），保留不需要输出的附件，以便后续子流程使用。
        // 
        // 子流程的输入是一组附件（attachment），它们是渲染过程中使用的图像，例如颜色缓冲、深度缓冲、模板缓冲等。
        //      子流程的输入还包括一些输入附件（input attachment），它们是在前一个子流程中作为输出附件使用过的图像，可以在当前子流程中作为输入使用。
        // 子流程的输出是一组附件，它们包含了渲染操作后的图像数据，可以用于显示或者作为其他渲染流程的输入。
        //      子流程的输出还包括一些保留附件（preserve attachment），它们是在当前子流程中不需要使用，但在后续子流程中可能需要使用的图像。
        // 
        //  子流程会使用整个渲染管线，是因为每个子流程都需要对输入的数据进行处理和输出。
        //      不同的子流程可以使用不同的渲染管线设置，例如管线类型、着色器、混合模式等。
        //      阶段掩码用于指定哪些管线阶段需要等待或被等待，以保证正确的执行顺序和内存访问。
        //      例如，如果源子流程需要在颜色附件输出阶段完成，而目标子流程需要在顶点着色器阶段开始，
        //          那么就需要设置源阶段掩码为VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT，目标阶段掩码为VK_PIPELINE_STAGE_VERTEX_SHADER_BIT。
        // 
        // 
        // 依赖的源子流程是指在依赖关系中先执行的子流程。如果依赖的源子流程是VK_SUBPASS_EXTERNAL，那么表示它不是渲染流程中的任何一个子流程，而是渲染流程开始前或结束后的外部命
        // 子流程依赖是指两个子流程之间的同步机制，它可以保证第一个子流程完成后，第二个子流程才开始。子流程依赖可以指定源子流程和目标子流程的阶段掩码和访问掩码，以及一些额外的选项12。
        // 源子流程与目标子流程是什么意思？
        //  源子流程（source subpass）是指在子流程依赖关系中先执行的子流程。
        //  目标子流程（destination subpass）是指在子流程依赖关系中后执行的子流程。
        //  子流程依赖关系（subpass dependency）是指两个子流程之间的同步机制，它可以保证第一个子流程完成后，第二个子流程才开始。
        //      子流程依赖关系可以指定源子流程和目标子流程的阶段掩码和访问掩码，以及一些额外的选项。
        // 渲染依赖（render dependency）是指两个渲染流程之间的同步机制，它可以保证第一个渲染流程完成后，第二个渲染流程才开始。
        //  渲染依赖可以分为两种类型：隐式渲染依和显式渲染依赖。
        //      隐式渲染依赖是指Vulkan自动创建的渲染依赖，它可以保证同一个队列中的相邻渲染流程按照提交顺序执行。隐式渲染依赖不需要用户指定任何参数，但是它可能不是最优化的。
        //      显式渲染依赖是指用户手动创建的渲染依赖，它可以保证不同队列中或者不相邻的渲染流程按照用户定义的顺序执行。
        //          显式渲染依赖需要用户指定一些参数，例如源和目标队列族索引、源和目标阶段掩码、源和目标访问掩码、源和目标附件布局等。显式渲染依赖可以提供更好的性能和灵活性。
        //      渲染依赖到底什么时候起作用，取决于它们的类型和参数。
        //          一般来说，隐式渲染依赖在同一个队列中的相邻渲染流程之间起作用，而显式渲染依赖在不同队列中或者不相邻的渲染流程之间起作用。
        //          渲染依赖改变与设置的到底是什么对象，取决于它们的参数。一般来说，渲染依赖改变与设置的对象包括以下几种：
        //          阶段（stage）：表示管线中执行特定操作的一个步骤。阶段可以分为不同的类型，例如顶点着色器阶段、片元着色器阶段、颜色附件输出阶段等。
        //          访问（access）：表示对内存或资源进行读取或写入操作的一种类型。访问可以分为不同的类型，例如颜色附件写入访问、深度模板附件读取访问、着色器读取访问等。
        //          附件（attachment）：表示渲染过程中使用的图像，例如颜色缓冲、深度缓冲、模板缓冲等。
        //          布局（layout）：表示附件在内存中的组织方式，以及它们在不同阶段中的可用性。布局可以分为不同的类型，例如颜色附件最优布局、深度模板附件最优布局、呈现源布局等。

        // 外部命令是指在渲染流程之外执行的Vulkan命令，例如vkAcquireNextImageKHR或vkQueuePresentKHR12。外部命令可能需要与渲染流程中使用的附件进行同步，因此需要设置子流程依赖
        // 命令缓冲区除了有渲染管线的命令还有什么其他类型的命令？
        //  命令缓冲区（command buffer）是一个存储Vulkan命令的对象，可以在设备上执行。
        //  命令缓冲区除了有渲染管线的命令，还有其他类型的命令，例如：
        //      拷贝命令（copy command），用于在不同的内存位置之间复制数据，例如从CPU到GPU或者从一个图像到另一个图像。
        //      计算命令（compute command），用于执行计算管线（compute pipeline），它是一种用于执行通用并行计算操作的管线类型。
        //      同步命令（synchronization command），用于在不同的命令之间建立依赖关系，以保证正确的执行顺序和内存访问。

        // 结构体VkSubpassDescription有以下成员变量3：
        //  flags：保留供将来使用。
        //  pipelineBindPoint：一个VkPipelineBindPoint枚举值，指定该子通道绑定到图形管线还是计算管线。
        //      VK_PIPELINE_BIND_POINT_GRAPHICS：指定绑定为图形管线。图形管线用于执行绘制命令，包括顶点、几何、曲面细分、光栅化、片段等阶段。
        //      VK_PIPELINE_BIND_POINT_COMPUTE：指定绑定为计算管线。计算管线用于执行分发命令，只包含一个计算阶段。
        //      VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR：指定绑定为光线追踪管线。光线追踪管线用于执行光线追踪命令，包括射线生成、任意命中、最近命中、没有命中和可调用等阶段。
        //  inputAttachmentCount：该子通道使用的输入附件的数量。
        //  pInputAttachments：一个指向VkAttachmentReference结构体数组的指针，描述该子通道使用的输入附件。
        //      输入附件（input attachment）是一种可以在片段着色器中读取的附件.
        //      通常用于从上一个子通道（subpass）相同片段位置上写入的帧缓冲区附件读取数据。
        //      输入附件可以提高性能和节省带宽，特别是在基于图块的渲染器（TBR）上。
        //      输入附件只能作为着色器输入使用，不能作为颜色或深度模板输出使用。
        //      颜色附件和深度模板附件可以作为输入使用，但是这需要在不同的子通道（subpass）之间进行图像布局转换（image layout transition）。
        //          图像布局转换是一种昂贵的操作，它会影响性能和带宽。而输入附件（input attachment）可以避免这种转换，
        //          因为它们可以在同一个子通道中作为输出和输入使用。这样就可以节省资源和时间，提高渲染效率。
        //      输入附件的另一个优势是，它们可以在基于图块的渲染器（TBR）上发挥更好的作用。
        //          基于图块的渲染器是一种将屏幕分割成多个小块（tile）来渲染的架构，它可以利用片上内存（on - chip memory）来缓存每个图块的颜色和深度数据。
        //          这样就可以减少对全局内存（global memory）的访问，从而节省功耗和带宽。
        //          输入附件可以直接从片上内存中读取数据，而不需要从全局内存中加载或存储数据。这对于移动设备来说是非常有益的。
        //  colorAttachmentCount：该子通道使用的颜色附件和解析附件的数量。
        //  pColorAttachments：一个指向VkAttachmentReference结构体数组的指针，描述该子通道使用的颜色附件。
        //      颜色附件（color attachment）是一种可以作为颜色输出的附件，通常用于存储片段着色器输出的颜色值。
        //      颜色附件可以有多个，每个对应一个颜色缓冲区（color buffer）。
        //      颜色附件可以作为着色器输入或输出使用，也可以作为传输源或目标使用。
        //  pResolveAttachments：一个指向VkAttachmentReference结构体数组的指针，描述该子通道使用的解析附件。如果不需要解析操作，则为NULL。
        //      解析附件（resolve attachment）是一种用于多重采样（multisampling）的附件..
        //      通常用于将多重采样的颜色或深度模板附件解析为单重采样的图像。
        //      解析附件只能作为输出使用，不能作为输入使用。
        //  pDepthStencilAttachment：一个指向VkAttachmentReference结构体的指针，描述该子通道使用的深度模板附件。如果不需要深度模板操作，则为NULL。
        //      深度模板附件（depth stencil attachment）是一种用于深度和模板测试的附件。
        //      通常用于存储深度值和模板值。深度模板附件只能有一个，对应一个深度模板缓冲区（depth stencil buffer）。
        //      深度模板附件可以作为着色器输入或输出使用，也可以作为传输源或目标使用。
        //  preserveAttachmentCount：该子通道保留但不使用或修改的附件的数量。
        //  pPreserveAttachments：一个指向uint32_t数组的指针，表示在VkRenderPassCreateInfo::pAttachments中对应索引处保留但不使用或修改的附件。
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;            // 这里只设定了颜色附件

        //  renderpass是一个现代渲染API提出的概念，它可以理解为一次完整的渲染流程，包括使用的所有资源的描述和操作方式。
        //      vulkan中必须有renderpass才能进行渲染，而且每个renderpass必须有一个或多个子步骤（subpass），每个subpass对应一个图形管线（pipeline）。
        //      子流程依赖于上一流程处理后的帧缓冲内容。
        //      renderpass需要的创建信息主要包括以下几个方面：
        //      一组附件（attachment），指定了颜色、深度、模板等缓冲区的格式和用途。
        //      一组附件引用（attachment reference），指定了每个subpass使用哪些附件作为输入或输出。
        //      一组subpass，指定了每个subpass的类型（图形或计算）、绑定的管线、依赖的附件等。
        //      一组subpass依赖（subpass dependency），指定了不同subpass之间的数据依赖关系和同步方式。
        //  渲染流程最开始的输入是一组附件描述（attachment description），它们指定了每个附件的格式、采样数、加载和存储操作、初始和最终布局等信息。
        //  渲染流程最后的输出是一组附件，它们包含了渲染操作后的图像数据，可以用于显示或者作为其他渲染流程的输入
        //  渲染流程和渲染管线的关系是：渲染流程定义了渲染管线如何使用帧缓冲对象中的图像进行渲染。渲染流程可以在命令缓冲对象中开始和结束，以执行一系列图形命令5。
        
        // 结构体VkRenderPassCreateInfo有以下成员变量3：
        //  sType：结构体类型，必须为VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO。
        //  pNext：保留供将来使用或扩展结构体功能。
        //  flags：保留供将来使用。
        //  attachmentCount：渲染过程使用的附件数量。
        //  pAttachments：一个指向VkAttachmentDescription结构体数组的指针，描述渲染过程使用的附件属性。
        //  subpassCount：要创建的子通道数量。
        //  pSubpasses：一个指向VkSubpassDescription结构体数组的指针，描述每个子通道的操作。
        //  dependencyCount：子通道间的内存依赖关系的数量。
        //      subpass依赖（subpass dependency）是Vulkan中一种用于指定不同子通道（subpass）之间的数据依赖关系和同步方式的结构体。
        //      subpass依赖可以让应用程序明确地告诉驱动程序哪些子通道需要等待或通知其他子通道，以及在何时进行图像布局转换（image layout transition）
        //  pDependencies：一个指向VkSubpassDependency结构体数组的指针，描述子通道间的内存依赖关系。
        //      VkSubpassDependency结构体包含以下信息：
        //      srcSubpass：指定了源子通道的索引，表示在该子通道完成后才能开始目标子通道。如果为VK_SUBPASS_EXTERNAL，表示在渲染通道开始前或结束后。
        //      dstSubpass：指定了目标子通道的索引，表示在该子通道开始前必须等待源子通道完成。如果为VK_SUBPASS_EXTERNAL，表示在渲染通道开始前或结束后。
        //      srcStageMask：指定了源子通道需要等待的管线阶段掩码，表示在该阶段完成后才能开始目标子通道。
        //      dstStageMask：指定了目标子通道需要等待的管线阶段掩码，表示在该阶段开始前必须等待源子通道完成。
        //          VkPipelineStageFlagBits是一个枚举类型，用于指定管线阶段（pipeline stage），它有以下值:
        //          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT：表示管线开始时的阶段，没有任何操作。
        //          VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT：表示执行间接绘制命令（如vkCmdDrawIndirect）时的阶段。
        //          VK_PIPELINE_STAGE_VERTEX_INPUT_BIT：表示处理顶点输入数据时的阶段。
        //          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT：表示执行顶点着色器时的阶段。
        //          VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT：表示执行曲面细分控制着色器时的阶段。
        //          VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT：表示执行曲面细分评估着色器时的阶段。
        //          VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT：表示执行几何着色器时的阶段。
        //          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT：表示执行片元着色器时的阶段。
        //          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT：表示执行早期片元测试（如深度测试）时的阶段。
        //          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT：表示执行晚期片元测试（如模板测试）时的阶段。
        //          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT：表示写入颜色附件时的阶段。
        //          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT：表示执行计算着色器时的阶段。
        //          VK_PIPELINE_STAGE_TRANSFER_BIT：表示执行传输操作（如拷贝、清除）时的阶段。
        //          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT：表示管线结束时的阶段，没有任何操作。
        //          VK_PIPELINE_STAGE_HOST_BIT：表示主机访问内存或队列时的阶段。
        //          VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT：表示所有图形相关的阶段，相当于把所有图形管线相关的位都设置为1。
        //          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT：表示所有命令相关的阶段，相当于把所有位都设置为1。
        //      srcAccessMask：指定了源子通道需要等待的访问类型掩码，表示在该类型的访问完成后才能开始目标子通道。
        //      dstAccessMask：指定了目标子通道需要等待的访问类型掩码，表示在该类型的访问开始前必须等待源子通道完成。
        //          VkAccessFlagBits是一个枚举类型，用于指定内存访问类型（memory access type），它有以下值:
        //          VK_ACCESS_INDIRECT_COMMAND_READ_BIT：表示读取间接绘制命令数据时的访问类型。
        //          VK_ACCESS_INDEX_READ_BIT：表示读取索引缓冲数据时的访问类型。
        //          VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT：表示读取顶点属性数据时的访问类型。
        //          VK_ACCESS_UNIFORM_READ_BIT：表示读取统一变量数据时的访问类型。
        //          VK_ACCESS_INPUT_ATTACHMENT_READ_BIT：表示读取输入附件数据时的访问类型。
        //          VK_ACCESS_SHADER_READ_BIT：表示着色器读取数据时的访问类型。
        //          VK_ACCESS_SHADER_WRITE_BIT：表示着色器写入数据时的访问类型。
        //          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT：表示读取颜色附件数据时的访问类型。
        //          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT：表示写入颜色附件数据时的访问类型。
        //          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT：表示读取深度模板附件数据时的访问类型。
        //          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT：表示写入深度模板附件数据时的访问类型。
        //          VK_ACCESS_TRANSFER_READ_BIT：表示传输操作读取数据时的访问类型。
        //          VK_ACCESS_TRANSFER_WRITE_BIT：表示传输操作写入数据时的访问类型。
        //          VK_ACCESS_HOST_READ_BIT：表示主机读取数据时的访问类型。
        //          VK_ACCESS_HOST_WRITE_BIT：表示主机写入数据时的访问类型。
        //          VK_ACCESS_MEMORY_READ_BIT：表示任何读取内存操作时的访问类型。
        //          VK_ACCESS_MEMORY_WRITE_BIT：表示任何写入内存操作时的访问类型。
        //      dependencyFlags：指定了一些可选的依赖标志，比如VK_DEPENDENCY_BY_REGION_BIT表示依赖只发生在相同区域内。
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;   // 设置结构体的类型，表示这是一个渲染通道创建信息
        renderPassInfo.attachmentCount = 1;                                 // 设置渲染通道使用的附件（attachment）的数量，这里为1
        renderPassInfo.pAttachments = &colorAttachment;                     // 设置渲染通道使用的附件的描述
        renderPassInfo.subpassCount = 1;                                    // 设置渲染通道包含的子通道（subpass）的数量，这里为1
        renderPassInfo.pSubpasses = &subpass;                               // 设置渲染通道包含的子通道的描述

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

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

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