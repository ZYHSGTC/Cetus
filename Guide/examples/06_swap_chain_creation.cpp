#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <algorithm>    //clmap需要用到
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
    // VK_KHR_SWAPCHAIN_EXTENSION_NAME是Vulkan的一个设备扩展，它允许你创建交换链对象，用于将渲染结果呈现到一个表面上。
    // 交换链对象需要显式地创建，不存在默认的交换链。
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


struct SwapChainSupportDetails {                // 定义一个结构体，用于存储表面支持的信息
    VkSurfaceCapabilitiesKHR capabilities;      // 表面的能力，比如最大最小图像数量、宽高等
    // VkSurfaceCapabilitiesKHR结构体用于描述一个表面的基本能力，比如支持的图像数量、尺寸、层数、变换、复合Alpha和用途等1。它的各个成员变量的含义如下：
    //  minImageCount是指定设备支持的交换链中图像的最小数量，至少为1。
    //          交换链中的图像是用于在表面上显示帧的缓冲区集合。交换链中的第一个缓冲区将替代已显示的缓冲区，这个过程称为交换;
    //  maxImageCount是指定设备支持的交换链中图像的最大数量，如果为0，表示没有限制，但可能受到内存使用的影响。
    //  currentExtent是表面当前的宽高，或者一个特殊值(0xFFFFFFFF, 0xFFFFFFFF)，extent是面积或者范围的意思，表示表面大小由目标交换链决定。
    //  minImageExtent是表面支持的交换范围（即图像尺寸）的最小值，宽高都不超过currentExtent，除非currentExtent是特殊值。
    //  maxImageExtent是表面支持的交换范围（即图像尺寸）的最大值，宽高都不小于currentExtent和minImageExtent。
    //          currentExtent和maxImageExtent的关系是，前者表示实际使用的图像大小，后者表示最大可能的图像大小(屏幕大小)。
    //          应用程序可以根据自己的需求选择合适的图像大小，但必须保证它在maxImageExtent之内，并与窗口表面一致或可自适应。
    //  maxImageArrayLayers是表面支持的呈现图像的最大层数，至少为1。
    //  supportedTransforms是一个VkSurfaceTransformFlagBitsKHR的位掩码，表示指定设备支持的表面变换，至少有一个位被设置。
    //          它有以下八个可能的值1：
    //          VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR：不需要任何变换，保持原样。
    //          VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR：顺时针旋转90度。
    //          VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR：顺时针旋转180度。
    //          VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR：顺时针旋转270度。
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR：水平镜像。
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR：水平镜像后再顺时针旋转90度。
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR：水平镜像后再顺时针旋转180度。
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR：水平镜像后再顺时针旋转270度。
    //  currentTransform是一个VkSurfaceTransformFlagBitsKHR值，表示表面相对于呈现引擎自然方向的当前变换。
    //          如果它与vkGetPhysicalDeviceSurfaceCapabilitiesKHR返回的值不匹配，呈现引擎会在呈现操作中对图像内容进行变换。
    //          它必须是supportedTransforms中的一个值。如果不需要进行任何变换，可以使用currentTransform的默认值，即表面的自然方向。
    //          VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR：这个值表示图像不需要任何变换，保持原样。
    //          VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR：这个值表示图像需要顺时针旋转90度。
    //          VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR：这个值表示图像需要顺时针旋转180度。
    //          VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR：这个值表示图像需要顺时针旋转270度。
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR：这个值表示图像需要水平镜像。
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR：这个值表示图像需要水平镜像后再顺时针旋转90度。
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR：这个值表示图像需要水平镜像后再顺时针旋转180度。
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR：这个值表示图像需要水平镜像后再顺时针旋转270度。
    //  supportedCompositeAlpha是一个VkCompositeAlphaFlagBitsKHR的位掩码，表示指定设备支持的表面复合Alpha模式，至少有一个位被设置。
    //          supportedCompositeAlpha 是一个设置交换链的选项，它决定了当多个图像叠加在一起时，它们的透明度是怎么处理的。它有以下几种选择1：
    //          复合Alpha可以通过使用没有Alpha分量的图像格式，或者确保所有呈现图像中的像素都有Alpha值为1.0来实现不透明复合。
    //          VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR：这个选项表示图像不透明，不管它们有没有透明度。
    //          VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR：这个选项表示图像的透明度会影响叠加的效果，但是图像的颜色rbg已经和透明度混合过了(相乘)。
    //          VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR：这个选项表示图像的透明度会影响叠加的效果，但是图像的颜色rbg还没有和透明度混合过，需要呈现引擎来混合它们。
    //          VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR：这个选项表示呈现引擎会自己决定怎么处理图像的透明度，Vulkan API 不知道具体的方法。
    //              应用程序需要用其他的命令来设置叠加的效果，如果没有设置，就会用默认的方法。
    //  supportedUsageFlags是一个VkImageUsageFlags的位掩码，表示指定设备支持的表面图像用途。
    //          VK_IMAGE_USAGE_TRANSFER_SRC_BIT：图像可以用作传输操作的源。   SRC：source，意思是源，来源，原因
    //          VK_IMAGE_USAGE_TRANSFER_DST_BIT：图像可以用作传输操作的目标。  DST：destination，意思是目的地，目标，终点
    //          VK_IMAGE_USAGE_SAMPLED_BIT：图像可以用作着色器中采样操作的对象。
    //          VK_IMAGE_USAGE_STORAGE_BIT：图像可以用作着色器中存储操作的对象。
    //          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT：图像可以用作颜色附件。
    //          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT：图像可以用作深度 / 模板附件。
    //          VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT：图像是短暂附件。
    //          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT：图像可以用作输入附件。

