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


// ����������ǰһ��������һ���߼��豸������ȡ������߼��豸��ͼ�ζ�����
// Vulkan���������豸�ĸ��ʵ����instance���������豸��physical device�����߼��豸��logical device����
// ʵ����VulkanӦ�ó������ڵ㣬����ʾ��һ����Vulkan����ʱ�����ӡ�
//      ʵ��������ǲ�ѯ�����豸��ִ��һЩ�豸����ǰ�Ĵ��룬����ָ����֤���ʵ����չ�ȡ�
//      ʵ����չ��һЩ����Ĺ��ܣ����細��ϵͳ���ɣ����Ա���ȡ�
//      ʵ����չ��Ҫĳ�̶ֳȵļ��س���֧�֣����س�����Vulkan����ʱ��һ���֣��������κ�GPU����
//      ����չ�����豸����ǰ�Ĵ��벿�֣����������豸�������Ϊ��
// �����豸��Ӧ�˵����ϵ�һ��GPU������ʾ��GPU��Ӳ�����Ժ�֧�ֵĹ��ܡ�
//      �����豸����ͨ��ʵ����ѯ������ÿ�������豸����һ��Ψһ�ľ����handle������ʶ����
//      �����豸�����ṩһЩ��Ϣ������֧�ֵĶ����壨queue family�����豸��չ��device extension���������豸���ԣ�physical device feature���ȡ�
// �߼��豸�ǻ���һ�������豸�����ģ�����ʾ���������豸���н����Ľӿڡ�
//      �߼��豸�������Vulkan����һ��������豸�Ͻ�����Ⱦ����ͨ�ü������Ϊ��

