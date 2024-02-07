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

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return graphicsFamily.has_value();
    }
};


// 这个程序相比前一个创建了一个逻辑设备，并获取的这个逻辑设备的图形队列族
// Vulkan中有三种设备的概念：实例（instance），物理设备（physical device）和逻辑设备（logical device）。
// 实例是Vulkan应用程序的入口点，它表示了一个与Vulkan运行时的连接。
//      实例处理的是查询物理设备，执行一些设备创建前的代码，比如指定验证层和实例扩展等。
//      实例扩展是一些额外的功能，比如窗口系统集成，调试报告等。
//      实例扩展需要某种程度的加载程序支持（加载程序是Vulkan运行时的一部分，独立于任何GPU），
//      它扩展的是设备创建前的代码部分，而不控制设备本身的行为。
// 物理设备对应了电脑上的一个GPU，它表示了GPU的硬件特性和支持的功能。
//      物理设备可以通过实例查询出来，每个物理设备都有一个唯一的句柄（handle）来标识它。
//      物理设备可以提供一些信息，比如支持的队列族（queue family），设备扩展（device extension），物理设备特性（physical device feature）等。
// 逻辑设备是基于一个物理设备创建的，它表示了与物理设备进行交互的接口。
//      逻辑设备处理的是Vulkan在这一个具体的设备上进行渲染或者通用计算的行为。