    // vulkan呈现引擎是指负责将vulkan交换链中的图像显示在窗口或屏幕上的软件或硬件组件。它可以是操作系统的一部分，也可以是独立的驱动程序或库。
    //          它的作用是将vulkan图像转换为表面可以接受的格式，并进行一些必要的变换、合成和同步操作。
    //          不同的平台和设备可能有不同的呈现引擎，它们对vulkan图像的处理方式也可能不同。
    //          因此，vulkan提供了一些参数和标志位，让应用程序可以查询和指定呈现引擎支持的特性和模式，以保证图像能够正确地显示在表面上。
    std::vector<VkSurfaceFormatKHR> formats;    // 表面支持的格式列表，每个格式包括颜色空间和色彩格式
    // VkSurfaceFormatKHR结构体用于描述一个支持的交换链格式-颜色空间对2。它的各个成员变量的含义如下：
    //          format是一个VkFormat值，表示与指定表面兼容的格式。
    //              格式是一种定义数据的方式，不同的格式可以表示不同的数据类型和布局。VkFormat有多个可能的值，比如
    //              VK_FORMAT_R8G8B8A8_UNORM表示一个四分量的无符号归一化格式，每个分量占8位，
    //              VK_FORMAT_D16_UNORM表示一个单分量的无符号归一化深度格式，占16位等。
    //          colorSpace是一个VkColorSpaceKHR值，表示与指定表面兼容的颜色空间。
    //              颜色空间是一种定义颜色的方式，不同的颜色空间可以表示不同的颜色范围和效果。VkColorSpaceKHR有多个可能的值，
    //              VK_COLOR_SPACE_SRGB_NONLINEAR_KHR表示sRGB颜色空间，
    //              VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT表示Display - P3颜色空间等1
    std::vector<VkPresentModeKHR> presentModes; // 表面支持的呈现模式列表，比如FIFO、MAILBOX等   
    // 交换链是一种用于管理多个缓冲区的对象，它可以实现双缓冲或三缓冲等技术，提高图形性能和流畅度。
    //      交换链中有两种类型的图像：颜色附件图像和呈现目标图像。
    //      颜色附件图像是用于渲染管线输出的图像，呈现目标图像是用于显示器输入的图像。
    //      呈现模式就是指交换链如何将颜色附件图像转换为呈现目标图像，并将其呈现到表面上。

    // 常见的呈现模式有三种：立即模式、邮箱模式和FIFO模式。(我的设备只支持立即模式与FIFO模式
    //      立即模式：当交换链收到一个新的颜色附件图像时，立即将其替换为当前的呈现目标图像，并将其呈现到表面上。
    //          这种模式可能会导致画面撕裂，因为显示器可能在刷新过程中收到新的图像。
    //      邮箱模式：当交换链收到一个新的颜色附件图像时，将其放入一个队列中，队列中只保留最新的一个图像。
    //          当显示器需要一个新的图像时，从队列中取出最新的一个，并将其呈现到表面上。
    //          这种模式可以避免画面撕裂，但可能会导致画面延迟，因为显示器可能无法及时获取最新的图像。
    //      FIFO模式：当交换链收到一个新的颜色附件图像时，将其放入一个队列中，队列中可以保留多个图像。
    //          当显示器需要一个新的图像时，从队列中按照先进先出的顺序取出一个，并将其呈现到表面上。
    //          这种模式也可以避免画面撕裂，但可能会导致画面卡顿，因为显示器可能无法跟上渲染管线的速度。
    