// ���Ǵ�����˳���ǣ�
//  �ȴ���ʵ����vkCreateInstance����������Vulkan����ʱ�������ӡ�
//  �ٲ�ѯ�����豸��vkEnumeratePhysicalDevices�������ڻ�ȡ���õ�GPU��Ϣ��
//  ��󴴽��߼��豸��vkCreateDevice���������������豸���н�����
// ���ǵ�����˳���ǣ�
//  �������߼��豸��vkDestroyDevice���������ͷ��������豸��ص���Դ��
//  ������ʵ����vkDestroyInstance�������ڶϿ���Vulkan����ʱ�����ӡ�
//  �����Ҫ���������豸����Ϊ��������Vulkan����ʱ����ġ�
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
    // Vulkan�д����߼��豸��Ҫָ�����¼�����Ϣ��
    //  �����豸�����VkPhysicalDevice��������ѡ��Ҫʹ�õ�GPU��
    //  ���д�����Ϣ��VkDeviceQueueCreateInfo��������ָ��Ҫʹ����Щ������Ͷ��С�
    //      �ڴ����߼��豸ʱ����Ҫָ��Ҫʹ����Щ�����壬�Լ�ÿ����������Ҫʹ�ö��ٸ����С�
    //      ÿ��������Ͷ��ж���Ҫһ��VkDeviceQueueCreateInfo�ṹ�����������ǵ����������������ȼ���
    //  �豸��չ��Ϣ��VkDeviceExtensionProperties��������ָ��Ҫ������Щ�豸��չ��
    //      �豸��չ��һЩ����Ĺ��ܣ����罻������swapchain������������Ⱦͼ�γ��ֵ�����ϵͳ��
    //      ��ͬ�������豸����֧�ֲ�ͬ���豸��չ���ڴ����߼��豸ʱ����Ҫָ��Ҫ������Щ�豸��չ���Լ�����������κβ�����
    //  �����豸������Ϣ��VkPhysicalDeviceFeatures��������ָ��Ҫʹ����Щ�����豸���ԡ�
    //      �����豸������һЩӲ�����ܣ����缸����ɫ����geometry shader�������ز�����multisampling���ȡ�
    //      ��ͬ�������豸����֧�ֲ�ͬ�������豸���ԡ��ڴ����߼��豸ʱ����Ҫָ��Ҫʹ����Щ�����豸���ԣ��Լ�����������κβ�����
    //  ��֤����Ϣ��VkLayerProperties��������ָ��Ҫ������Щ��֤�㡣
    //      ��֤����Է�Ϊʵ����֤����豸��֤�㡣ʵ����֤��ֻ����ȫ��Vulkan������صĵ��ã�����Vulkanʵ����
    //      �豸��֤��ֻ�����ض�GPU��صĵ��á�
    
    // Vulkan��ʹ���߼��豸���������¼����£�@@@@@
    // ��������������Vulkan���󣬱��绺������buffer����ͼ��image�������ߣ�pipeline���ȡ�
    //      ��Щ������Ҫһ���߼��豸�����ָ�����������ĸ��߼��豸���Լ����ǵ�һЩ���ԺͲ�����
    // ������ͷ��ڴ棬���ڴ洢Vulkan������������ݡ�Vulkan�е��ڴ��������ʽ�ģ���Ҫ�������Լ�������ͷ��ڴ档
    //      �߼��豸�ṩ��һЩ������������ͷ��ڴ棬�Լ���ѯ�����豸���ڴ����Ժ�����
    // ��¼���ύָ�������command buffer�������ڴ洢ͼ�λ����ָ�ָ�������һ��Ԥ�ȼ�¼�õ�ͼ�λ����ָ�������������Σ�����ͼ��ȡ�
    //      �߼��豸�ṩ��һЩ����������������ָ��������Լ��������ύ���������ִ�С�
    // ͬ���Ͳ�ѯ����״̬�����ڱ�֤����֮�����ȷ˳��ͽ����Vulkan�еĲ������첽�ģ���Ҫ�������Լ�ͬ���Ͳ�ѯ���ǵ�״̬��
    //      �߼��豸�ṩ��һЩ����������������ͬ�����󣬱����ź�����semaphore����դ����fence�����¼���event���ȣ��Լ���ѯ������״̬�ͽ����
    
    VkQueue graphicsQueue;
    // VkQueue��һ�����������ʾһ��������ж�����һ��Ӳ����Դ��
    //      ����Vulkan�������ύͼ�κͼ�������Ľӿڣ�������GPU�����У�����ִ���ύ�����ǵ�ָ�������
    // �ύָ�������vkQueueSubmit�������ڽ�һ��ͼ�λ����ָ��͸�GPUִ�С�
    //      ÿ��ָ���������Ҫһ��������о����ָ����Ҫ�ύ���ĸ�������У��Լ�����һЩ���ԺͲ�����
    // ���ֽ�����ͼ��vkQueuePresentKHR�������ڽ���Ⱦ�õ�ͼ����ʾ������ϵͳ�ϡ�
    //      ÿ��������ͼ����Ҫһ��������о����ָ����Ҫ���ֵ��ĸ�������У��Լ�����һЩ���ԺͲ�����
    // �ȴ����п��У�vkQueueWaitIdle�������ڵȴ���������������ύ�Ĳ�����
    //      ���������������ͬ�������ϵĲ����������������߼��豸֮ǰ�ȴ�����������в�����

    // VkQueue��VkDevice���ǲ�͸�����(opaque handle)��������һЩָ��Vulkan�����ָ�룬�������ǵľ������ͺ����ݶ���Ӧ�ó����ǲ��ɼ��ġ�
    //      Ӧ�ó���ֻ��ͨ��Vulkan�ṩ�ĺ������������ǣ�����ֱ�ӷ��ʻ��޸����ǵ��ڲ����ݡ�
    // ��͸���������������¼����ô���
    //     ����������Vulkan�����ʵ��ϸ�ڣ�ʹ��API���Ӽ���һ�¡�
    //     �����Ա�֤Vulkan�����״̬�����ݵ������ԣ�����Ӧ�ó�������ǽ��зǷ������Ĳ�����
    //     ���������Vulkan�������ֲ�Ժͼ����ԣ�ʹ�ò�ͬ��ƽ̨����������ʹ�ò�ͬ�ķ�ʽ��ʵ�ֺ͹������ǡ�
    // Vulkan�г���VkQueue��VkDevice֮�⣬���кܶ������Ĳ�͸���������������VK_DEFINE_HANDLE����ģ�
    //      ����VkInstance, VkPhysicalDevice, VkBuffer, VkImage, VkPipeline�ȡ�
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

    
    void createLogicalDevice() {    // ���ڴ����߼��豸����ȡ�߼��豸�еĶ���
        // ���֧��ͼ�ζ��еĶ���������
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        VkDeviceQueueCreateInfo queueCreateInfo{};          // ����һ���ṹ�壬���������豸������Ĵ�����Ϣ
        // VkDeviceQueueCreateInfo��һ���ṹ�壬�������˴�����������������Ϣ�����磺
        // ������������queueFamilyIndex��������ָ��Ҫʹ���ĸ������塣
        //      ÿ�������豸����һ�����������壬ÿ�������嶼��һ��Ψһ����������ʶ����
        // ����������queueCount��������ָ��Ҫʹ�øö������еĶ��ٸ����С�
        //      ÿ�������嶼��һ�������ö�����Ŀ�����ܳ��������Ŀ��
        // �������ȼ���pQueuePriorities��������ָ��ÿ�����е����ȼ�����Χ��0.0��1.0��
        //      ���ȼ������˵��������ͬʱ�ύ����ʱ���ĸ����лᱻ����ִ�С�
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; // ����Ϊ���ֵ
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();  // ָ������������
        queueCreateInfo.queueCount = 1;                     // ���ö��е�����������ֻʹ����һ������
        float queuePriority = 1.0f;                         // ����һ�������������ڱ�ʾ����������ȼ� 
        queueCreateInfo.pQueuePriorities = &queuePriority;  // ���ýṹ����ָ��������ȼ������ָ�룬��ͬ���в�ͬ���ȼ�

        VkPhysicalDeviceFeatures deviceFeatures{};          // ����һ���ṹ�壬�������������豸������
        //VkPhysicalDeviceFeatures��һ���ṹ�壬��������һϵ�в���ֵ��
        // ��Щ����ֵ��ʾ�����豸֧�ֵ�һЩӲ�����ܣ����缸����ɫ����geometry shader�������ز�����multisampling���ȡ�
        // ʾ�����룺
        // VkPhysicalDeviceFeatures deviceFeatures{};
        // deviceFeatures.geometryShader = VK_TRUE; // ���ü�����ɫ��
        // deviceFeatures.sampleRateShading = VK_TRUE; // ���ö��ز���


        VkDeviceCreateInfo createInfo{};                    // ����һ���ṹ�壬���������߼��豸�Ĵ�����Ϣ
        
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;            // ���ýṹ�������
        createInfo.queueCreateInfoCount = 1;                // �����豸���д�����Ϣ����Ĵ�С������ֻ��һ��������
        createInfo.pQueueCreateInfos = &queueCreateInfo;    // ���ýṹ����ָ���豸���д�����Ϣ�����ָ��
        createInfo.pEnabledFeatures = &deviceFeatures;      // ���ýṹ����ָ�������豸���Խṹ���ָ��
        
        createInfo.enabledExtensionCount = 0;               // �������õ���չ����������û�������κ���չ
        if (enableValidationLayers) {                       // �����������֤�㣬���������õ���֤������������   
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {                                              // ���û��������֤�㣬���������õ���֤������Ϊ0
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            // ����Vulkan API���������ڴ����߼��豸����������洢��device������
            // �������ʧ�ܣ����׳�һ������ʱ�����쳣
            throw std::runtime_error("failed to create logical device!");
        }

        // ����Vulkan API���������ڻ�ȡ�߼��豸�е�ͼ�ζ��У���������洢��graphicsQueue������
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