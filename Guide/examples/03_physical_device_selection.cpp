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

//����������Ƿ��������豸�������豸�Ķ�������û��֧����Ӧ�����Ķ���(ͼ�μ���)�����ҳ�ʼ�������豸��

struct QueueFamilyIndices {
    // ����һ���ṹ�壬�����洢һ�������豸���Կ���֧�ֵĶ����壨queue family��������
    // ����ֻ��ע֧��ͼ�β����Ķ����������������ֻ��һ����ѡ��uint32_t���͵ĳ�Ա����
    
    // ��������һ�������ͬ���ԵĶ��У�queue���������������ύͼ�κͼ�������Ľӿ�
    
    // ͼ�ι��߰󶨵�֧��ͼ�β����Ķ������ϲ���ִ����Ⱦ����
    // ����Ϊ�����壬����Ϊ���Ƕ���ͨ��������ʵ�ֵģ���һ��ָ�������ִ���������д������յ�������ָ�������command buffer��
    // ָ�������һ��Ԥ�ȼ�¼�õ�ͼ�λ����ָ�������������Σ�����ͼ��ȡ�����ʵ��������GPU�����еģ����Ǹ���ִ���ύ�����ǵ�ָ�������

    // һ����������ִ��ͼ�β�������ô���Ͳ���ͬʱִ�м������������ȴ�ͼ�β�����ɺ����ִ�м��������
    // ���һ������֧�ֶ������͵Ĳ�������ô����Ҫ���л���ͬ���͵Ĳ���ʱ����ͬ��
    // ͨ��ʹ�ö��У�Vulkan�����ÿ����߸��õؿ��ƺ��Ż�GPU����Դ���ú͵��ȣ��Ӷ����Ӧ�ó����Ч�ʺ�������
    // ���磬�����߿��Ը��ݲ�ͬ������ѡ����ʵĶ�����ִ�в�ͬ���ͻ�ͬ���ȼ��Ĳ���������ʹ�ö��������ʵ�ֲ��л��첽�Ĳ�����

    // ������Ƶ�Ŀ��
    // ����CPU��������ͳ��ͼ��API����OpenGL��ʹ��ͬ����Ⱦģ�ͣ�����ζ����������������ÿ����Ⱦ������״̬����Դ����֤��������ȷ��˳��ִ�У�
    //              ������Ҫʱ����CPU�ȴ�GPU��ɹ�������Щ����������CPU��ʱ�����Դ��Ӱ������Ч�ʺ���Ӧ�ԡ�
    //              Vulkanʹ���첽��Ⱦģ�ͣ�����ζ�ſ����߿���ֱ�ӽ��������������У�ʹ����ʽ��������ϵ�����������ִ��˳��
    //              ��ʹ����ʽ��ͬ��ԭ����Э��CPU��GPU֮��Ĺ�������Щ�������Լ�����������ĸ���������CPU�Ŀ�������߳�������ܺ������ȡ�
    // ��ǿ���߳���������ͳ��ͼ��API����OpenGL��ʹ�û��������ģ�context������Ⱦģʽ������ζ��ÿ���߳�ֻ�ܷ���һ�������ģ���ÿ��������ֻ�ܰ󶨵�һ��GPU��
    //              ����ģʽ�����˶��߳���Ⱦ�Ŀ����ԣ���Ϊ��ͬ�߳�֮����ҪƵ�����л������ĺ�ͬ��״̬��
    //              Vulkanʹ�û��ڶ��У�queue������Ⱦģʽ������ζ��ÿ���߳̿��Է��ʶ�����У���ÿ�����п��԰󶨵����GPU��
    //              ����ģʽ��ǿ�˶��߳���Ⱦ����������Ϊ��ͬ�߳�֮����Բ��е��ύ�����ͬ���к�GPU��
    // ֧�ֶ����������ͣ���ͳ��ͼ��API����OpenGL����Ҫ֧��ͼ����Ⱦ��ص����񣬶������������͵���������㴦����ڴ洫�䣩��֧�ֽ�����ȱ����׼����
    //              Vulkan֧�ֶ����������ͣ�����ͼ����Ⱦ�����㴦���ڴ洫�䡢ϡ����Դ�󶨵ȡ�Vulkan�����������Զ�����չ��֧�ָ������͵�����
    //              Vulkanͨ����ͬ���͵Ķ����壨queue family�������ֲ�ͬ���͵�����ÿ��������ֻ֧��һ�ֻ�����ض����͵�����
    std::optional<uint32_t> graphicsFamily;
    // optional�����������¼��㣺
    //  optional����������ʾһ�����ܲ����ڵ�ֵ������һ�������ķ���ֵ��һ����ĳ�Ա����������һ�������Ĳ�����
    //  optional����ͨ��һЩ�������ж��Ƿ����ֵ������isPresent()��has_value()������������bool�������
    //  optional����ͨ��һЩ��������ȡ�������ð�����ֵ������get()��value()��of()��ofNullable()��orElse()��orElseGet()��orElseThrow()�ȡ�
    //  optional����ͨ��һЩ�������԰�����ֵ���в���������map()��flatMap()��filter()�ȡ�
    //  optional���Ա�����ʽ�ؽ��п�ֵ�����쳣������ߴ���ļ���Ժͽ�׳�ԡ�
    // optional��Ӧ�ó��������¼��֣�
    //  optional����������Ϊһ������ʧ�ܵĺ����ķ���ֵ���������Ա��ⷵ��null�����׳��쳣������ǿ�Ƶ����ߴ���û�н���������
    //  optional����������Ϊһ������Ϊ�յ���ĳ�Ա���������ͣ��������Ա���ʹ�ÿն���ģʽ��������ֵ�������������ķ�װ�ԺͲ����ԡ�
    //  optional����������Ϊһ������Ϊ�յķ����Ĳ��������ͣ��������Ա��⴫��null�������ط�������������˷����Ŀɶ��Ժ�����ԡ�
    