    // VkPresentModeKHR枚举类用于表示一个支持的呈现模式3。它有以下几个可能的值：
    //          VK_PRESENT_MODE_IMMEDIATE_KHR：这种模式下，当您请求交换链显示图像时，它会立即替换当前的表面图像，而不管表面是否处于垂直同步（vertical synchronization）状态。
    //              这种模式可能导致撕裂现象（tearing），但也有最低的延迟。
    //          VK_PRESENT_MODE_FIFO_KHR：这种模式下，交换链中的图像被当作一个先进先出（FIFO）的队列处理，当您请求交换链显示图像时，
    //              它会将队列中的第一个图像显示在表面上，并将新的图像插入队列尾部。
    //              这种模式保证了垂直同步，避免了撕裂现象，但也可能导致延迟和卡顿。
    //          VK_PRESENT_MODE_FIFO_RELAXED_KHR：这种模式下，当您请求交换链显示图像时，如果队列为空（即没有可用的图像），
    //              它会将新的图像显示在表面上，而不管表面是否处于垂直同步状态。如果队列不为空，它会按照FIFO模式工作。
    //              这种模式在应用程序帧率低于显示器刷新率时有用，可以避免卡顿，但也可能导致撕裂现象。
    //          VK_PRESENT_MODE_MAILBOX_KHR：这种模式下，当您请求交换链显示图像时，如果队列已满（即达到了最大图像数量），
    //              它会将新的图像替换队列中的最后一个图像，并将队列中的第一个图像显示在表面上。
    //              这种模式可以实现三重缓冲（triple buffering），在保证垂直同步和避免撕裂现象的同时，也有较低的延迟
    //          VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR表示呈现引擎和应用程序共享对交换链图像的访问权限。
    //              应用程序可以通过vkAcquireNextImageKHR获取一个图像，并且只有当应用程序调用vkQueuePresentKHR时，图像才会被显示。
    //              如果没有新的呈现请求，当前图像会保持不变。
    //          VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR表示呈现引擎和应用程序共享对交换链图像的访问权限。
    //              应用程序可以通过vkAcquireNextImageKHR获取一个图像，并且只要应用程序保持对该图像的访问权限，该图像就会被连续地显示。
    //              如果没有新的呈现请求，当前图像会保持不变。

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
    // 交换链是一个管理和呈现图像缓冲区的对象，与窗口表面和呈现引擎相关联。用于将渲染结果显示在窗口上。
    //      交换链中的图像缓冲区由Vulkan实现或平台创建，创建交换链需要一个逻辑设备和一个与之兼容的表面。
    //      应用程序可以从交换链中获取一个图像缓冲区，对其进行渲染操作，然后将其提交给表面进行呈现。
    //      交换链句柄内有一个指向交换链中所有图像缓冲区的指针数组，可以通过vkGetSwapchainImagesKHR函数获取。
    
    // 交换链，叫做交换，是因为它的工作原理是在屏幕缓冲区（screen buffer）和后向缓冲区（back buffer）之间交换图像。
    //          屏幕缓冲区是显示在视频卡输出的图像，后向缓冲区是存储下一个要显示的图像的地方。
    // 交换链，叫做链，是因为它是由一系列虚拟帧缓冲区组成的，每个帧缓冲区都可以作为屏幕缓冲区或后向缓冲区。
    
