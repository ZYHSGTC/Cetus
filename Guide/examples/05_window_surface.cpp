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
        // ��Ҫͬʱ����Ҫ��ͼ�ζ���������ֶ����壬������У���ô����ṹ������
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
    VkSurfaceKHR surface; // KHR��Khronos Group����д
    // ������һ�����ڹ���ͳ���ͼ���Vulkan�����������벻ͬ�Ĵ���ϵͳ���н�����
    // Vulkan�������һ�����ڻ��棬�����д�С��ߡ���ɫ��ʽ��������,Vulkan��������ʹ�ö������
    // VkSurfaceKHR��һ����͸���ľ������������һ�����ڱ���������ڽ�Vulkan��Ⱦ��ͼ����ʾ�ڴ����ϡ�
    // VkSurfaceKHR�Ĵ�����Ҫʹ�ò�ͬ��ƽ̨�ض���չ������VK_KHR_win32_surface����VK_KHR_xcb_surface�����ݲ�ͬ�Ĵ���ϵͳ�ṩ��Ӧ�Ĳ���������HWND����xcb_window_t234��
    // VkSurfaceKHR������Ӧ�ó����˳�ʱ��Ҫ�����١�
    // VkSurfaceKHR�������������ѯ�����豸�Ƿ�֧���ڸñ����ϳ���ͼ���Լ�֧�ֵĽ�������ʽ��ģʽ����Ϣ ��
    // VkSurfaceKHR����������������������������ڹ���ͳ���ͼ��

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    // ���ֶ������ͼ�ζ����������ֲ�ͬ�Ķ��������ͣ����Ƿֱ��ʾ֧���ڱ����ϳ���ͼ���ִ��ͼ�β����Ķ����塣
    // ���ܴ���һ�����������ͬһ���������֧�ֳ��ֲ�������֧��ͼ�β�����
    // �����Ļ������ǾͿ���ʹ��ͬһ��������������������ͬ���͵Ķ��У�ֻ��Ҫ�����Ƿ��䲻ͬ�����ȼ���

    
    // ���ֶ�����һ��Vulkan���У�������ִ�н��������е�ͼ����ʾ�ڱ����ϵĲ�����
    //      ���ֶ��и��𽫽������е�ͼ�񴫵ݸ����棬������һЩ��Ҫ��ͬ���ͱ任��
    //      ����������湵ͨ��;���������ǳ��ֶ��У��������������棬���Ǹ��𽫽������е�ͼ��ת��Ϊ������Խ��ܵĸ�ʽ��
    //      ������һЩ�ϳɺ͵����������Ӳ�������
    // ���ֶ��л����ͼ����һЩ������ͼ���������Ҫ�����¼��֣�
    //     ͬ�����������ֶ�����Ҫ����Ⱦ���кͱ������ͬ�����Ա�֤ͼ������ȷ��ʱ�䱻��ȡ����Ⱦ����ʾ��
    //          ͬ����������ʹ���ź�����դ�����¼��Ȼ���ʵ�֣��Ա��⾺�����������ݲ�һ�µ����⡣
    //     �任���������ֶ�����Ҫ���ݽ�������ָ���ı任��ʽ����ͼ�������ת������ȱ任����������Ӧ����ķ���Ͳ��֡�
    //          �任��������ʹ�ý���������ʱָ����currentTransform������vkSetSwapchainTransformKHR���������ơ�
    //     �ϳɲ��������ֶ�����Ҫ���ݽ�������ָ���ĺϳ�ģʽ����ͼ�����alpha��ϻ������ϳɲ���������Ӧ�����͸���Ⱥ͵���Ч����
    //          �ϳɲ�������ʹ�ý���������ʱָ����compositeAlpha������vkSetSwapchainCompositeAlphaKHR���������ơ�
    
    // ���ֶ��еĶ�����Ϊ�˽���������⣺
    // Vulkan��һ��ƽ̨�޹ص�API��������ֱ�Ӻʹ���ϵͳ������Ϊ�˽�Vulkan��Ⱦ��ͼ����ʾ�ڴ����ϣ�
    //      ������Ҫʹ��WSI��Window System Integration����չ���ṩһ������Ľӿڡ�
    // ��ͬ��ƽ̨�����в�ͬ�Ĵ���ϵͳ�ͳ��ֻ��ƣ�������Ҫһ��ͳһ�ķ�ʽ������ͬ�������
    //      ���ֶ����ṩ��һ�ֳ���㣬ʹ��Ӧ�ó�������������ƽ̨ϸ�ڣ�ֻ��Ҫ���ı������ͽ���������
    // ��ͬ�������豸�����в�ͬ�ĳ������������ƣ�������Ҫһ�����ķ�ʽ��ѡ����ʵ������豸�Ͷ����塣
    //      ���ֶ����ṩ��һ�ֲ�ѯ���ƣ�ʹ��Ӧ�ó�����Ը��ݱ���������ж������豸�Ƿ��������󣬲���ѡ����ʵĽ�����ģʽ�͸�ʽ��
    
    // �������������������飺
    // ����������������������������ڹ���ͳ���ͼ��
    //      ������������һ���������ͼ�񻺳��������飬ÿ��ͼ�񻺳���������������������ҿ��Ա���������ʹ�á�
    // ������������ύͼ�񻺳������������棬�����ڱ�������ʾͼ��
    //      �ύͼ�񻺳�����Ҫʹ��vkQueuePresentKHR���������ṩһ��VkPresentInfoKHR�ṹ�壬���а���Ҫ�ύ�Ľ����������ͼ��������
    // �������������ȡ��һ�����õ�ͼ�񻺳��������ڽ�����Ⱦ������
    //      ��ȡ��һ�����õ�ͼ�񻺳�����Ҫʹ��vkAcquireNextImageKHR���������ṩһ�������������һ����ʱֵ��
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
        // ���ڱ���Ĵ�����Ҫ�ڴ��������豸���߼��豸֮ǰ����Ϊ���ڱ����Ӱ�������豸��ѡ����߼��豸�����á�
        // ������Ҫ���ݴ��ڱ������ж������豸�Ƿ��������ǵ����󣬱����Ƿ�֧��presentQueue�����壬�Ƿ�֧�ֽ�������ʽ��ģʽ�ȡ�
        // ���ǻ���Ҫ���ݴ��ڱ�����ָ���߼��豸�������չ�����ԣ�����VK_KHR_swapchain��չ��
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
            // glfwCreateWindowSurface��GLFW���ṩ��һ������������������һ����GLFW���ڹ�����Vulkan�������
            // Vulkan���������������ʾ���Գ���ͼ��ĳ�������
            // ���е�ʱ���ڲ���������������8��
            // ���ݲ�ͬ��ƽ̨��������Ӧ��ƽ̨�ض���չ��������vkCreateWin32SurfaceKHR����vkCreateXcbSurfaceKHR��������һ����ԭ������ϵͳ������Vulkan�������
            // ��������Vulkan���������GLFW���ڶ�����й���������֮��������ٺͲ�ѯ������
            // ���ش����������Ľ��������ɹ����򷵻�VK_SUCCESS�����򷵻���Ӧ�Ĵ����롣
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
        // �ҵ�֧�ֳ��ֺ�ͼ�β����Ķ��������������������ֶ�����ͼ�ζ����ǲ�ͬ�ģ���ô��ҪΪ�߼��豸�趨����������
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };
        //std::set��C++��׼���е�һ�������������Դ洢һ�鲻�ظ���Ԫ�أ����Ұ���һ����˳���������
        //std::set����;�����¼���1��
        // �����������һ��Ԫ���Ƿ�����ڼ����У���Ϊstd::set�ڲ�ʹ��ƽ�������ʵ�֣����Բ��ҵ�ʱ�临�Ӷ���O(log n)��
        // ��������ȥ���ظ���Ԫ�أ���Ϊstd::set����������ͬ��Ԫ�ء�
        // ����������һ��Ԫ�ؽ���������Ϊstd::set���Զ�����Ԫ�ص�ֵ�����Զ���ıȽϺ�����������

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {// ����ÿ�����ظ��Ķ���������
            VkDeviceQueueCreateInfo queueCreateInfo{}; // ����һ���ṹ�壬�����洢���д�����Ϣ
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; // ���ýṹ������
            queueCreateInfo.queueFamilyIndex = queueFamily;     // ����Ҫʹ�õĶ���������
            queueCreateInfo.queueCount = 1;                     // ����Ҫ�����Ķ�������
            queueCreateInfo.pQueuePriorities = &queuePriority;  // ���ö������ȼ�ָ��
            queueCreateInfos.push_back(queueCreateInfo);        // ���ṹ����ӵ�������
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        // vector.data()���ص���һ���ڴ������һ��Ԫ�ص�ָ�룻
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
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue); // ���ú�������ȡ���ֶ��ж���
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        return indices.isComplete();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        // ����һ�������������洢�����豸֧�ֵĶ���������
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        // ���ú�������ȡÿ�������������VkQueueFamilyProperties
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {             // ����ÿ��������
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {   // ���������֧��ͼ�β���
                indices.graphicsFamily = i;                         // ����ǰ������ֵ��ͼ�ζ���������
            }

            // ���ú�������ѯ��ǰ�������Ƿ�֧�ָ����ı������
            VkBool32 presentSupport = false;// ����һ�������������洢��ǰ�������Ƿ�֧�ֱ������
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {                                   // ���֧�ֱ������
                indices.presentFamily = i;                          // ����ǰ������ֵ�����ֶ���������
            }                                                       // �󲿷�gpu��һ����ֶ�����ͼ�ζ���ͨ������ͬ��

            if (indices.isComplete()) {     // ���ͼ�ζ�����ͳ��ֶ����嶼�Ѿ��ҵ�
                break;                      // ����ѭ��
            }

            i++;                            // ������һ��������һ��������
        }

        return indices;                     // �����ҵ��Ķ����������ṹ��
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