    // ����һ�������������ж��Ƿ��ҵ���֧��ͼ�β����Ķ�����
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

    // �����洢ѡ��������豸���Կ���
    // VkPhysicalDevice��ָ�������豸(�Կ�)�ľ��(ָ��)����Vulkan���ڲ�ά����һ�������豸������߽ṹ�壬�����޷�ֱ�ӷ��ʻ��޸�����
    // ���ǿ���ͨ��������ȡ�����豸�����Ժ����ԣ��������ƣ����ͣ��汾������ѹ����������ɫ���ȡ�
    // ����Ҳ����ͨ�����������߼��豸��logical device���������������豸���н�����
    // һ��������һ���Կ������ǿ����ж�������豸�������豸��ָ֧��Vulkan API��ͼ��Ӳ���������Կ����߼����Կ���
    // ��Щ�Կ����ܰ�����������豸������˫о�Կ�����SLI/CrossFireģʽ�µĶ���Կ���
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

    // ����һ�����������������п��õ������豸��ѡ��һ�����ʵ������豸
    void pickPhysicalDevice() {
        
        uint32_t deviceCount = 0;                                                   // ����һ�������������洢�����豸������
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);                // ����Vulkan API��������ȡ�����豸������������instance������ʾVulkanʵ�����󣬴���nullptr��ʾ����ȡ����������豸�б�

        
        std::cout << "deviceCount:" << deviceCount << std::endl;                    // �ҵĵ���ֻ��һ�������豸�������Կ�;
        if (deviceCount == 0) {                                                     // ���û���ҵ��κ�֧��Vulkan�������豸�����׳�һ������ʱ����
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);                         // ����һ��vector�����������洢���п��õ������豸
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());         // �ٴε���Vulkan API��������ȡ���п��õ������豸������instance������ʾVulkanʵ�����󣬴���devices.data()��ʾ��ȡ����������豸�б�

        
        for (const auto& device : devices) {                                        // �������п��õ������豸
            if (isDeviceSuitable(device)) {                                         // �����Զ���ĺ������жϵ�ǰ�����豸�Ƿ�������Ҫ�Ķ�����
                physicalDevice = device;                                            // ������㣬�ͽ��丳ֵ�����Ա�����豸physicalDevice��������ѭ����ָֻѡ��һ�������豸
                break;
            }
        }
        
        if (physicalDevice == VK_NULL_HANDLE) {                                     // ���û���ҵ��κκ��ʵ������豸�����׳�һ������ʱ����
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    // ����һ�������������ж�һ�������豸�Ƿ��������ǵ�����
    bool isDeviceSuitable(VkPhysicalDevice device) {
        // �����Զ���ĺ�������ȡ�������豸֧�ֵĶ����������
        QueueFamilyIndices indices = findQueueFamilies(device);

        // ���ظ������豸�Ƿ�֧��ͼ�β���
        return indices.isComplete();
    }

    // ����һ��������������ȡһ�������豸֧�ֵĶ����������
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        // ����һ��QueueFamilyIndices�ṹ����������洢����������
        QueueFamilyIndices indices;

        // ����һ�������������洢�����������
        uint32_t queueFamilyCount = 0;
        // ����Vulkan API��������ȡ�����������������device������ʾ�����豸���󣬴���nullptr��ʾ����ȡ����Ķ����������б�
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        // �ҵ��Կ�ֻ֧��һ��������
        std::cout << "queueFamilyCount:" << queueFamilyCount << std::endl;
        // ����һ��vector�����������洢���ж����������
        // VkQueueFamilyProperties�������������һ���ṹ�壨struct���������ڴ洢һ���������������Ϣ�����������¼�����Ա������
        //  queueFlags����ʾ�ö�����֧����Щ���͵Ĳ���������ͼ�Ρ����㡢�ڴ洫��ȡ�
        //  queueCount����ʾ�ö�����������ٸ����С�
        //  timestampValidBits����ʾ�ö�����֧�ֵ�ʱ�������λ����
        //  minImageTransferGranularity����ʾ�ö�����֧�ֵ���Сͼ�������ȡ�
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        // �������豸�л�ȡ�����壬queueFamilies.data()��ʾ��ȡ����Ķ����������б�
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        // ����һ��������������Ϊ�����������ļ�����
        int i = 0;
        // �������ж����������
        for (const auto& queueFamily : queueFamilies) {
            // �ҵ�Ψһ�Ķ�������ֻ��һ������
            std::cout << "queueCount:" << queueFamily.queueCount << std::endl;
            // �жϵ�ǰ�������Ƿ�֧��ͼ�β��������֧�֣��ͽ���������ֵ��indices.graphicsFamily
            // queueFlags��һ��λ��bit field������ʾ�ö�����֧����Щ���͵Ĳ���������ͼ�Ρ����㡢�ڴ洫��ȡ�queueFlags�����¼���ֵ�����ǿ��Ե���ʹ�û������ʹ�ã�
            //  VK_QUEUE_GRAPHICS_BIT����ʾ�ö�����֧��ͼ�β�����������Ⱦ���ߣ�����ָ��ȡ�
            //  VK_QUEUE_COMPUTE_BIT����ʾ�ö�����֧�ּ�����������������ɫ��������ָ��ȡ�
            //  VK_QUEUE_TRANSFER_BIT����ʾ�ö�����֧���ڴ洫����������縴�ƻ�����������ͼ��ȡ�
            //  VK_QUEUE_SPARSE_BINDING_BIT����ʾ�ö�����֧��ϡ����Դ�󶨲����������ϡ�軺������ϡ��ͼ��ȡ�
            //  VK_QUEUE_PROTECTED_BIT����ʾ�ö�����֧�ֱ����ڴ���������紴����ʹ�ñ����ڴ����ȡ�
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                // VK_QUEUE_GRAPHICS_BIT��һ��������constant��������ֵ��1������ʾһ������������binary number��00000001������ֻ�����λ��least significant bit����1������λ����0��
                // queueFlags����15��ʾһ����������00001111�����������λ����1������λ����0��
                // ����ʾ�ҵ���Ψһ�Ķ�����ͬʱ֧��ͼ�Ρ����㡢�ڴ洫���ϡ����Դ�󶨲�����
                indices.graphicsFamily = i;
            }

            // �ж��Ƿ��Ѿ��ҵ���֧��ͼ�β����Ķ����壬����ǣ�������ѭ��
            if (indices.isComplete()) {
                break;
            }

            // ������������һ
            i++;
        }

        // ���ض����������ṹ�����
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