    // 交换链图像
    //      交换链图像与图像缓冲有一对一的对应关系，每个交换链图像都有一个对应的图像缓冲区，用于存储图像数据。
    //      交换链图像有两种类型的图像：颜色附件图像和呈现目标图像。
    //          颜色附件图像是用于渲染管线输出的图像，呈现目标图像是用于显示器输入的图像。
    //      交换链图像的访问，需要从交换链获取一个可用的图像索引，然后使用一个与之兼容的队列族来提交包含渲染命令的命令缓冲区（command buffer）。
    //          在渲染完成后，您需要将图像返回给交换链，并请求交换链将图像显示在表面上。
    //          不同的队列族可以访问交换链中的图像，但需要满足一些条件，例如支持表面的呈现能力、使用互斥信号量（semaphore）进行同步等。
    // 交换链图象数不把层数算在内，交换链图像数指的是交换链中包含的图像缓冲区的数量，它与图像缓冲区中的层数（layer count）没有关系。
    //          层数是指图像缓冲区中可以存储多少个二维图像，它用于实现一些特殊的效果，如立体视觉（stereoscopic vision）或多视角渲染（multiview rendering）。
    //          层数是由图像缓冲区的创建参数指定的，与交换链无关。

   
    // 交换链可以实现双缓冲或三缓冲等技术，提高图形性能和流畅度
    // 交换链在opengl中有对应。opengl中也有类似于交换链的机制，称为双缓冲（double buffering）。
    //          双缓冲也是使用两个帧缓冲区来提高渲染效率和稳定性，一个作为屏幕缓冲区，一个作为后向缓冲区。
    //          opengl中使用glSwapBuffers函数来实现帧缓冲区的交换。

    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

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
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
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
        // 用于创建交换链对象
        // 查询物理设备和表面的交换链支持细节，包括表面特性、格式和呈现模式
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        // 从交换链支持的格式列表中选择一个合适的表面格式，包括像素格式和颜色空间
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        // 从交换链支持的呈现模式列表中选择一个合适的呈现模式，决定了图像在屏幕上显示的方式
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        // 从交换链支持的表面特性中选择一个合适的分辨率，决定了图像缓冲区的大小
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // 根据交换链支持的最小图像数量加一来确定交换链中图像缓冲区的数量，如果超过了最大图像数量，则取最大值
        // 根据交换链支持的最小图像数量加一来确定交换链中图像缓冲区的数量，它可以保证至少有一个图像缓冲区可以用于渲染，
        //      而另一个图像缓冲区可以用于呈现。这样就可以实现双缓冲，提高渲染效率和稳定性。
        // 如果超过了最大图像数量，则取最大值，因为交换链的图像最小数不能超过表面支持的图象数；
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        /*VkSwapchainCreateInfoKHR{
        * 1
            VkStructureType                  sType;
            const void* pNext;
            VkSwapchainCreateFlagsKHR        flags;
            VkSurfaceKHR                     surface;
        * 2
            uint32_t                         minImageCount;
            VkFormat                         imageFormat;
            VkColorSpaceKHR                  imageColorSpace;
            VkExtent2D                       imageExtent;
            uint32_t                         imageArrayLayers;
            VkImageUsageFlags                imageUsage;
        * 3
            VkSharingMode                    imageSharingMode;
            uint32_t                         queueFamilyIndexCount;
            const uint32_t* pQueueFamilyIndices;
        * 4
            VkSurfaceTransformFlagBitsKHR    preTransform;
            VkCompositeAlphaFlagBitsKHR      compositeAlpha;
            VkPresentModeKHR                 presentMode;
            VkBool32                         clipped;
            VkSwapchainKHR                   oldSwapchain;
        }数字是下面初始化过程*/
        // 创建一个VkSwapchainCreateInfoKHR结构体，用于存储创建交换链所需的参数，定义之后不能改变！
        VkSwapchainCreateInfoKHR createInfo{};

        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR; // 设置结构体类型必须为VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR
        createInfo.surface = surface;                                   // 设置要绑定到交换链的表面对象

