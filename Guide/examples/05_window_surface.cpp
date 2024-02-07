#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
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
        // 需要同时有需要的图形队列族与呈现队列族，如果都有，那么这个结构体已满
    }
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
    VkSurfaceKHR surface; // KHR是Khronos Group的缩写
    // 表面是一种用于管理和呈现图像的Vulkan对象，它可以与不同的窗口系统进行交互。
    // Vulkan表面就是一个窗口画面，它是有大小宽高、颜色格式等特征的,Vulkan允许创建和使用多个表面
    // VkSurfaceKHR是一个不透明的句柄，它代表了一个窗口表面对象，用于将Vulkan渲染的图像显示在窗口上。
    // VkSurfaceKHR的创建需要使用不同的平台特定扩展，比如VK_KHR_win32_surface或者VK_KHR_xcb_surface，根据不同的窗口系统提供相应的参数，比如HWND或者xcb_window_t234。
    // VkSurfaceKHR对象在应用程序退出时需要被销毁。
    // VkSurfaceKHR对象可以用来查询物理设备是否支持在该表面上呈现图像，以及支持的交换链格式和模式等信息 。
    // VkSurfaceKHR对象可以用来创建交换链对象，用于管理和呈现图像。

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    // 呈现队列族和图形队列族是两种不同的队列族类型，它们分别表示支持在表面上呈现图像和执行图形操作的队列族。
    // 可能存在一种情况，就是同一个队列族既支持呈现操作，又支持图形操作。
    // 这样的话，我们就可以使用同一个队列族来创建两个不同类型的队列，只需要给它们分配不同的优先级。

    
    // 呈现队列是一种Vulkan队列，它可以执行将交换链中的图像显示在表面上的操作。
    //      呈现队列负责将交换链中的图像传递给表面，并进行一些必要的同步和变换。
    //      交换链与表面沟通的途径不仅仅是呈现队列，还包括呈现引擎，它是负责将交换链中的图像转换为表面可以接受的格式，
    //      并进行一些合成和调整的软件或硬件组件。
    // 呈现队列还会对图像做一些其他的图像操作，主要有以下几种：
    //     同步操作：呈现队列需要与渲染队列和表面进行同步，以保证图像在正确的时间被获取、渲染和显示。
    //          同步操作可以使用信号量、栅栏或事件等机制实现，以避免竞争条件或数据不一致等问题。
    //     变换操作：呈现队列需要根据交换链中指定的变换方式，对图像进行旋转、镜像等变换操作，以适应表面的方向和布局。
    //          变换操作可以使用交换链创建时指定的currentTransform参数或vkSetSwapchainTransformKHR函数来控制。
    //     合成操作：呈现队列需要根据交换链中指定的合成模式，对图像进行alpha混合或其他合成操作，以适应表面的透明度和叠加效果。
    //          合成操作可以使用交换链创建时指定的compositeAlpha参数或vkSetSwapchainCompositeAlphaKHR函数来控制。
    
    // 呈现队列的定义是为了解决以下问题：
    // Vulkan是一个平台无关的API，它不能直接和窗口系统交互。为了将Vulkan渲染的图像显示在窗口上，
    //      我们需要使用WSI（Window System Integration）扩展，提供一个抽象的接口。
    // 不同的平台可能有不同的窗口系统和呈现机制，我们需要一种统一的方式来处理不同的情况。
    //      呈现队列提供了一种抽象层，使得应用程序可以无需关心平台细节，只需要关心表面对象和交换链对象。
    // 不同的物理设备可能有不同的呈现能力和限制，我们需要一种灵活的方式来选择合适的物理设备和队列族。
    //      呈现队列提供了一种查询机制，使得应用程序可以根据表面对象来判断物理设备是否满足需求，并且选择合适的交换链模式和格式。
    
    // 表面具体可以做以下事情：
    // 表面可以用来创建交换链对象，用于管理和呈现图像。
    //      交换链对象是一个包含多个图像缓冲区的数组，每个图像缓冲区都与表面对象关联，并且可以被呈现引擎使用。
    // 表面可以用来提交图像缓冲区给呈现引擎，用于在表面上显示图像。
    //      提交图像缓冲区需要使用vkQueuePresentKHR函数，并提供一个VkPresentInfoKHR结构体，其中包含要提交的交换链对象和图像索引。
    // 表面可以用来获取下一个可用的图像缓冲区，用于进行渲染操作。
    //      获取下一个可用的图像缓冲区需要使用vkAcquireNextImageKHR函数，并提供一个交换链对象和一个超时值。
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
        // 窗口表面的创建需要在创建物理设备和逻辑设备之前，因为窗口表面会影响物理设备的选择和逻辑设备的配置。
        // 我们需要根据窗口表面来判断物理设备是否满足我们的需求，比如是否支持presentQueue队列族，是否支持交换链格式和模式等。
        // 我们还需要根据窗口表面来指定逻辑设备所需的扩展和特性，比如VK_KHR_swapchain扩展。
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
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
            // glfwCreateWindowSurface是GLFW库提供的一个函数，它用来创建一个与GLFW窗口关联的Vulkan表面对象。
            // Vulkan表面对象是用来表示可以呈现图像的抽象类型
            // 运行的时候，内部发生了以下事情8：
            // 根据不同的平台，调用相应的平台特定扩展函数，如vkCreateWin32SurfaceKHR或者vkCreateXcbSurfaceKHR，来创建一个与原生窗口系统交互的Vulkan表面对象。
            // 将创建的Vulkan表面对象与GLFW窗口对象进行关联，方便之后进行销毁和查询操作。
            // 返回创建表面对象的结果，如果成功，则返回VK_SUCCESS，否则返回相应的错误码。
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
        // 找到支持呈现和图形操作的队列族索引，这里假设呈现队列与图形队列是不同的，那么就要为逻辑设备设定了两个队列
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };
        //std::set是C++标准库中的一个容器，它可以存储一组不重复的元素，并且按照一定的顺序进行排序。
        //std::set的用途有以下几点1：
        // 可以用来检查一个元素是否存在于集合中，因为std::set内部使用平衡二叉树实现，所以查找的时间复杂度是O(log n)。
        // 可以用来去除重复的元素，因为std::set不允许有相同的元素。
        // 可以用来对一组元素进行排序，因为std::set会自动按照元素的值或者自定义的比较函数进行排序。

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {// 遍历每个不重复的队列族索引
            VkDeviceQueueCreateInfo queueCreateInfo{}; // 创建一个结构体，用来存储队列创建信息
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; // 设置结构体类型
            queueCreateInfo.queueFamilyIndex = queueFamily;     // 设置要使用的队列族索引
            queueCreateInfo.queueCount = 1;                     // 设置要创建的队列数量
            queueCreateInfo.pQueuePriorities = &queuePriority;  // 设置队列优先级指针
            queueCreateInfos.push_back(queueCreateInfo);        // 将结构体添加到向量中
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        // vector.data()返回的是一个内存数组第一个元素的指针；
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = 0;

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
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue); // 调用函数，获取呈现队列对象
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        return indices.isComplete();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        // 定义一个变量，用来存储物理设备支持的队列族数量
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        // 调用函数，获取每个队列族的属性VkQueueFamilyProperties
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {             // 遍历每个队列族
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {   // 如果队列族支持图形操作
                indices.graphicsFamily = i;                         // 将当前索引赋值给图形队列族索引
            }

            // 调用函数，查询当前队列族是否支持给定的表面呈现
            VkBool32 presentSupport = false;// 定义一个变量，用来存储当前队列族是否支持表面呈现
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {                                   // 如果支持表面呈现
                indices.presentFamily = i;                          // 将当前索引赋值给呈现队列族索引
            }                                                       // 大部分gpu中一般呈现队列与图形队列通常是相同的

            if (indices.isComplete()) {     // 如果图形队列族和呈现队列族都已经找到
                break;                      // 跳出循环
            }

            i++;                            // 索引加一，继续下一个队列族
        }

        return indices;                     // 返回找到的队列族索引结构体
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