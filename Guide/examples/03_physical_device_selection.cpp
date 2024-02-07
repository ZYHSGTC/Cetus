#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>

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

//这个程序检查是否有物理设备，物理设备的队列族有没有支持相应操作的队列(图形计算)，并且初始化物理设备；

struct QueueFamilyIndices {
    // 定义一个结构体，用来存储一个物理设备（显卡）支持的队列族（queue family）的索引
    // 这里只关注支持图形操作的队列族的索引，所以只有一个可选的uint32_t类型的成员变量
    
    // 队列族是一组具有相同特性的队列（queue），队列是用来提交图形和计算命令的接口
    
    // 图形管线绑定到支持图形操作的队列族上才能执行渲染命令
    // 命名为队列族，是因为它们都是通过队列来实现的，是一个指令缓冲区的执行器；队列传输或接收的数据是指令缓冲区（command buffer）
    // 指令缓冲区是一组预先记录好的图形或计算指令，比如绘制三角形，复制图像等。队列实际上是在GPU上运行的，它们负责执行提交给它们的指令缓冲区。

    // 一个队列正在执行图形操作，那么它就不能同时执行计算操作，必须等待图形操作完成后才能执行计算操作。
    // 如果一个队列支持多种类型的操作，那么它需要在切换不同类型的操作时进行同步
    // 通过使用队列，Vulkan可以让开发者更好地控制和优化GPU的资源利用和调度，从而提高应用程序的效率和质量。
    // 比如，开发者可以根据不同的需求选择合适的队列来执行不同类型或不同优先级的操作，或者使用多个队列来实现并行或异步的操作。

    // 队列设计的目的
    // 降低CPU开销：传统的图形API（如OpenGL）使用同步渲染模型，这意味着驱动程序必须跟踪每个渲染操作的状态和资源，保证任务按照正确的顺序执行，
    //              并在需要时阻塞CPU等待GPU完成工作。这些操作会消耗CPU的时间和资源，影响程序的效率和响应性。
    //              Vulkan使用异步渲染模型，这意味着开发者可以直接将命令打包到队列中，使用显式的依赖关系来控制任务的执行顺序，
    //              并使用显式的同步原语来协调CPU和GPU之间的工作。这些操作可以减少驱动程序的负担，降低CPU的开销，提高程序的性能和流畅度。
    // 增强多线程能力：传统的图形API（如OpenGL）使用基于上下文（context）的渲染模式，这意味着每个线程只能访问一个上下文，而每个上下文只能绑定到一个GPU。
    //              这种模式限制了多线程渲染的可能性，因为不同线程之间需要频繁地切换上下文和同步状态。
    //              Vulkan使用基于队列（queue）的渲染模式，这意味着每个线程可以访问多个队列，而每个队列可以绑定到多个GPU。
    //              这种模式增强了多线程渲染的能力，因为不同线程之间可以并行地提交命令到不同队列和GPU。
    // 支持多种任务类型：传统的图形API（如OpenGL）主要支持图形渲染相关的任务，而对于其他类型的任务（如计算处理或内存传输）则支持较弱或缺乏标准化。
    //              Vulkan支持多种任务类型，包括图形渲染、计算处理、内存传输、稀疏资源绑定等。Vulkan还允许开发者自定义扩展来支持更多类型的任务。
    //              Vulkan通过不同类型的队列族（queue family）来区分不同类型的任务，每个队列族只支持一种或多种特定类型的任务
    std::optional<uint32_t> graphicsFamily;
    // optional的特性有以下几点：
    //  optional可以用来表示一个可能不存在的值，例如一个函数的返回值，一个类的成员变量，或者一个方法的参数。
    //  optional可以通过一些方法来判断是否包含值，例如isPresent()，has_value()，或者重载了bool运算符。
    //  optional可以通过一些方法来获取或者设置包含的值，例如get()，value()，of()，ofNullable()，orElse()，orElseGet()，orElseThrow()等。
    //  optional可以通过一些方法来对包含的值进行操作，例如map()，flatMap()，filter()等。
    //  optional可以避免显式地进行空值检查和异常处理，提高代码的简洁性和健壮性。
    // optional的应用场景有以下几种：
    //  optional可以用来作为一个可能失败的函数的返回值，这样可以避免返回null或者抛出异常，并且强制调用者处理没有结果的情况。
    //  optional可以用来作为一个可能为空的类的成员变量的类型，这样可以避免使用空对象模式或者特殊值，并且提高了类的封装性和不变性。
    //  optional可以用来作为一个可能为空的方法的参数的类型，这样可以避免传递null或者重载方法，并且提高了方法的可读性和灵活性。
    