        createInfo.minImageCount = imageCount;                          // 设置交换链图像缓冲区的最低图象数
        createInfo.imageFormat = surfaceFormat.format;                  // 设置交换链图像缓冲区的颜色格式为自己选的一个表面颜色格式
        createInfo.imageColorSpace = surfaceFormat.colorSpace;          // 设置交换链图像缓冲区的颜色格式为自己选的一个表面颜色空间
        createInfo.imageExtent = extent;                                // 设置交换链图像缓冲区的分辨率与表面一致
        createInfo.imageArrayLayers = 1;                                // 设置交换链图像缓冲区的层数
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;    // 设置交换链图像缓冲区的用途会颜色附件

        
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice); // 查询物理设备支持的队列族索引，包括图形队列族和呈现队列族
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
        // imageSharingMode是一个枚举类型，它用于指定一个Vulkan资源（如图像缓冲区）如何被不同的队列族（queue family）访问和共享。
        // imageSharingMode有两个可能的值：
        //      VK_SHARING_MODE_EXCLUSIVE：表示该资源在同一时间只能被一个队列族独占访问。
        //          如果要在不同的队列族之间切换访问，必须执行一个队列族所有权转移（queue family ownership transfer）操作。
        //      VK_SHARING_MODE_CONCURRENT：表示该资源可以被多个队列族并发访问，无需执行所有权转移操作。但是，这种模式可能会降低访问性能。
        if (indices.graphicsFamily != indices.presentFamily) {
            // 如果图形队列族和呈现队列族不同，则设置交换链图像缓冲区的共享模式为并发模式，并指定两个队列族索引
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;   
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {// 否则，设置交换链图像缓冲区的共享模式为独占模式
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;   // 设置交换链图像缓冲区的变换方式为当前表面支持的变换方式，通常为无变换
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;              // 设置交换链图像缓冲区的混合方式为不透明混合，即忽略alpha通道
        createInfo.presentMode = presentMode;                                       // 设置交换链图像缓冲区的呈现模式为之前选择的呈现模式
        createInfo.clipped = VK_TRUE;                                               // 设置交换链图像缓冲区的裁剪方式为裁剪，即忽略被窗口遮挡的像素
        createInfo.oldSwapchain = VK_NULL_HANDLE;                                   // 设置交换链的旧交换链对象为VK_NULL_HANDLE，表示不需要重建交换链

        // 调用vkCreateSwapchainKHR函数，使用创建信息结构体和逻辑设备对象，创建交换链对象，并存储到swapChain变量中
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        // 再次调用vkGetSwapchainImagesKHR函数，使用逻辑设备对象和交换链对象，获取交换链中图像缓冲区的句柄，并存储到swapChainImages向量中
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);   // 查询交换链中图像缓冲区的数量，这里等于3
        swapChainImages.resize(imageCount);                                 // 根据图像缓冲区的数量，调整swapChainImages向量的大小
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        // 将交换链图像缓冲区的格式和分辨率存储到swapChainImageFormat和swapChainExtent变量中，以便后续使用
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        // 检查设备是否有自己所需要的颜色格式；并返回自己需要的表面格式结构体VkSurfaceFormatKHR；
        // 使用最常用的颜色格式srbg(int)，srbg非线性颜色空间
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        // 检查设备是否有自己所支持的呈现格式；并返回自己需要的呈现模式枚举值VkPresentModeKHR；
        // 默认使用的邮箱模式的呈现模式，方便使用多个缓冲区的交换链；但是我的设备并不支持；
        // 所以我电脑只支持双重缓冲，不支持三重缓冲；
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                std::cout << "you" << std::endl;
                return availablePresentMode;
            }
        }

        // 因为我的设备不支持邮箱模式，所以使用FIFO模式；
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        // 用于选择交换链的分辨率
        // currentExtent为一个特殊值(0xFFFFFFFF, 0xFFFFFFFF)
        // 也就是std::numeric_limits<uint32_t>::max()),就意味着表面支持任意分辨率
        // std::numeric_limits是一个模板类，它提供了一些静态成员函数和常量，用于查询不同数值类型的特性和限制
        // 例如，std::numeric_limits<int>::min()返回一个int类型的最小值，std::numeric_limits<double>::epsilon()返回一个double类型的机器精度。
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            // 如果表面支持任意分辨率，那么使用窗口的当前分辨率
            return capabilities.currentExtent;
        }
        else {
            // 否则，获取窗口的帧缓冲大小
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            // 创建一个VkExtent2D结构体，用于存储窗口的分辨率
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            // 将窗口的分辨率限制在表面支持的最小和最大分辨率之间
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            // 返回选择的分辨率
            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        // 获得物理设备交换链或者表面的相关信息，包括表面特性，允许的呈现颜色格式与呈现模式，即下面结构体；
        /*struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        }*/
        SwapChainSupportDetails details;
        // 获得物理设备表面性能
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
        // 我电脑的性质是： 
        //  minImageCount           2
        //  maxImageCount           64
        //  currentExtent           目前窗口大小
        //  minImageExtent          glfw设置的大小
        //  maxImageExtent          glfw设置的大小
        //  maxImageArrayLayers     2048
        //  supportedTransforms     1
        //      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR：0001
        //      VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR：0010
        //      VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR：0100
        //      VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR：1000
        //      VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR：00010000
        //      VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR：00100000
        //      VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR：01000000
        //      VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR：10000000
        //      VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR：100000000
        //      意味着表面只支持一种变换方式：VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,表示不进行任何变换，保持原始方向。
        //  currentTransform        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        //  supportedCompositeAlpha 9
        //      Vulkan支持以下四种混合方式2：
        //      (1).VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR：表示忽略alpha分量，将图像视为不透明的。它的值为0x00000001，二进制为0001。
        //      (2).VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR：表示尊重alpha分量，假设图像已经被alpha分量预乘过。它的值为0x00000002，二进制为0010。
        //      (3).VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR：表示尊重alpha分量，假设图像没有被alpha分量预乘过。它的值为0x00000004，二进制为0100。
        //      (4).VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR：表示不确定alpha分量的处理方式，由应用程序通过本地窗口系统命令来设置混合方式。它的值为0x00000008，二进制为1000。
        //      supportedCompositeAlpha等于9，那么意味着你的表面支持两种混合方式(1)和(4)。因为9的二进制表示为1001
        //  supportedUsageFlags     31
        //      (1).VK_IMAGE_USAGE_TRANSFER_SRC_BIT：00000001    
        //      (2).VK_IMAGE_USAGE_TRANSFER_DST_BIT：00000010
        //      (3).VK_IMAGE_USAGE_SAMPLED_BIT：00000100
        //      (4).VK_IMAGE_USAGE_STORAGE_BIT：00001000
        //      (5).VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT：00010000
        //      (6).VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT：00100000
        //      (7).VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT：01000000
        //      (8).VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT：10000000
        //      31的二进制表示为11111，意味着交换链图像缓冲区支持五种用途:(1) (2) (3) (4) (5);

        // 获得设备支持的表面颜色格式数量 4个
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        // 获得设备支持的表面颜色格式信息 
        // {format=VK_FORMAT_B8G8R8A8_UNORM (44) colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (0) }
        // {format=VK_FORMAT_B8G8R8A8_SRGB (50) colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (0) }
        // {format=VK_FORMAT_R8G8B8A8_UNORM (37) colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (0) }
        // {format=VK_FORMAT_R8G8B8A8_SRGB (43) colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (0) }
        if (formatCount != 0) {
            details.formats.resize(formatCount);    // 预先分配format vector的大小，减少消耗
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // 获得设备支持的呈现模式数量 2个
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        // 获得设备支持的呈现模式信息：
        // VK_PRESENT_MODE_IMMEDIATE_KHR 支持邮箱模式
        // VK_PRESENT_MODE_FIFO_KHR      支持FIFO模式
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }
        // 为什么我电脑只支持双缓冲，但是我表面最大支持的图象数是64
        //      一个交换链可以有超过2，3的图像缓冲区，这取决于在创建交换链时指定的图像缓冲区的数量。
        //      表面呈现模式并不直接影响交换链有多少个缓冲区，但是它会影响交换链中的图像缓冲区如何被呈现到表面上。
        //      双缓冲或三缓冲是指交换链中有两个或三个呈现目标图像(缓冲)，它们轮流被显示到屏幕上。
        // 返回本设备的SwapChainSupportDetails结构体，交换链的详细信息；
        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {// 创建物理设备的时候还检查一下设备是不是支持交换链
        QueueFamilyIndices indices = findQueueFamilies(device);

        // 检查设备是否有我需要的扩展：deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME} 
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            // 获得物理设备交换链或者表面的相关信息，包括表面特性，允许的呈现颜色格式与呈现模式；
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            // 如果设备至少支持一个表面颜色格式与呈现模式，那么交换链可用；
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        // 如果设备含有自己需要的队列族，还支持交换链创建扩展，同时设备支持交换链交换图像到屏幕上(如果设备至少支持一个表面颜色格式与呈现模式)，则返回真；
        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) { // 检查设备是否有我们所需要的扩展deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME} 
        uint32_t extensionCount; 
        // 通过物理设备获得我电脑所支持的全部扩展；
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        // VkExtensionProperties是一个vulkan扩展信息结构体，包含扩展的名称和版本号。
        // 获得我的电脑所支持的57个扩展，储存在availableExtensions扩展性质vector里面；
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // 导入自己需要的vulkan扩展；
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        // 遍历全部的vulkan版本号，如果有我们需要的扩展，那么对应的在requiredExtensions(需要扩展)中删去对应扩展
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        // 遍历结束，如果我们需要的扩展都有，那么requiredExtensions为空，返回true，如果不为空，返回false；
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