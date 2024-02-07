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
    // ͼ����ͼ��image view����һ����������������η���һ��ͼ��image�����ض����ֺ͸�ʽ��
    // ͼ����ͼ����Ⱦ·����render pass��������������descriptor set��ʹ�ã��Ա�����Ⱦ��������ɫ����ʹ��ͼ��
    // һ��ͼ����ͼ�涨�˽�������һ��ͼ����α�ʹ������ʾ��
    // ͼ����ͼ���Ծ�����������ͼ���ά�ȡ���ʽ����ɫͨ��������Դ��Χ��
    
    // һ��ͼ�������Դ��subresource����ָͼ���һ���֣�������һ������;��aspect����ϸ������mip level��������㣨array layer����һ��ͼ������ж������Դ�����ǹ���ͬһ��ͼ�����ݴ洢�ռ�1��
    
    // һ��ͼ��������ɫ��color������ȣ�depth����ģ�壨stencil����ϸ������mip level����ͼ��㣨array layer����ɵ�
    // ��ɫ��color����ָͼ���ÿ�����ص���ɫֵ���������ɲ�ͬ����ɫͨ����color channel����ɣ���RGB��HSV��CMYK�ȡ�
    // ��ȣ�depth����ָͼ����ÿ�����ص�����ľ��룬����������ʵ����Ȳ��ԣ�depth test�������ж���Щ�����ǿɼ��ģ���Щ�Ǳ��ڵ���4��
    // ģ�壨stencil����ָͼ����ÿ�����ص�ģ��ֵ������������ʵ��ģ����ԣ�stencil test����������һЩ����������Щ���ؿ��Ա����ƻ��޸�5��
    // ϸ������mip level����ָͼ��Ĳ�ͬ�ֱ��ʰ汾������������ʵ�ֶ༶��������ӳ�䣨mipmap���������ݹ۲����ѡ����ʵķֱ�������Ⱦͼ�񣬴Ӷ�������ܺ�������
    // ͼ��㣨array layer����ָͼ��Ĳ�ͬ����������������ʵ����������ͼ��cube map������ά����3D texture������������array texture�������������άͼ��ѵ������γ�һ�������ӵ�ͼ��ṹ��
    
    // VkImageView��һ���Ƿ��ɾ��VK_DEFINE_NON_DISPATCHABLE_HANDLE������ɷ��ɾ��VK_DEFINE_HANDLE������ͬ
    // �Ƿ��ɾ����non-dispatchable handle����һ�����ڱ�ʶVulkan������������ͣ�����һ��64λ������������Ψһ��ʶһ������
    // �Ƿ��ɾ����ɷ��ɾ������������
    //  �Ƿ��ɾ������ľ����һ��64λ����������Ҫͨ������ָ��������ö�����صĺ���������ֱ�Ӵ��ݸ�������Ϊ���������õ���ϵͳ�ײ�api��ֻ��Ҫvulkan�ڲ�api��
    //  �Ƿ��ɾ��ͨ�����ڱ�ʾ����Ҫ���豸���ʵ������Ķ�����ͼ����ͼ��image view������������buffer���ȡ�
    //  �ɷ��ɾ������ľ����һ��ָ�����ͣ���ָ��һ������������Ϣ�ͺ���ָ���Ľṹ�壬������Ҫ����ϵͳ�ײ�api��
    //  �ɷ��ɾ��ͨ�����ڱ�ʾ��Ҫ���豸���ʵ������Ķ������豸��device����ʵ����instance���ȡ�

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
        // Ϊ�������е�ÿ��ͼ�����һ��ͼ����ͼ����Ŀռ�
        swapChainImageViews.resize(swapChainImages.size());
        // ͼ����ͼ������Ҫһ��VkImageViewCreateInfo�ṹ��
        // �������³�Ա����4��
        //  sType������ָ���ṹ������ͣ�����ΪVK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO��
        //  pNext������ָ����չ�ṹ���ָ�룬һ��Ϊnullptr��
        //  flags������ָ��������־��Ŀǰ����Ϊ0��
        //  image������ָ��Ҫ������ͼ��ͼ�����
        //  viewType������ָ����ͼ�����ͣ�������һά����ά����ά����������ͼ�ȡ�
        //      ��һ��ö�����ͣ�������ָ��ͼ����ͼ��image view�������͡��������°˸�ֵ��
        //      VK_IMAGE_VIEW_TYPE_1D����ʾһάͼ����ͼ��ֻ�п��һ��ά�ȡ�
        //      VK_IMAGE_VIEW_TYPE_2D����ʾ��άͼ����ͼ���п�Ⱥ͸߶�����ά�ȡ�
        //      VK_IMAGE_VIEW_TYPE_3D����ʾ��άͼ����ͼ���п�ȡ��߶Ⱥ��������ά�ȡ�
        //      VK_IMAGE_VIEW_TYPE_CUBE����ʾ��������ͼ��cube map��ͼ����ͼ����������άͼ����ɣ�ÿ��ͼ���Ӧ�������һ���档
        //      VK_IMAGE_VIEW_TYPE_1D_ARRAY����ʾһά����ͼ����ͼ���ɶ��һάͼ����ɣ�ÿ��ͼ���Ӧ�����һ���㣨layer����
        //      VK_IMAGE_VIEW_TYPE_2D_ARRAY����ʾ��ά����ͼ����ͼ���ɶ����άͼ����ɣ�ÿ��ͼ���Ӧ�����һ���㡣
        //      VK_IMAGE_VIEW_TYPE_CUBE_ARRAY����ʾ��������ͼ����ͼ����ͼ���ɶ����������ͼ��ɣ�ÿ����������ͼ��Ӧ�����һ���㡣
        //      VK_IMAGE_VIEW_TYPE_MAX_ENUM����ʾö�����͵����ֵ������һ����Ч��ֵ��
        //  format������ָ����ͼ��ɫ�ĸ�ʽ�����Ժ�ͼ��ĸ�ʽһ�»���ݡ�
        //  components������ָ����ɫͨ����ӳ�䷽ʽ�����Զ�ÿ��ͨ�����к�ȡ��㡢һ��ת�Ȳ�����
        //      �����ĸ�������VkComponentSwizzle(rbga)��VkComponentSwizzle��һ��ö�����ͣ�������ָ����ɫͨ����color channel����ӳ�䷽ʽ��
        //      VkComponentSwizzle����������ֵ��
        //      VK_COMPONENT_SWIZZLE_IDENTITY����ʾ���ӳ�䣬������ԭ���ķ������䡣���磬R->R, G->G, B->B, A->A��
        //      VK_COMPONENT_SWIZZLE_ZERO����ʾ��ӳ�䣬����ԭ���ķ����滻Ϊ0�����磬R -> 0, G -> 0, B -> 0, A -> 0��
        //      VK_COMPONENT_SWIZZLE_ONE����ʾһӳ�䣬����ԭ���ķ����滻Ϊ1�����磬R -> 1, G -> 1, B -> 1, A -> 1��
        //      VK_COMPONENT_SWIZZLE_R����ʾ��ӳ�䣬����ԭ���ķ����滻Ϊ����������磬R->R, G->R, B->R, A->R��
        //      VK_COMPONENT_SWIZZLE_G����ʾ��ӳ�䣬����ԭ���ķ����滻Ϊ�̷��������磬R->G, G->G, B->G, A->G��
        //      VK_COMPONENT_SWIZZLE_B����ʾ��ӳ�䣬����ԭ���ķ����滻Ϊ�����������磬R->B, G->B, B->B, A->B��
        //      VK_COMPONENT_SWIZZLE_A����ʾ͸����ӳ�䣬����ԭ���ķ����滻Ϊ͸���ȷ��������磬R->A, G->A, B->A, A->A��
        //  subresourceRange������ָ������Դ��Χ������Ҫ���ʵ�ͼ�����Щ����������ɫ����ȡ�ģ��ȣ�����Щϸ���������Щ����㡣
        //      ����һ��VkImageSubresourceRange�ṹ�壬������ָ������Դ��Χ��subresource range����VkImageSubresourceRange�������ĸ���Ա������
        //      aspectMask������ָ��Ҫ���ʵ�ͼ�����Щ��������������ɫ��color������ȣ�depth����ģ�壨stencil���������ģ�壨depth stencil����
        //          aspectMask��һ��λ��bit field����������ָ��Ҫ���ʵ�ͼ��image������Щ����;
        //          aspectMask��������˸�ֵ�е�һЩ�ͣ�Ҳ���ǿ���ͬʱ����ͼ��Ķ�����������磬���aspectMaskΪVK_IMAGE_ASPECT_COLOR_BIT |
        //          aspectMask�����°˸�ֵ2��
        //          VK_IMAGE_ASPECT_COLOR_BIT����ʾҪ����ͼ�����ɫ��������ÿ�����ص���ɫֵ��
        //          VK_IMAGE_ASPECT_DEPTH_BIT����ʾҪ����ͼ�����ȷ�������ÿ�����ص�����ľ��롣
        //          VK_IMAGE_ASPECT_STENCIL_BIT����ʾҪ����ͼ���ģ���������ÿ�����ص�ģ��ֵ��
        //          VK_IMAGE_ASPECT_METADATA_BIT����ʾҪ����ͼ���Ԫ���ݷ�������һЩ�������Ϣ����ѹ����ʽ�ȡ�
        //          VK_IMAGE_ASPECT_PLANE_0_BIT����ʾҪ����ͼ��ĵ�һ��ƽ����������ڶ�ƽ���ʽ��ͼ����YUV�ȡ�
        //          VK_IMAGE_ASPECT_PLANE_1_BIT����ʾҪ����ͼ��ĵڶ���ƽ����������ڶ�ƽ���ʽ��ͼ����YUV�ȡ�
        //          VK_IMAGE_ASPECT_PLANE_2_BIT����ʾҪ����ͼ��ĵ�����ƽ����������ڶ�ƽ���ʽ��ͼ����YUV�ȡ�
        //          VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT����ʾҪ����ͼ��ĵ�һ���ڴ�ƽ����������ڶ��ڴ��ʽ��ͼ��
        //      baseMipLevel������ָ��Ҫ���ʵ�ͼ������ϸ�����𣬴�0��ʼ������
        //      levelCount������ָ��Ҫ���ʵ�ͼ���ϸ����������������ΪVK_REMAINING_MIP_LEVELS����ʾ��baseMipLevel��ʼ�����ϸ�����������
        //      baseArrayLayer������ָ��Ҫ���ʵ�ͼ����������㣬��0��ʼ������
        //      layerCount������ָ��Ҫ���ʵ�ͼ������������������ΪVK_REMAINING_ARRAY_LAYERS����ʾ��baseArrayLayer��ʼ���������������
        
        // �����������е�ÿ��ͼ��
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};// ����һ��ͼ����ͼ������Ϣ�ṹ�壬����ָ��ͼ����ͼ�Ĳ���
            
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;// ���ýṹ�������
            createInfo.image = swapChainImages[i];                      // ����Ҫ������ͼ��ͼ�����
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;                // ������ͼ�����ͣ������Ƕ�άͼ��
            createInfo.format = swapChainImageFormat;                   // ������ͼ�ĸ�ʽ������ͽ������ĸ�ʽһ��
            // ������ɫͨ����ӳ�䷽ʽ��������Ĭ�ϵĺ��ӳ��
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            // ��������Դ��Χ������ָ��Ҫ���ʵ�ͼ�����һ���ֺ���;
            // ����ֻ��������ͼ�����ɫ������û��ϸ������������
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;     // ������mipmap
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;     // ͼ��ֻ��һ��

            // ����vkCreateImageView������ʹ�ô�����Ϣ�ṹ����豸���󣬴���һ��ͼ����ͼ���󣬲��洢��������
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