    // 定义一个方法，用来判断是否找到了支持图形操作的队列族
    bool isComplete() {
        return graphicsFamily.has_value();
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

    // 用来存储选择的物理设备（显卡）
    // VkPhysicalDevice是指向物理设备(显卡)的句柄(指针)，是Vulkan库内部维护的一个物理设备对象或者结构体，我们无法直接访问或修改它。
    // 我们可以通过它来获取物理设备的属性和特性，比如名称，类型，版本，纹理压缩，几何着色器等。
    // 我们也可以通过它来创建逻辑设备（logical device），用于与物理设备进行交互。
    // 一个电脑有一个显卡，但是可能有多个物理设备。物理设备是指支持Vulkan API的图形硬件，比如显卡或者集成显卡。
    // 有些显卡可能包含多个物理设备，比如双芯显卡或者SLI/CrossFire模式下的多块显卡。
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        pickPhysicalDevice();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

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

    // 定义一个函数，用来从所有可用的物理设备中选择一个合适的物理设备
    void pickPhysicalDevice() {
        
        uint32_t deviceCount = 0;                                                   // 定义一个变量，用来存储物理设备的数量
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);                // 调用Vulkan API函数，获取物理设备的数量，传入instance参数表示Vulkan实例对象，传入nullptr表示不获取具体的物理设备列表

        
        std::cout << "deviceCount:" << deviceCount << std::endl;                    // 我的电脑只有一个物理设备，集成显卡;
        if (deviceCount == 0) {                                                     // 如果没有找到任何支持Vulkan的物理设备，就抛出一个运行时错误
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);                         // 定义一个vector容器，用来存储所有可用的物理设备
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());         // 再次调用Vulkan API函数，获取所有可用的物理设备，传入instance参数表示Vulkan实例对象，传入devices.data()表示获取具体的物理设备列表

        
        for (const auto& device : devices) {                                        // 遍历所有可用的物理设备
            if (isDeviceSuitable(device)) {                                         // 调用自定义的函数，判断当前物理设备是否有我需要的队列族
                physicalDevice = device;                                            // 如果满足，就将其赋值给类成员物理设备physicalDevice，并跳出循环，只指选择一个物理设备
                break;
            }
        }
        
        if (physicalDevice == VK_NULL_HANDLE) {                                     // 如果没有找到任何合适的物理设备，就抛出一个运行时错误
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    // 定义一个函数，用来判断一个物理设备是否满足我们的需求
    bool isDeviceSuitable(VkPhysicalDevice device) {
        // 调用自定义的函数，获取该物理设备支持的队列族的索引
        QueueFamilyIndices indices = findQueueFamilies(device);

        // 返回该物理设备是否支持图形操作
        return indices.isComplete();
    }

    // 定义一个函数，用来获取一个物理设备支持的队列族的索引
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        // 定义一个QueueFamilyIndices结构体对象，用来存储队列族索引
        QueueFamilyIndices indices;

        // 定义一个变量，用来存储队列族的数量
        uint32_t queueFamilyCount = 0;
        // 调用Vulkan API函数，获取队列族的数量，传入device参数表示物理设备对象，传入nullptr表示不获取具体的队列族属性列表
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        // 我的显卡只支持一个队列族
        std::cout << "queueFamilyCount:" << queueFamilyCount << std::endl;
        // 定义一个vector容器，用来存储所有队列族的属性
        // VkQueueFamilyProperties这个数据类型是一个结构体（struct），它用于存储一个队列族的属性信息。它包含以下几个成员变量：
        //  queueFlags：表示该队列族支持哪些类型的操作，比如图形、计算、内存传输等。
        //  queueCount：表示该队列族包含多少个队列。
        //  timestampValidBits：表示该队列族支持的时间戳精度位数。
        //  minImageTransferGranularity：表示该队列族支持的最小图像传输粒度。
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        // 从物理设备中获取队列族，queueFamilies.data()表示获取具体的队列族属性列表
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        // 定义一个变量，用来作为队列族索引的计数器
        int i = 0;
        // 遍历所有队列族的属性
        for (const auto& queueFamily : queueFamilies) {
            // 我的唯一的队列族里只有一个队列
            std::cout << "queueCount:" << queueFamily.queueCount << std::endl;
            // 判断当前队列族是否支持图形操作，如果支持，就将其索引赋值给indices.graphicsFamily
            // queueFlags是一个位域（bit field），表示该队列族支持哪些类型的操作，比如图形、计算、内存传输等。queueFlags有以下几种值，它们可以单独使用或者组合使用：
            //  VK_QUEUE_GRAPHICS_BIT：表示该队列族支持图形操作，比如渲染管线，绘制指令等。
            //  VK_QUEUE_COMPUTE_BIT：表示该队列族支持计算操作，比如计算着色器，调度指令等。
            //  VK_QUEUE_TRANSFER_BIT：表示该队列族支持内存传输操作，比如复制缓冲区，复制图像等。
            //  VK_QUEUE_SPARSE_BINDING_BIT：表示该队列族支持稀疏资源绑定操作，比如绑定稀疏缓冲区，稀疏图像等。
            //  VK_QUEUE_PROTECTED_BIT：表示该队列族支持保护内存操作，比如创建和使用保护内存对象等。
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                // VK_QUEUE_GRAPHICS_BIT是一个常量（constant），它的值是1。它表示一个二进制数（binary number）00000001，其中只有最低位（least significant bit）是1，其他位都是0。
                // queueFlags等于15表示一个二进制数00001111，其中最低四位都是1，其他位都是0。
                // 它表示我电脑唯一的队列族同时支持图形、计算、内存传输和稀疏资源绑定操作。
                indices.graphicsFamily = i;
            }

            // 判断是否已经找到了支持图形操作的队列族，如果是，就跳出循环
            if (indices.isComplete()) {
                break;
            }

            // 索引计数器加一
            i++;
        }

        // 返回队列族索引结构体对象
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