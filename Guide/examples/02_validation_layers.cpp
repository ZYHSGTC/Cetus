#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};  // ����һ�������ַ��������Ķ��壬���ڴ洢��Ҫ���õ�У�������ƣ�����ֻ������һ��ͨ�õ�У���
    // KURONOS��һ�����ʷ�ӯ����֯���������ƶ���ά��һЩ���ŵı�׼������OpenGL��Vulkan��ͼ��API��

#ifdef NDEBUG                               // ����һ��Ԥ����ָ������ж��Ƿ�����NDEBUG�꣬��������ˣ���ʾ����Ϊ����ģʽ
const bool enableValidationLayers = false;  // ����һ���������������Ķ��壬���������Ƿ�����У��㣬��������Ϊfalse����ʾ������
#else                                       // ����һ��Ԥ����ָ������ж��Ƿ�û�ж���NDEBUG�꣬���û�ж��壬��ʾ����Ϊ����ģʽ
const bool enableValidationLayers = true;   // ����һ���������������Ķ��壬���������Ƿ�����У��㣬��������Ϊtrue����ʾ����
#endif                                      // ����һ��Ԥ����ָ����ڽ�����������

// VkResult��һ��ö�����ͣ������ڱ�ʾVulkan API�������õķ���ֵ�������ǳɹ���������������ʱ�Ĵ����롣
// Utils��utility�ĸ�����ʽ��utility��һ��Ӣ�ﵥ�ʣ���˼��ʵ�ó���򹤾ߡ�
// EXT��extension����д��extension��һ��Ӣ�ﵥ�ʣ���˼����չ�����䡣
// CreateDebugUtilsMessengerEXT��һ���Զ���ĺ��������ڴ���һ��������Ϣ�����������������ڿ���ʱ���ͱ������;��档
// ���Ĳ����ֱ��ǣ�
//  instance��       һ��Vulkanʵ���������ڳ�ʼ��Vulkan�����Vulkan�������򽻻���
//  pCreateInfo��    һ��ָ�������Ϣ��������Ϣ�ṹ���ָ�룬���ڴ洢������Ϣ�������������Ϣ�����������Ϣ�����ؼ������ͺͻص������ȡ�
//  pAllocator��     һ��ָ���ڴ����ص�������ָ�룬�����Զ����ڴ������ͷŵķ�ʽ�����ʹ��Ĭ�Ϸ�ʽ����������Ϊnullptr��
//                  ����������Ϣ��������Ҫһ���ڴ�ص�������ָ�룬����Ϊ���ָ�������Զ����ڴ������ͷŵķ�ʽ��
//                  Ҳ����һ�����ڿ��Ƶ�����Ϣ��������ռ���ڴ�ռ�ĺ�����
//                  ��������������Ϳ����ڴ��������ٵ�����Ϣ������ʱ�������Լ���������Ż����������������ڴ�ռ䡣
//                  �������Ҫ�Զ����ڴ����ʽ�����Խ����ָ������Ϊnullptr����ʾʹ��Ĭ�ϵ��ڴ������ͷŷ�ʽ��
//  pDebugMessenger��һ��ָ�������Ϣ�������ָ�룬���ڽ��մ����ɹ���ĵ�����Ϣ��������(ָ��)��
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // PFN��pointer to function����д����ʾ����ָ������͡�
    // ProcAddr��procedure address����д����ʾ������ַ��
    // ���Ĳ����ֱ��ǣ�
    //  instance��һ��Vulkanʵ���������VK_NULL_HANDLE������ָ����ȡ������ַ�ķ�Χ�����ΪVK_NULL_HANDLE����ֻ�ܻ�ȡȫ�ַ�Χ��ʵ����Χ�ĺ�����ַ�����Ϊһ����Ч��ʵ���������ܻ�ȡ�豸��Χ�ĺ�����ַ��
    //  pName��һ��ָ���Կ��ַ���β���ַ�����ָ�룬����ָ��Ҫ��ȡ��ַ�ĺ��������ơ�
    // ����һ����ֵ��䣬���ڻ�ȡVulkan���е�vkCreateDebugUtilsMessengerEXT�����ĵ�ַ����ת��ΪPFN_vkCreateDebugUtilsMessengerEXT���͵ĺ���ָ�롣
    // vkCreateDebugUtilsMessengerEXT��һ��Vulkan API���������ƣ������ڽ��մ����ĵ�����Ϣ������
    // ���������������һ���ģ����������޸Ļ��滻������ᵼ�»�ȡ������ַʧ�ܻ��ߵ��ô���ĺ�����
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        // ����������ûᴴ��һ��������Ϣ�����󣬷��صĽ����һ��VkResultö��ֵ
        // ����ʾ�Ƿ�ɹ�������һ��������Ϣ�����󣬲���������ֵ��pDebugMessengerָ��ָ��ı�����
        // �������ֵ����VK_SUCCESS����ʾ�����ɹ����������ֵ����VK_ERROR_EXTENSION_NOT_PRESENT����ʾ��չ�����ڣ�
        // �������ֵ�������������룬��ʾ����ʧ�ܡ�
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