// 它们创建的顺序是：
//  先创建实例（vkCreateInstance），用于与Vulkan运行时建立连接。
//  再查询物理设备（vkEnumeratePhysicalDevices），用于获取可用的GPU信息。
//  最后创建逻辑设备（vkCreateDevice），用于与物理设备进行交互。
// 它们的销毁顺序是：
//  先销毁逻辑设备（vkDestroyDevice），用于释放与物理设备相关的资源。
//  再销毁实例（vkDestroyInstance），用于断开与Vulkan运行时的连接。
//  最后不需要销毁物理设备，因为它们是由Vulkan运行时管理的。
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

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    // Vulkan中创建逻辑设备需要指定以下几个信息：
    //  物理设备句柄（VkPhysicalDevice），用于选择要使用的GPU。
    //  队列创建信息（VkDeviceQueueCreateInfo），用于指定要使用哪些队列族和队列。
    //      在创建逻辑设备时，需要指定要使用哪些队列族，以及每个队列族中要使用多少个队列。
    //      每个队列族和队列都需要一个VkDeviceQueueCreateInfo结构体来描述它们的索引、数量和优先级。
    //  设备扩展信息（VkDeviceExtensionProperties），用于指定要启用哪些设备扩展。
    //      设备扩展是一些额外的功能，比如交换链（swapchain），它允许将渲染图形呈现到窗口系统。
    //      不同的物理设备可能支持不同的设备扩展。在创建逻辑设备时，需要指定要启用哪些设备扩展，以及它们所需的任何参数。
    //  物理设备特性信息（VkPhysicalDeviceFeatures），用于指定要使用哪些物理设备特性。
    //      物理设备特性是一些硬件功能，比如几何着色器（geometry shader），多重采样（multisampling）等。
    //      不同的物理设备可能支持不同的物理设备特性。在创建逻辑设备时，需要指定要使用哪些物理设备特性，以及它们所需的任何参数。
    //  验证层信息（VkLayerProperties），用于指定要启用哪些验证层。
    //      验证层可以分为实例验证层和设备验证层。实例验证层只检查和全局Vulkan对象相关的调用，比如Vulkan实例。
    //      设备验证层只检查和特定GPU相关的调用。
    
    // Vulkan中使用逻辑设备可以做以下几件事：@@@@@
    // 创建和销毁其他Vulkan对象，比如缓冲区（buffer），图像（image），管线（pipeline）等。
    //      这些对象都需要一个逻辑设备句柄来指定它们属于哪个逻辑设备，以及它们的一些属性和参数。
    // 分配和释放内存，用于存储Vulkan对象或其他数据。Vulkan中的内存管理是显式的，需要开发者自己申请和释放内存。
    //      逻辑设备提供了一些函数来分配和释放内存，以及查询物理设备的内存属性和需求。
    // 记录和提交指令缓冲区（command buffer），用于存储图形或计算指令。指令缓冲区是一组预先记录好的图形或计算指令，比如绘制三角形，复制图像等。
    //      逻辑设备提供了一些函数来创建和销毁指令缓冲区，以及将它们提交给命令队列执行。
    // 同步和查询操作状态，用于保证操作之间的正确顺序和结果。Vulkan中的操作是异步的，需要开发者自己同步和查询它们的状态。
    //      逻辑设备提供了一些函数来创建和销毁同步对象，比如信号量（semaphore），栅栏（fence），事件（event）等，以及查询操作的状态和结果。
    
    VkQueue graphicsQueue;
    // VkQueue是一个句柄，它表示一个命令队列对象，是一个硬件资源。
    //      它是Vulkan中用来提交图形和计算命令的接口，它们在GPU上运行，负责执行提交给它们的指令缓冲区。
    // 提交指令缓冲区（vkQueueSubmit），用于将一组图形或计算指令发送给GPU执行。
    //      每个指令缓冲区都需要一个命令队列句柄来指定它要提交到哪个命令队列，以及它的一些属性和参数。
    // 呈现交换链图像（vkQueuePresentKHR），用于将渲染好的图像显示到窗口系统上。
    //      每个交换链图像都需要一个命令队列句柄来指定它要呈现到哪个命令队列，以及它的一些属性和参数。
    // 等待队列空闲（vkQueueWaitIdle），用于等待队列完成所有已提交的操作。
    //      这个函数可以用来同步队列上的操作，或者在销毁逻辑设备之前等待队列完成所有操作。

    // VkQueue和VkDevice都是不透明句柄(opaque handle)，它们是一些指向Vulkan对象的指针，但是它们的具体类型和内容对于应用程序是不可见的。
    //      应用程序只能通过Vulkan提供的函数来操作它们，不能直接访问或修改它们的内部数据。
    // 不透明句柄的设计有以下几个好处：
    //     它可以隐藏Vulkan对象的实现细节，使得API更加简洁和一致。
    //     它可以保证Vulkan对象的状态和数据的完整性，避免应用程序对它们进行非法或错误的操作。
    //     它可以提高Vulkan对象的移植性和兼容性，使得不同的平台和驱动可以使用不同的方式来实现和管理它们。
    // Vulkan中除了VkQueue和VkDevice之外，还有很多其他的不透明句柄，他们是由VK_DEFINE_HANDLE定义的；
    //      比如VkInstance, VkPhysicalDevice, VkBuffer, VkImage, VkPipeline等。
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

    
    void createLogicalDevice() {    // 用于创建逻辑设备，获取逻辑设备中的队列
        // 获得支持图形队列的队列族索引
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        VkDeviceQueueCreateInfo queueCreateInfo{};          // 定义一个结构体，用于描述设备队列族的创建信息
        // VkDeviceQueueCreateInfo是一个结构体，它包含了创建命令队列所需的信息，比如：
        // 队列族索引（queueFamilyIndex），用于指定要使用哪个队列族。
        //      每个物理设备都有一个或多个队列族，每个队列族都有一个唯一的索引来标识它。
        // 队列数量（queueCount），用于指定要使用该队列族中的多少个队列。
        //      每个队列族都有一个最大可用队列数目，不能超过这个数目。
        // 队列优先级（pQueuePriorities），用于指定每个队列的优先级，范围是0.0到1.0。
        //      优先级决定了当多个队列同时提交命令时，哪个队列会被优先执行。
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; // 必须为这个值
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();  // 指定队列族索引
        queueCreateInfo.queueCount = 1;                     // 设置队列的数量，这里只使用了一个队列
        float queuePriority = 1.0f;                         // 定义一个浮点数，用于表示队列族的优先级 
        queueCreateInfo.pQueuePriorities = &queuePriority;  // 设置结构体中指向队列优先级数组的指针，不同队列不同优先级

        VkPhysicalDeviceFeatures deviceFeatures{};          // 定义一个结构体，用于描述物理设备的特性
        //VkPhysicalDeviceFeatures是一个结构体，它包含了一系列布尔值。
        // 这些布尔值表示物理设备支持的一些硬件功能，比如几何着色器（geometry shader），多重采样（multisampling）等。
        // 示例代码：
        // VkPhysicalDeviceFeatures deviceFeatures{};
        // deviceFeatures.geometryShader = VK_TRUE; // 启用几何着色器
        // deviceFeatures.sampleRateShading = VK_TRUE; // 启用多重采样


        VkDeviceCreateInfo createInfo{};                    // 定义一个结构体，用于描述逻辑设备的创建信息
        
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;            // 设置结构体的类型
        createInfo.queueCreateInfoCount = 1;                // 设置设备队列创建信息数组的大小，这里只有一个队列族
        createInfo.pQueueCreateInfos = &queueCreateInfo;    // 设置结构体中指向设备队列创建信息数组的指针
        createInfo.pEnabledFeatures = &deviceFeatures;      // 设置结构体中指向物理设备特性结构体的指针
        
        createInfo.enabledExtensionCount = 0;               // 设置启用的扩展数量，这里没有启用任何扩展
        if (enableValidationLayers) {                       // 如果启用了验证层，就设置启用的验证层数量和名称   
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {                                              // 如果没有启用验证层，就设置启用的验证层数量为0
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            // 调用Vulkan API函数，用于创建逻辑设备，并将结果存储在device变量中
            // 如果创建失败，就抛出一个运行时错误异常
            throw std::runtime_error("failed to create logical device!");
        }

        // 调用Vulkan API函数，用于获取逻辑设备中的图形队列，并将结果存储在graphicsQueue变量中
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    }


    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        return indices.isComplete();
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