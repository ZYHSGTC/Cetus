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
    // VkRenderPass��Vulkan�е�һ��������������һ����Ⱦ�����еĸ�����attachment������ͨ����subpass����������dependency�����Լ�����֮��Ĺ�ϵ��ת����
    // VkRenderPass��������������֪����Ⱦ�����롢������м������Ӷ������Ż��͵��ȣ�������ܺͽ�ʡ����

    // ��Ӳ���ڴ��У�VkRenderPass����ʽȡ����GPU�ļܹ����͡�һ����˵����������Ҫ��GPU�ܹ����ͣ�����ģʽ��Ⱦ����IMR���ͻ���ͼ�����Ⱦ����TBR����
    // ��IMR�У�GPU������ִ������������д��֡�������С�
    //      ���ַ�ʽ���ڴ����Ҫ��ϸߣ����Ƕ���Ⱦ˳��Ҫ��ϵ͡�
    //      ����������£�VkRenderPass���԰��������������µ�������Ա��ⲻ��Ҫ��ͬ����ͼ�񲼾�ת����
    // ��TBR�У�GPU�Ὣ��Ļ�ָ�ɶ��С�飨tile��������ÿ��tile�ļ������ݻ��浽Ƭ���ڴ棨on - chip memory���С�
    //      Ȼ��GPU���������ÿ��tile���������д��֡�������С����ַ�ʽ���ڴ����Ҫ��ϵͣ����Ƕ���Ⱦ˳��Ҫ��ϸߡ�
    //      ����������£�VkRenderPass���԰�����������ȷ����ʱ�����ݽ�����뿪Ƭ���ڴ棬�����ж��Ƿ���Ҫ�����ݷ��õ��ڴ����tile�ڵ�ȫ�����ݡ�
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
        // �ṹ��VkAttachmentDescription��������һ����Ⱦ������ʹ�õĸ��������ԣ������ʽ�������������غʹ洢����������ת����.
        //  �����³�Ա������
        //  flags��һ��VkAttachmentDescriptionFlagBitsö��ֵ��ָ�������Ķ������ԡ�
        //  format��һ��VkFormatö��ֵ��ָ������ʹ�õ�ͼ����ͼ�ĸ�ʽ��
        //  samples��һ��VkSampleCountFlagBitsö��ֵ��ָ������ʹ�õ�ͼ��Ĳ�������
        //  loadOp�����ز�����ָ������Ⱦ��ʼʱ������������Ӧ����δ����Ը����ļ��ز����Ƕ���subpass���Եģ�ֻ��subpass����·��
        //      ��һ��VkAttachmentLoadOpö��ֵ�������¿��ܵ�ֵ��
        //      VK_ATTACHMENT_LOAD_OP_LOAD�����ز�������Ⱦ��ʼʱ���ڴ��ж�ȡ�������ݡ�
        //      VK_ATTACHMENT_LOAD_OP_CLEAR�������������Ⱦ��ʼʱ��һ������ֵ����������ݡ�
        //      VK_ATTACHMENT_LOAD_OP_DONT_CARE�������Ĳ�������Ⱦ��ʼʱ���ܸ������ݣ�����δ���塣
        //      VK_ATTACHMENT_LOAD_OP_NONE_EXT��������ͼ������Ⱦ�����ڵ����ݣ�ͼ���������δ����ġ�
        //  storeOp���洢����,ָ��������Ⱦͨ������ʱ�Ը����Ĵ���ʽ�����������Ⱦ���������ɵ����������Ƿ������ڴ棬�Լ��������ĸ��ڴ�����
        //      ��һ��VkAttachmentStoreOpö��ֵ,�����¼��ֿ��ܵ�ֵ��
        //      VK_ATTACHMENT_STORE_OP_STORE������ͼ������Ⱦ�����ڵ����ݡ�
        //      VK_ATTACHMENT_STORE_OP_DONT_CARE��������ͼ������Ⱦ�����ڵ����ݣ�ͼ���������δ����ġ�
        //      VK_ATTACHMENT_STORE_OP_NONE_EXT��������ͼ������Ⱦ�����ڵ����ݣ�ͼ���������δ����ġ�
        //  stencilLoadOp��һ��VkAttachmentLoadOpö��ֵ��ָ��������ģ������ڵ�һ��ʹ��������ͨ����ʼʱ�Ĵ���ʽ��
        //  stencilStoreOp��һ��VkAttachmentStoreOpö��ֵ��ָ��������ģ����������һ��ʹ��������ͨ������ʱ�Ĵ���ʽ��
        //  
        //  initialLayout��һ��VkImageLayoutö��ֵ����ʼ���֣�ָ����randerpass��Ⱦ��ʼǰ����δ��ڴ��ж�ȡ������ͼ�񲼾�
        //  finalLayout��һ��VkImageLayoutö��ֵ�����ղ��֣�ָ����randerpass��Ⱦ��������δ��ڴ���д�븽����ͼ�񲼾֡�
        //      ͼ�񲼾֣�VkImageLayout��ָ����ͼ������Դ���ڴ��еĴ洢��ʽ��һ��ͼ������Դ������Ϊһ����������һ������ֻ�ܰ���һ��ͼ������Դ��
        //      VkImageLayout������ָ��ͼ������Ⱦ�����л������������еĲ���ת���������¼��ֿ��ܵ�ֵ��
        //      VK_IMAGE_LAYOUT_UNDEFINED��ͼ��û���ض��Ĳ��֣����Դ��κ���������ת����������֡�
        //      VK_IMAGE_LAYOUT_GENERAL��ͼ����Ա��κβ������ʣ������ܿ��ܲ��ѡ�
        //      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL��ͼ����Ϊ��ɫ����ʹ�ã�ֻ�ܱ���ɫ�������ʡ�
        //      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL��ͼ����Ϊ���ģ�帽��ʹ�ã�ֻ�ܱ����ģ�帽�����ʡ�
        //      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL��ͼ����Ϊֻ�����ģ�帽��ʹ�ã�ֻ�ܱ����ģ�帽������ɫ�����ʡ�
        //      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL��ͼ����Ϊֻ������ʹ�ã�ֻ�ܱ���ɫ�����ʡ�
        //      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL��ͼ����Ϊ����Դʹ�ã�ֻ�ܱ�����������ʡ�
        //      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL��ͼ����Ϊ����Ŀ��ʹ�ã�ֻ�ܱ�����������ʡ�
        //      VK_IMAGE_LAYOUT_PREINITIALIZED��ͼ���Ѿ���������ʼ�������Դ��������ת�����������֡�
        //      VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL��ͼ����Ϊֻ����ȺͿ�дģ�帽��ʹ�ã�ֻ�ܱ����ģ�帽������ɫ�����ʡ�
        //      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL��ͼ����Ϊ��д��Ⱥ�ֻ��ģ�帽��ʹ�ã�ֻ�ܱ����ģ�帽������ɫ�����ʡ�
        //      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR��ͼ����Ϊ����������Դʹ�á�
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;                      // ���ø����ĸ�ʽ���뽻����ͼ��ĸ�ʽ��ͬ
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                    // ���ø����Ĳ�����������Ϊ���ز���
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               // ���ø����ļ��ز���������Ϊ�����������ʾ��Ⱦ��ʼʱ��һ������ֵ�����������
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             // ���ø����Ĵ洢����������Ϊ�洢��������ʾ��Ⱦ����ʱ����������д���ڴ���
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    // ���ø�����ģ����ز���������Ϊ�����Ĳ�������ʾ��Ⱦ��ʼʱ����ģ�����ݣ�����δ����
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // ���ø�����ģ��洢����������Ϊ�����Ĳ�������ʾ��Ⱦ����ʱ����ģ�����ݣ�����δ����
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // ���ø����ĳ�ʼ���֣�����Ϊδ����Ĳ��֣���ʾ������֮ǰ�Ĳ��֣�Ҳ����������
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      // ���ø��������ղ��֣�����Ϊ����Դ�Ż��Ĳ��֣���ʾ���ڳ��ֵ���Ļ��

        // �����˸�������Ҫ���帽�����õ�ԭ���ǣ�
        //  ��������ֻ�Ƕ����˸�����һ�����ԣ�������ָ���˸�������Ⱦ�����еľ�����; ��
        //  ����������ָ���˸������������еľ�����;��������Ϊ��ɫ���������ģ�帽�������븽���� ��
        //  �������û�ָ���˸������������еľ��岼�֣�������ɫ�������š����ģ�帽�����ŵ� ��
        //  ͨ�����帽�����ã�������Vulkan֪���������������ʹ�ú�ת������ ��
        // �ṹ��VkAttachmentReference�����趨renderpass��subpass�����ø�������Щ������renderpass���и����е�һ����
        //  attachment��һ������ֵ����ʾ��VkRenderPassCreateInfo::pAttachments�ж�Ӧ�������ĸ���
        //      ֵΪVK_ATTACHMENT_UNUSED��ʾ�ø���δ��ʹ�á�
        //  layout��һ��VkImageLayoutö��ֵ��ָ����������ͨ����ʹ�õĲ��֡�
        //      һ����˵��VkAttachmentReference�е�layout����Ӧ����VkAttachmentDescription�е�initialLayout����finalLayout�е�һ��һ�£�����ȡ���ڸ���������Ⱦͨ���е�һ��ʹ�û������һ��ʹ�á�
        //      ���һ����������Ⱦͨ����ֻ��һ����ͨ��ʹ�ã���ô����layout����Ӧ����finalLayoutһ�£������Ϳ��Ա������Ĳ���ת��3��
        //      ���һ����������Ⱦͨ�����ж����ͨ��ʹ�ã���ô����layout����Ӧ����initialLayoutһ�£������Ϳ��Ա�֤�ڵ�һ����ͨ����ʼʱ�Ѿ�����˲���ת��3
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // ����������Ⱦ�����е�һ���׶Σ���������һ����Ⱦ������ʹ�õĸ����������̿����໥���ӣ��γ�һ����������Ⱦ���̡�
        //  ����������Ⱦ���������еĹ�ϵ����Ϊһ����Ⱦ������һ��������������ɡ������̿��Թ���ͬһ����Ⱦͨ�������֡��������Լ�ͬһ�鸽����
        //  �����̻�������ͬһ��ͼ���Ͻ��ж����Ⱦ��ʵ��һЩ�߼�����Ⱦ�����������ӳ���ɫ��
        //  ÿ�������̶���ʹ��һ����Ⱦ���ߣ���Ϊÿ�������̶���Ҫ����������ݽ��д�����������ͬ�������̿���ʹ�ò�ͬ����Ⱦ�������ã�����������͡���ɫ�������ģʽ�ȡ�
        // 
        // �����������±�Ҫ����ɲ��� :
        //  �󶨵㣺ָ����������ʹ�õĹ������ͣ�����ͼ�ι��߻������ߡ�
        //  ��ɫ�������ã�ָ����������ʹ�õ���ɫ�������Լ��������������еĲ��֡�
        //  ���ģ�帽�����ã�ָ����������ʹ�õ����ģ�帽�����Լ������������еĲ��֡�
        //  ���븽�����ã�ָ����������ʹ�õ����븽�����Լ��������������еĲ��֡�
        //  �����������ã�ָ���������̲�ʹ�õ���Ҫ�����ĸ������Լ��������������еĲ��֡�
        //  ���߲��֣�ָ����������ʹ�õĹ��߲��֣����������������ֺ����ͳ�����Χ��
        // 
        // ��������gpu�о����������¼����£�
        //  ��֡�������framebuffer object���л�ȡ������Ϊ���룬���ߴ�ǰһ�������̵�����л�ȡ���븽����input attachment����
        //  ����������������subpass description����ָ���İ󶨵㣨binding point����ѡ��ʹ��ͼ�ι��ߣ�graphics pipeline�����߼�����ߣ�compute pipeline����
        //  ����������������ָ���Ĺ��߲��֣�pipeline layout��������Ӧ������������descriptor set�������ͳ�����push constant�������ṩ��ɫ����������ݡ�
        //  ����������������ָ������ɫ�������ã�color attachment reference�������ģ�帽�����ã�depth stencil attachment reference����������ȾĿ�꣨render target�����������ͼ��λ�á�
        //  ִ����Ⱦ���߻��߼�������еĸ����׶Σ�����������ݽ��д������������ͼ�����ݡ�
        //  ����������������ָ���ı����������ã�preserve attachment reference������������Ҫ����ĸ������Ա����������ʹ�á�
        // 
        // �����̵�������һ�鸽����attachment������������Ⱦ������ʹ�õ�ͼ��������ɫ���塢��Ȼ��塢ģ�建��ȡ�
        //      �����̵����뻹����һЩ���븽����input attachment������������ǰһ������������Ϊ�������ʹ�ù���ͼ�񣬿����ڵ�ǰ����������Ϊ����ʹ�á�
        // �����̵������һ�鸽�������ǰ�������Ⱦ�������ͼ�����ݣ�����������ʾ������Ϊ������Ⱦ���̵����롣
        //      �����̵����������һЩ����������preserve attachment�����������ڵ�ǰ�������в���Ҫʹ�ã����ں����������п�����Ҫʹ�õ�ͼ��
        // 
        //  �����̻�ʹ��������Ⱦ���ߣ�����Ϊÿ�������̶���Ҫ����������ݽ��д���������
        //      ��ͬ�������̿���ʹ�ò�ͬ����Ⱦ�������ã�����������͡���ɫ�������ģʽ�ȡ�
        //      �׶���������ָ����Щ���߽׶���Ҫ�ȴ��򱻵ȴ����Ա�֤��ȷ��ִ��˳����ڴ���ʡ�
        //      ���磬���Դ��������Ҫ����ɫ��������׶���ɣ���Ŀ����������Ҫ�ڶ�����ɫ���׶ο�ʼ��
        //          ��ô����Ҫ����Դ�׶�����ΪVK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT��Ŀ��׶�����ΪVK_PIPELINE_STAGE_VERTEX_SHADER_BIT��
        // 
        // 
        // ������Դ��������ָ��������ϵ����ִ�е������̡����������Դ��������VK_SUBPASS_EXTERNAL����ô��ʾ��������Ⱦ�����е��κ�һ�������̣�������Ⱦ���̿�ʼǰ���������ⲿ��
        // ������������ָ����������֮���ͬ�����ƣ������Ա�֤��һ����������ɺ󣬵ڶ��������̲ſ�ʼ����������������ָ��Դ�����̺�Ŀ�������̵Ľ׶�����ͷ������룬�Լ�һЩ�����ѡ��12��
        // Դ��������Ŀ����������ʲô��˼��
        //  Դ�����̣�source subpass����ָ��������������ϵ����ִ�е������̡�
        //  Ŀ�������̣�destination subpass����ָ��������������ϵ�к�ִ�е������̡�
        //  ������������ϵ��subpass dependency����ָ����������֮���ͬ�����ƣ������Ա�֤��һ����������ɺ󣬵ڶ��������̲ſ�ʼ��
        //      ������������ϵ����ָ��Դ�����̺�Ŀ�������̵Ľ׶�����ͷ������룬�Լ�һЩ�����ѡ�
        // ��Ⱦ������render dependency����ָ������Ⱦ����֮���ͬ�����ƣ������Ա�֤��һ����Ⱦ������ɺ󣬵ڶ�����Ⱦ���̲ſ�ʼ��
        //  ��Ⱦ�������Է�Ϊ�������ͣ���ʽ��Ⱦ������ʽ��Ⱦ������
        //      ��ʽ��Ⱦ������ָVulkan�Զ���������Ⱦ�����������Ա�֤ͬһ�������е�������Ⱦ���̰����ύ˳��ִ�С���ʽ��Ⱦ��������Ҫ�û�ָ���κβ��������������ܲ������Ż��ġ�
        //      ��ʽ��Ⱦ������ָ�û��ֶ���������Ⱦ�����������Ա�֤��ͬ�����л��߲����ڵ���Ⱦ���̰����û������˳��ִ�С�
        //          ��ʽ��Ⱦ������Ҫ�û�ָ��һЩ����������Դ��Ŀ�������������Դ��Ŀ��׶����롢Դ��Ŀ��������롢Դ��Ŀ�긽�����ֵȡ���ʽ��Ⱦ���������ṩ���õ����ܺ�����ԡ�
        //      ��Ⱦ��������ʲôʱ�������ã�ȡ�������ǵ����ͺͲ�����
        //          һ����˵����ʽ��Ⱦ������ͬһ�������е�������Ⱦ����֮�������ã�����ʽ��Ⱦ�����ڲ�ͬ�����л��߲����ڵ���Ⱦ����֮�������á�
        //          ��Ⱦ�����ı������õĵ�����ʲô����ȡ�������ǵĲ�����һ����˵����Ⱦ�����ı������õĶ���������¼��֣�
        //          �׶Σ�stage������ʾ������ִ���ض�������һ�����衣�׶ο��Է�Ϊ��ͬ�����ͣ����綥����ɫ���׶Ρ�ƬԪ��ɫ���׶Ρ���ɫ��������׶εȡ�
        //          ���ʣ�access������ʾ���ڴ����Դ���ж�ȡ��д�������һ�����͡����ʿ��Է�Ϊ��ͬ�����ͣ�������ɫ����д����ʡ����ģ�帽����ȡ���ʡ���ɫ����ȡ���ʵȡ�
        //          ������attachment������ʾ��Ⱦ������ʹ�õ�ͼ��������ɫ���塢��Ȼ��塢ģ�建��ȡ�
        //          ���֣�layout������ʾ�������ڴ��е���֯��ʽ���Լ������ڲ�ͬ�׶��еĿ����ԡ����ֿ��Է�Ϊ��ͬ�����ͣ�������ɫ�������Ų��֡����ģ�帽�����Ų��֡�����Դ���ֵȡ�

        // �ⲿ������ָ����Ⱦ����֮��ִ�е�Vulkan�������vkAcquireNextImageKHR��vkQueuePresentKHR12���ⲿ���������Ҫ����Ⱦ������ʹ�õĸ�������ͬ���������Ҫ��������������
        // ���������������Ⱦ���ߵ������ʲô�������͵����
        //  ���������command buffer����һ���洢Vulkan����Ķ��󣬿������豸��ִ�С�
        //  ���������������Ⱦ���ߵ���������������͵�������磺
        //      �������copy command���������ڲ�ͬ���ڴ�λ��֮�临�����ݣ������CPU��GPU���ߴ�һ��ͼ����һ��ͼ��
        //      �������compute command��������ִ�м�����ߣ�compute pipeline��������һ������ִ��ͨ�ò��м�������Ĺ������͡�
        //      ͬ�����synchronization command���������ڲ�ͬ������֮�佨��������ϵ���Ա�֤��ȷ��ִ��˳����ڴ���ʡ�

        // �ṹ��VkSubpassDescription�����³�Ա����3��
        //  flags������������ʹ�á�
        //  pipelineBindPoint��һ��VkPipelineBindPointö��ֵ��ָ������ͨ���󶨵�ͼ�ι��߻��Ǽ�����ߡ�
        //      VK_PIPELINE_BIND_POINT_GRAPHICS��ָ����Ϊͼ�ι��ߡ�ͼ�ι�������ִ�л�������������㡢���Ρ�����ϸ�֡���դ����Ƭ�εȽ׶Ρ�
        //      VK_PIPELINE_BIND_POINT_COMPUTE��ָ����Ϊ������ߡ������������ִ�зַ����ֻ����һ������׶Ρ�
        //      VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR��ָ����Ϊ����׷�ٹ��ߡ�����׷�ٹ�������ִ�й���׷����������������ɡ��������С�������С�û�����кͿɵ��õȽ׶Ρ�
        //  inputAttachmentCount������ͨ��ʹ�õ����븽����������
        //  pInputAttachments��һ��ָ��VkAttachmentReference�ṹ�������ָ�룬��������ͨ��ʹ�õ����븽����
        //      ���븽����input attachment����һ�ֿ�����Ƭ����ɫ���ж�ȡ�ĸ���.
        //      ͨ�����ڴ���һ����ͨ����subpass����ͬƬ��λ����д���֡������������ȡ���ݡ�
        //      ���븽������������ܺͽ�ʡ�����ر����ڻ���ͼ�����Ⱦ����TBR���ϡ�
        //      ���븽��ֻ����Ϊ��ɫ������ʹ�ã�������Ϊ��ɫ�����ģ�����ʹ�á�
        //      ��ɫ���������ģ�帽��������Ϊ����ʹ�ã���������Ҫ�ڲ�ͬ����ͨ����subpass��֮�����ͼ�񲼾�ת����image layout transition����
        //          ͼ�񲼾�ת����һ�ְ���Ĳ���������Ӱ�����ܺʹ��������븽����input attachment�����Ա�������ת����
        //          ��Ϊ���ǿ�����ͬһ����ͨ������Ϊ���������ʹ�á������Ϳ��Խ�ʡ��Դ��ʱ�䣬�����ȾЧ�ʡ�
        //      ���븽������һ�������ǣ����ǿ����ڻ���ͼ�����Ⱦ����TBR���Ϸ��Ӹ��õ����á�
        //          ����ͼ�����Ⱦ����һ�ֽ���Ļ�ָ�ɶ��С�飨tile������Ⱦ�ļܹ�������������Ƭ���ڴ棨on - chip memory��������ÿ��ͼ�����ɫ��������ݡ�
        //          �����Ϳ��Լ��ٶ�ȫ���ڴ棨global memory���ķ��ʣ��Ӷ���ʡ���ĺʹ���
        //          ���븽������ֱ�Ӵ�Ƭ���ڴ��ж�ȡ���ݣ�������Ҫ��ȫ���ڴ��м��ػ�洢���ݡ�������ƶ��豸��˵�Ƿǳ�����ġ�
        //  colorAttachmentCount������ͨ��ʹ�õ���ɫ�����ͽ���������������
        //  pColorAttachments��һ��ָ��VkAttachmentReference�ṹ�������ָ�룬��������ͨ��ʹ�õ���ɫ������
        //      ��ɫ������color attachment����һ�ֿ�����Ϊ��ɫ����ĸ�����ͨ�����ڴ洢Ƭ����ɫ���������ɫֵ��
        //      ��ɫ���������ж����ÿ����Ӧһ����ɫ��������color buffer����
        //      ��ɫ����������Ϊ��ɫ����������ʹ�ã�Ҳ������Ϊ����Դ��Ŀ��ʹ�á�
        //  pResolveAttachments��һ��ָ��VkAttachmentReference�ṹ�������ָ�룬��������ͨ��ʹ�õĽ����������������Ҫ������������ΪNULL��
        //      ����������resolve attachment����һ�����ڶ��ز�����multisampling���ĸ���..
        //      ͨ�����ڽ����ز�������ɫ�����ģ�帽������Ϊ���ز�����ͼ��
        //      ��������ֻ����Ϊ���ʹ�ã�������Ϊ����ʹ�á�
        //  pDepthStencilAttachment��һ��ָ��VkAttachmentReference�ṹ���ָ�룬��������ͨ��ʹ�õ����ģ�帽�����������Ҫ���ģ���������ΪNULL��
        //      ���ģ�帽����depth stencil attachment����һ��������Ⱥ�ģ����Եĸ�����
        //      ͨ�����ڴ洢���ֵ��ģ��ֵ�����ģ�帽��ֻ����һ������Ӧһ�����ģ�建������depth stencil buffer����
        //      ���ģ�帽��������Ϊ��ɫ����������ʹ�ã�Ҳ������Ϊ����Դ��Ŀ��ʹ�á�
        //  preserveAttachmentCount������ͨ����������ʹ�û��޸ĵĸ�����������
        //  pPreserveAttachments��һ��ָ��uint32_t�����ָ�룬��ʾ��VkRenderPassCreateInfo::pAttachments�ж�Ӧ��������������ʹ�û��޸ĵĸ�����
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;            // ����ֻ�趨����ɫ����

        //  renderpass��һ���ִ���ȾAPI����ĸ�����������Ϊһ����������Ⱦ���̣�����ʹ�õ�������Դ�������Ͳ�����ʽ��
        //      vulkan�б�����renderpass���ܽ�����Ⱦ������ÿ��renderpass������һ�������Ӳ��裨subpass����ÿ��subpass��Ӧһ��ͼ�ι��ߣ�pipeline����
        //      ��������������һ���̴�����֡�������ݡ�
        //      renderpass��Ҫ�Ĵ�����Ϣ��Ҫ�������¼������棺
        //      һ�鸽����attachment����ָ������ɫ����ȡ�ģ��Ȼ������ĸ�ʽ����;��
        //      һ�鸽�����ã�attachment reference����ָ����ÿ��subpassʹ����Щ������Ϊ����������
        //      һ��subpass��ָ����ÿ��subpass�����ͣ�ͼ�λ���㣩���󶨵Ĺ��ߡ������ĸ����ȡ�
        //      һ��subpass������subpass dependency����ָ���˲�ͬsubpass֮�������������ϵ��ͬ����ʽ��
        //  ��Ⱦ�����ʼ��������һ�鸽��������attachment description��������ָ����ÿ�������ĸ�ʽ�������������غʹ洢��������ʼ�����ղ��ֵ���Ϣ��
        //  ��Ⱦ�������������һ�鸽�������ǰ�������Ⱦ�������ͼ�����ݣ�����������ʾ������Ϊ������Ⱦ���̵�����
        //  ��Ⱦ���̺���Ⱦ���ߵĹ�ϵ�ǣ���Ⱦ���̶�������Ⱦ�������ʹ��֡��������е�ͼ�������Ⱦ����Ⱦ���̿��������������п�ʼ�ͽ�������ִ��һϵ��ͼ������5��
        
        // �ṹ��VkRenderPassCreateInfo�����³�Ա����3��
        //  sType���ṹ�����ͣ�����ΪVK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO��
        //  pNext������������ʹ�û���չ�ṹ�幦�ܡ�
        //  flags������������ʹ�á�
        //  attachmentCount����Ⱦ����ʹ�õĸ���������
        //  pAttachments��һ��ָ��VkAttachmentDescription�ṹ�������ָ�룬������Ⱦ����ʹ�õĸ������ԡ�
        //  subpassCount��Ҫ��������ͨ��������
        //  pSubpasses��һ��ָ��VkSubpassDescription�ṹ�������ָ�룬����ÿ����ͨ���Ĳ�����
        //  dependencyCount����ͨ������ڴ�������ϵ��������
        //      subpass������subpass dependency����Vulkan��һ������ָ����ͬ��ͨ����subpass��֮�������������ϵ��ͬ����ʽ�Ľṹ�塣
        //      subpass����������Ӧ�ó�����ȷ�ظ�������������Щ��ͨ����Ҫ�ȴ���֪ͨ������ͨ�����Լ��ں�ʱ����ͼ�񲼾�ת����image layout transition��
        //  pDependencies��һ��ָ��VkSubpassDependency�ṹ�������ָ�룬������ͨ������ڴ�������ϵ��
        //      VkSubpassDependency�ṹ�����������Ϣ��
        //      srcSubpass��ָ����Դ��ͨ������������ʾ�ڸ���ͨ����ɺ���ܿ�ʼĿ����ͨ�������ΪVK_SUBPASS_EXTERNAL����ʾ����Ⱦͨ����ʼǰ�������
        //      dstSubpass��ָ����Ŀ����ͨ������������ʾ�ڸ���ͨ����ʼǰ����ȴ�Դ��ͨ����ɡ����ΪVK_SUBPASS_EXTERNAL����ʾ����Ⱦͨ����ʼǰ�������
        //      srcStageMask��ָ����Դ��ͨ����Ҫ�ȴ��Ĺ��߽׶����룬��ʾ�ڸý׶���ɺ���ܿ�ʼĿ����ͨ����
        //      dstStageMask��ָ����Ŀ����ͨ����Ҫ�ȴ��Ĺ��߽׶����룬��ʾ�ڸý׶ο�ʼǰ����ȴ�Դ��ͨ����ɡ�
        //          VkPipelineStageFlagBits��һ��ö�����ͣ�����ָ�����߽׶Σ�pipeline stage������������ֵ:
        //          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT����ʾ���߿�ʼʱ�Ľ׶Σ�û���κβ�����
        //          VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT����ʾִ�м�ӻ��������vkCmdDrawIndirect��ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_VERTEX_INPUT_BIT����ʾ��������������ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT����ʾִ�ж�����ɫ��ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT����ʾִ������ϸ�ֿ�����ɫ��ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT����ʾִ������ϸ��������ɫ��ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT����ʾִ�м�����ɫ��ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT����ʾִ��ƬԪ��ɫ��ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT����ʾִ������ƬԪ���ԣ�����Ȳ��ԣ�ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT����ʾִ������ƬԪ���ԣ���ģ����ԣ�ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT����ʾд����ɫ����ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT����ʾִ�м�����ɫ��ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_TRANSFER_BIT����ʾִ�д���������翽���������ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT����ʾ���߽���ʱ�Ľ׶Σ�û���κβ�����
        //          VK_PIPELINE_STAGE_HOST_BIT����ʾ���������ڴ�����ʱ�Ľ׶Ρ�
        //          VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT����ʾ����ͼ����صĽ׶Σ��൱�ڰ�����ͼ�ι�����ص�λ������Ϊ1��
        //          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT����ʾ����������صĽ׶Σ��൱�ڰ�����λ������Ϊ1��
        //      srcAccessMask��ָ����Դ��ͨ����Ҫ�ȴ��ķ����������룬��ʾ�ڸ����͵ķ�����ɺ���ܿ�ʼĿ����ͨ����
        //      dstAccessMask��ָ����Ŀ����ͨ����Ҫ�ȴ��ķ����������룬��ʾ�ڸ����͵ķ��ʿ�ʼǰ����ȴ�Դ��ͨ����ɡ�
        //          VkAccessFlagBits��һ��ö�����ͣ�����ָ���ڴ�������ͣ�memory access type������������ֵ:
        //          VK_ACCESS_INDIRECT_COMMAND_READ_BIT����ʾ��ȡ��ӻ�����������ʱ�ķ������͡�
        //          VK_ACCESS_INDEX_READ_BIT����ʾ��ȡ������������ʱ�ķ������͡�
        //          VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT����ʾ��ȡ������������ʱ�ķ������͡�
        //          VK_ACCESS_UNIFORM_READ_BIT����ʾ��ȡͳһ��������ʱ�ķ������͡�
        //          VK_ACCESS_INPUT_ATTACHMENT_READ_BIT����ʾ��ȡ���븽������ʱ�ķ������͡�
        //          VK_ACCESS_SHADER_READ_BIT����ʾ��ɫ����ȡ����ʱ�ķ������͡�
        //          VK_ACCESS_SHADER_WRITE_BIT����ʾ��ɫ��д������ʱ�ķ������͡�
        //          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT����ʾ��ȡ��ɫ��������ʱ�ķ������͡�
        //          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT����ʾд����ɫ��������ʱ�ķ������͡�
        //          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT����ʾ��ȡ���ģ�帽������ʱ�ķ������͡�
        //          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT����ʾд�����ģ�帽������ʱ�ķ������͡�
        //          VK_ACCESS_TRANSFER_READ_BIT����ʾ���������ȡ����ʱ�ķ������͡�
        //          VK_ACCESS_TRANSFER_WRITE_BIT����ʾ�������д������ʱ�ķ������͡�
        //          VK_ACCESS_HOST_READ_BIT����ʾ������ȡ����ʱ�ķ������͡�
        //          VK_ACCESS_HOST_WRITE_BIT����ʾ����д������ʱ�ķ������͡�
        //          VK_ACCESS_MEMORY_READ_BIT����ʾ�κζ�ȡ�ڴ����ʱ�ķ������͡�
        //          VK_ACCESS_MEMORY_WRITE_BIT����ʾ�κ�д���ڴ����ʱ�ķ������͡�
        //      dependencyFlags��ָ����һЩ��ѡ��������־������VK_DEPENDENCY_BY_REGION_BIT��ʾ����ֻ��������ͬ�����ڡ�
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;   // ���ýṹ������ͣ���ʾ����һ����Ⱦͨ��������Ϣ
        renderPassInfo.attachmentCount = 1;                                 // ������Ⱦͨ��ʹ�õĸ�����attachment��������������Ϊ1
        renderPassInfo.pAttachments = &colorAttachment;                     // ������Ⱦͨ��ʹ�õĸ���������
        renderPassInfo.subpassCount = 1;                                    // ������Ⱦͨ����������ͨ����subpass��������������Ϊ1
        renderPassInfo.pSubpasses = &subpass;                               // ������Ⱦͨ����������ͨ��������

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