//  ����һ���Զ���ĺ�������������������һ��������Ϣ�����󣬲����ֱ�Ϊ��Vulkanʵ������Ҫ���ٵĵ�����Ϣ�������Զ����ڴ����ص�����ָ�룻
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    // ����һ����ֵ��䣬���ڻ�ȡVulkan���е�vkDestroyDebugUtilsMessengerEXT�����ĵ�ַ����ת��ΪPFN_vkDestroyDebugUtilsMessengerEXT���͵ĺ���ָ��
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

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
    // ��������һ�����ƻص����������ڽ��պʹ������Ե��Բ���¼�
    // ���ǿ���������ʱע�뵽vulkan����ĵ������У�����Ƿ�����κδ��󡢾�������������⡣
    // ���ǿ����ڴ���vulkanʵ��ʱ���õ��Բ㣬��ָ��Ҫ���õ���֤������ơ�
    // ���ǿ���ͨ��vkCreateDebugReportCallbackEXT������������������������һ���û��Զ���ĺ���ָ���һ�����Ȥ���¼���־��
    // �¼���־���������¼���֮һ������ϣ�VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_DEBUG_BIT_EXT��
    // ÿ����һ�����ϱ�־�������¼�����ʱ���ͻᴥ���ص�������
    // �����¼������͡���Ϣ������Ȳ������ݸ��û��Զ���ĺ�����
    // �û���������������д�ӡ����¼���ߴ�����Щ�¼���

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }// Ҫ����vkDestroyInstanceִ��

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }// �����֧��У��㣬��ôcheckValidationLayerSupport�᷵��false

        VkApplicationInfo appInfo{};
        appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName    = "Hello Triangle";
        appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName         = "No Engine";
        appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion          = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // ������������չ��
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());// vector.size() ���ص���һ�� size_t ���͵�ֵ������һ���޷�����������
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};   // ������������洢����������Ϣ�������������Ϣ
        if (enableValidationLayers) {   // ����Ƿ���������֤�㡣���if��else��ȥ������һ�����У����ǲ����ڿ�ʼ���������һ����ʱ����������ȡ���������ʱ��ĵ�����Ϣ
            // �����������֤�㣬��ô������ createInfo �ṹ���е�����ֶΡ�
            // ���� createInfo �ṹ���е� enabledLayerCount �ֶ�Ϊ validationLayers ����Ĵ�С��
            // validationLayers ������һ���ַ����������洢��������Ҫʹ�õ���֤������
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            // ���� populateDebugMessengerCreateInfo ���������� debugCreateInfo ������������Ϊ������
            // ������������������Ҫʹ�õĵ��Թ�������� debugCreateInfo �ṹ���е��ֶ�
            populateDebugMessengerCreateInfo(debugCreateInfo);
            // ���� createInfo �ṹ���е� pNext �ֶ�Ϊ debugCreateInfo �����ĵ�ַ����ǿ��ת��Ϊ VkDebugUtilsMessengerCreateInfoEXT ���͵�ָ�롣
            // ����ֶα�ʾ������Ҫ���ӵ� Vulkan ʵ���ϵ���չ�ṹ��
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
            // �����vkCreateInstance�����н�pCreateInfo��pNext��Ա�趨Ϊһ��VkDebugUtilsMessengerCreateInfoEXT�ṹ��
            // ��ô�����ڴ���ʵ�������ͬʱ����һ����ʱ�ĵ�����Ϣ������
            // ���������Ϣ������ֻ��vkCreateInstance��vkDestroyInstance������ִ���ڼ���Ч�����ڲ����ڴ���������ʵ������ʱ�������¼����翪ʼ�����ʱ����ûص��������������Ϣ��
            // �����Ҫ����һ���־õĵ�����Ϣ������������ʹ��vkCreateDebugUtilsMessengerEXT����,Ҳ����setupDebugMessenger��������
        }
        else {
            // ���û��������֤�㣬��ô������ createInfo �ṹ���е� enabledLayerCount �ֶ�Ϊ0����ʾ���ǲ���Ҫ�����κ���֤��
            createInfo.enabledLayerCount = 0;
            // ���� createInfo �ṹ���е� pNext �ֶ�Ϊ nullptr����ʾ���ǲ���Ҫ�����κ���չ�ṹ��
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    // ����ֵ������void�������Ǵ��������õ�����Ϣ������
    void setupDebugMessenger() {
        //����һ��������䣬��������Ƿ���������֤�㡣
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        // ������������createInfo�ṹ���е���Ϣ������һ��������Ϣ�����󣬲����丳ֵ��debugMessenger������
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        // populate������������˼
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        // VkDebugUtilsMessengerCreateInfoEXT����������һ���ṹ�壬�����洢����������Ϣ�������������Ϣ��
        // ������Ϣ����������ڷ�������Ȥ���¼�ʱ����һ�����Իص��������������ͼ�¼Vulkan API�Ĵ���;��档
        // ����ṹ����������ֶΣ�
        // sType��һ��VkStructureTypeֵ����ʾ����ṹ������͡���������VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT��
        // pNext��һ��ָ�룬��ʾ���ӵ�����ṹ�����չ��Ϣ����������NULL����ָ�������ṹ�塣
        // flags��һ��VkDebugUtilsMessengerCreateFlagsEXTֵ����ʾ�����ı�־λ����������0��
        // messageSeverity��һ��VkDebugUtilsMessageSeverityFlagBitsEXT��λ���룬��ʾ��Щ������¼��ᴥ���ص�������������������ֵ�����2��
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT����ʾ��ϸ���¼���ͨ���������Ŀ�ġ�
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT����ʾ��Ϣ�Ե��¼���ͨ������֪ͨĿ�ġ�
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT����ʾ�����Ե��¼���ͨ��������ʾ���ܵ����⡣
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT����ʾ�����Ե��¼���ͨ�����ڱ������
        // messageType��һ��VkDebugUtilsMessageTypeFlagBitsEXT��λ���룬��ʾ��Щ���͵��¼��ᴥ���ص�������������������ֵ�����2��
        //      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT����ʾһ���Ե��¼���ͨ����Vulkan API�޹ء�
        //      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT����ʾ��֤�Ե��¼���ͨ����Vulkan API����ȷʹ���йء�
        //      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT����ʾ�����Ե��¼���ͨ����Vulkan API��Ч���йء�
        // pfnUserCallback��һ��PFN_vkDebugUtilsMessengerCallbackEXT���͵ĺ���ָ�룬��ʾ�û�����Ļص������������������ÿ�δ����¼�ʱ�����ã���������Ӧ�Ĳ�����
        // pUserData��һ��ָ�룬��ʾ�û��Զ�������ݡ�������ݻᱻ���ݸ��ص�������
    }

    // ����һ��������������getRequiredExtensions������ֵ�������ַ������飬�����ǻ�ȡVulkanʵ���������չ��
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;    // ����һ���޷�������������������glfwExtensionCount����ʼ��Ϊ0�������洢GLFW���������չ����
        const char** glfwExtensions;        // ����һ��ָ���ַ���ָ���ָ�������������glfwExtensions�������洢GLFW���������չ��
            //char**���ַ���������
            //char* names[] = { "Alice", "Bob", "Charlie" }; // ����һ���ַ�������
            //char** p = names; // ����һ��ָ���ַ��������ָ��
            //cout << p[0] << endl; // ��� Alice
            //cout << p[1] << endl; // ��� Bob
            //cout << p[2] << endl; // ��� Charlie

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        // ����GLFW���ṩ��һ����������������᷵��GLFW�������Vulkanʵ����չ����������glfwExtensionCount��ֵ
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        // ����һ���ַ������������������extensions����ʹ��glfwExtensions��glfwExtensionCount��ʼ������
        // ������������洢����Vulkanʵ���������չ��������GLFW������ĺ�����������Ҫ��
        // const char* ��һ��ָ�����ַ���ָ�룬Ҳ����˵���������޸���ָ����ַ���ֵ��
        //      const char* arr[] = {"one", "two", "three"}; // ����һ���ַ�������
        //      std::vector<const char*> vec(arr, arr + 3); // ʹ��������׵�ַ��β��ַ��Ϊ����������ʼ������
        //      cout << vec[0] << endl; // ��� one
        //      cout << vec[1] << endl; // ��� two
        //      cout << vec[2] << endl; // ��� three

        if (enableValidationLayers) { // �����������֤�㣬��ô����һ����չ
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); 
            // VK_EXT_DEBUG_UTILS_EXTENSION_NAME ��һ���궨�壬����ֵ��һ���ַ���������VK_EXT_debug_utils��
            // ��extensions���������һ��Ԫ�أ���ʾVulkan���Թ�����չ�����ơ�
        }

        return extensions;
    }

    bool checkValidationLayerSupport() {    // �����Ǽ��Vulkan API�Ƿ�֧����֤��
        uint32_t layerCount;                // �����洢���õ���֤������
        // ����Vulkan API�ṩ��һ������������layerCount�ĵ�ַ�Ϳ�ָ����Ϊ��������������᷵�ؿ��õ���֤��������������layerCount��ֵ
        // Enumerate��ö��
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        // �ҵ�vulkan����֧��11��ļ���㣻
        std::cout << layerCount << std::endl;

        // ����һ��VkLayerProperties���͵����������ʹ��layerCount��ʼ������������������洢���õ���֤������
        // VkLayerProperties��һ���ṹ�����ͣ������洢Vulkan API3�п��õ���֤������ԡ�
        //  layerName��һ���ַ����飬��ʾ��֤������ơ�
        //  specVersion��һ���޷�����������ʾ��֤������ѭ��Vulkan�淶�İ汾�š�
        //  implementationVersion��һ���޷�����������ʾ��֤���ʵ�ְ汾�š�
        //  description��һ���ַ����飬��ʾ��֤���������Ϣ��
        std::vector<VkLayerProperties> availableLayers(layerCount);
        // �ٴε���Vulkan API�ṩ��һ������������layerCount�ĵ�ַ��availableLayers������ָ����Ϊ������
        // ��������᷵�ؿ��õ���֤�����ԣ�������availableLayers��ֵvector.data()���ص���һ��ָ�롣
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // ����һ��ѭ����䣬����validationLayers�����е�ÿһ��Ԫ�ء�
        for (const char* layerName : validationLayers) {
            // ����һ������ֵ������������layerFound������ʼ��Ϊfalse����������������������Ҫʹ�õ���֤���Ƿ��ڿ��õ���֤�����ҵ�
            bool layerFound = false;
            
            // vailableLayers������һ��VkLayerProperties���͵����飬�洢�˿��õ���֤������
            for (const auto& layerProperties : availableLayers) {
                // ʹ��strcmp�����Ƚϵ�ǰ����������֤�����ƺͿ�����֤�������е������Ƿ���ͬ
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            // ����ڱ��������п�����֤���layerFound������ȻΪfalse��
            // ��ô˵��������Ҫʹ�õ���֤�㲻���ã���ô�ͷ���false��Ϊ�����Ľ��������������ִ��
            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        // ����һ����̬������������ debugCallback
        // ������һ���¼�ʱ��Vulkan API������¼���ص���Ϣ�洢��һ��VkDebugUtilsMessengerCallbackDataEXT�ṹ���У�����Ϊ�������ݸ��ص�������
        // ����ֵ������ VkBool32��һ��32λ�޷����������ͣ�������ʾ����ֵ����������VK_TRUE��VK_FALSE��    
        // VKAPI_ATTRȫд��Vulkan Application Programming Interface Attribute����ʾVulkanӦ�ó���ӿ����ԡ�
        // VKAPI_CALL�����ں��������еķ�������֮������MSVC��ʽ�ĵ���Լ���﷨3��VKAPI_CALL��һ���궨�壬�������ƺ����ĵ���Լ��������stdcall��cdecl�ȡ�
        // ���ǵ��������ò�ͬ�ı�������ƽ̨�ܹ���ȷ�ص���Vulkan API�ĺ��������ǵľ�������Ǹ��ݱ�������ƽ̨�����ͣ�չ��Ϊ��ͬ�Ĺؼ��ֻ���ţ��������κ�����������
        // ��һ����ʹ�� VKAPI_ATTR �� VKAPI_CALL ���壬ȷ�������Ա� Vulkan ����á�
        
        // VKAPI_ATTR����չ��Ϊ���¹ؼ���֮һ���ջ�
        // __declspec(dllexport)��һ���������ԣ�����ָʾ���������ú�������Ϊ��̬���ӿ⣨DLL����һ�����ţ�������������Ϳ���ͨ����̬����DLL�����øú�����
        // __attribute__((visibility(��default��))��һ���������ԣ�����ָʾ���������ú�������Ϊ�������SO����һ�����ţ�������������Ϳ���ͨ����̬����SO�����øú�����
        // VKAPI_CALL����չ��Ϊ���¹ؼ���֮һ���ջ�
        // __stdcall������MSVC���ı���������ʾһ�ֵ���Լ�������ɱ����ú��������ջ�����Һ�����ǰ�����»��ߺ���ϲ����ֽ�����Ϊ��׺��
        // __cdecl������MSVC���ı���������ʾһ�ֵ���Լ�������ɵ����������ջ�����Һ�����ǰֻ�����»�����Ϊǰ׺��
        // MSVC��Microsoft Visual C++����д����ʾ΢���һ��C++�������ͼ��ɿ�������
  
        // ����1��VkDebugUtilsMessageSeverityFlagBitsEXT��һ��ö�����ͣ���ʾ�¼��������Լ���������������ֵ֮һ�������ǵ���ϣ�
        //  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT����ʾ��ϸ���¼���ͨ���������Ŀ�ġ�
        //  VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT����ʾ��Ϣ�Ե��¼���ͨ������֪ͨĿ�ġ�
        //  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT����ʾ�����Ե��¼���ͨ��������ʾ���ܵ����⡣
        //  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT����ʾ�����Ե��¼���ͨ�����ڱ�����󡣲���������
        // ����2��VkDebugUtilsMessageTypeFlagsEXT��һ��λ�������ͣ���ʾ�¼������͡�������������ֵ֮һ�������ǵ���ϣ�
        //  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT����ʾһ���Ե��¼���ͨ����Vulkan API�޹ء�
        //  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT����ʾ��֤�Ե��¼���ͨ����Vulkan API����ȷʹ���йء�
        //  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT����ʾ�����Ե��¼���ͨ����Vulkan API��Ч���йء�
        // ����3��VkDebugUtilsMessengerCallbackDataEXT��һ���ṹ�����ͣ���ʾ������Ϣ�����ݡ������������ֶΣ�
        //  sType��һ��VkStructureTypeֵ����ʾ����ṹ������͡���������VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT��
        //  pNext��һ��ָ�룬��ʾ���ӵ�����ṹ�����չ��Ϣ����������NULL����ָ�������ṹ�塣
        //  flags��һ�������ı�־λ����������0��
        //  pMessageIdName��һ��ָ���ַ�����ָ�룬��ʾ��ϢID���ơ���������NULL����ָ��һ���Կ��ַ���β���ַ�����
        //  messageIdNumber��һ��32λ��������ʾ��ϢID���롣��������0��������ֵ��
        //  pMessage��һ��ָ���ַ�����ָ�룬��ʾ��Ϣ���ݡ���������NULL�����ұ���ָ��һ���Կ��ַ���β���ַ�����
        //  queueLabelCount��һ���޷�����������ʾ���б�ǩ������Ԫ����������������0��������ֵ��
        //  pQueueLabels��һ��ָ��VkDebugUtilsLabelEXT�ṹ�������ָ�룬��ʾ���б�ǩ���顣��������NULL����ָ����Ч�ڴ��ַ��
        //  cmdBufLabelCount��һ���޷�����������ʾ���������ǩ������Ԫ����������������0��������ֵ��
        //  pCmdBufLabels��һ��ָ��VkDebugUtilsLabelEXT�ṹ�������ָ�룬��ʾ���������ǩ���顣��������NULL����ָ����Ч�ڴ��ַ��
        //  objectCount��һ���޷�����������ʾ����������Ԫ����������������0��������ֵ��
        //  pObjects��һ��ָ��VkDebugUtilsObjectNameInfoEXT�ṹ�������ָ�룬��ʾ�������顣��������NULL����ָ����Ч�ڴ��ַ��
        
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        // ʹ�� std::cerr �������һ��������Ϣ��������֤������ƺ͵�����Ϣ�����ݡ�
        // pCallbackData ��һ��ָ�� VkDebugUtilsMessengerCallbackDataEXT �ṹ���ָ�룬��ʾ������Ϣ�����ݡ�
        // pCallbackData->pMessage ��һ��ָ���ַ�����ָ�룬��ʾ������Ϣ������
        return VK_FALSE;
    }
};

int main() {
    HelloTriangleApplication app;

    // ����һ���޷������������������洢Loader�İ汾��
    uint32_t loaderVersion = 0;
    // ����Vulkan API�ṩ��һ������������NULL��loaderVersion�ĵ�ַ��Ϊ��������������᷵��Loader�İ汾�ţ�������loaderVersion��ֵ
    vkEnumerateInstanceVersion(&loaderVersion);
    // ���������޷������������������洢Loader�����汾�š��ΰ汾�źͲ����汾��
    uint32_t major = VK_VERSION_MAJOR(loaderVersion);
    uint32_t minor = VK_VERSION_MINOR(loaderVersion);
    uint32_t patch = VK_VERSION_PATCH(loaderVersion);
    // ��ӡLoader�İ汾����Ϣ
    printf("Vulkan Loader Version: %u.%u.%u\n", major, minor, patch);

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}