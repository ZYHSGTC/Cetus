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
        createGraphicsPipeline();
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

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("examples/vert.spv");
        auto fragShaderCode = readFile("examples/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
        
        // VkPipelineShaderStageCreateInfo�ĸ���Ҫȷ���Ĳ���
        // sType����ΪVK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
        // stage��ָ����ɫ���׶ε����ͣ���ɫ���׶���ָ����ɫ��ģ������һ����ɫ����
        //      ���������ֵ�������оٳ��õģ�
        //      VK_SHADER_STAGE_VERTEX_BIT����ʾ����׶Σ����ڴ��������Ժ��������λ�á�
        //      VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT����ʾ����ϸ�ֿ��ƽ׶Σ����ڿ�������ϸ�ֵĳ̶Ⱥͷ�3��
        //      VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT����ʾ����ϸ�������׶Σ����ڸ�������ϸ�ֿ��ƽ׶ε���������µĶ��㡣
        //      VK_SHADER_STAGE_GEOMETRY_BIT����ʾ���ν׶Σ����ڶ������ͼԪ���б任�������µ�ͼԪ��
        //      VK_SHADER_STAGE_FRAGMENT_BIT����ʾƬԪ�׶Σ����ڼ���ÿ��ƬԪ����ɫ�����ֵ��
        //      VK_SHADER_STAGE_COMPUTE_BIT����ʾ����׶Σ�����ִ��ͨ�õĲ��м�������
        //      VK_SHADER_STAGE_ALL_GRAPHICS����ʾ���е�ͼ�ν׶Σ�����������׶Σ������������ö��ֵ�����1��
        //      VK_SHADER_STAGE_ALL����ʾ����֧�ֵ���ɫ���׶Σ�������չ����Ķ���׶Σ���������ö��ֵ�����1��
        // module��ָ��һ��VkShaderModule���󣬱�ʾ������ɫ�������ģ�顣
        // pName��ָ��һ���ַ�������ʾ��ɫ�������е���ڵ㺯�������ƣ�ͨ��Ϊ"main"�������ж����ڣ�����һ���ַ�����ָ��
        // 
        
        // ����һ��VkPipelineShaderStageCreateInfo�ṹ�������������������ɫ���׶ε���Ϣ
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        // ���ýṹ������ͣ�����ΪVK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        // ������ɫ���׶ε����ͣ�ΪVK_SHADER_STAGE_VERTEX_BIT����ʾ������ɫ��
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        // ������ɫ��ģ��Ķ���ΪvertShaderModule����ʾ����������ɫ�������ģ��
        vertShaderStageInfo.module = vertShaderModule;
        // ������ɫ�������е���ڵ㺯�������ƣ�Ϊ"main"
        vertShaderStageInfo.pName = "main";

        // ����һ��VkPipelineShaderStageCreateInfo�ṹ�������������ƬԪ��ɫ���׶ε���Ϣ
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        // ���ýṹ������ͣ�����ΪVK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        // ������ɫ���׶ε����ͣ�ΪVK_SHADER_STAGE_FRAGMENT_BIT����ʾƬԪ��ɫ��
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        // ������ɫ��ģ��Ķ���ΪfragShaderModule����ʾ����ƬԪ��ɫ�������ģ��
        fragShaderStageInfo.module = fragShaderModule;
        // ������ɫ�������е���ڵ㺯�������ƣ�Ϊ"main"
        fragShaderStageInfo.pName = "main";


        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

// ����һ��������������һ���ַ����������ã���ʾSPIR-V�ֽ���
VkShaderModule createShaderModule(const std::vector<char>& code) {
        // ����һ��VkShaderModuleCreateInfo�ṹ���������������ɫ��ģ��Ĵ�����Ϣ
        VkShaderModuleCreateInfo createInfo{};

        // ���ýṹ������ͣ�����ΪVK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        // �����ֽ���Ĵ�С�����ַ������Ĵ�С
        createInfo.codeSize = code.size();
        // �����ֽ����ָ�룬����VulkanҪ���ֽ�����uint32_t���͵�����洢��������Ҫʹ��reinterpret_cast���ַ�ָ��ת��Ϊuint32_tָ��
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        // reinterpret_cast���ַ�ָ��ת��Ϊuint32_tָ��Ĺ�����ͨ����ָ���λģʽ�������½��ͣ������ı���ֵ������ζ��ת�����ָ����Ȼָ��ԭ�����ڴ��ַ�����ǰ���uint32_t���������ʸõ�ַ��
        //      ����char����ռ��һ���ֽڣ���uint32_t����ռ���ĸ��ֽڣ�������ת����ÿ��uint32_tָ���ָ��ԭ�����ĸ�char���͵�ֵ��
        //      ���磬���ԭ�����ַ�ָ��ָ����һ������"abcd"���ַ�������ôת�����uint32_tָ���ָ��һ����������ֵΪ0x64636261��ʮ�����Ʊ�ʾ����
        //      ��ת�������У�����Ҫ���κζ���Ĳ�������Ϊreinterpret_castֻ�Ƕ�ָ���λģʽ�������½��ͣ������ı���ֵ��
        // reinterpret_cast���ڽ��и��ֲ�ͬ���͵�ָ��֮�䡢��ͬ���͵�����֮���Լ�ָ���������ָ�����������֮���ת����
        //      ת��ʱ��ִ�е���������ظ��ƵĲ���������ת���ṩ�˺�ǿ������ԣ���ת���İ�ȫ��ֻ���ɳ���Ա�Լ���֤�����ܵ���δ������Ϊ������ʱ����
        // static_cast���ڽ��бȽϡ���Ȼ���͵ͷ��յ�ת���������ͺ͸����͡��ַ���֮��Ļ���ת����
        //      ��Ҳ�������ڵ�����������ص�ǿ������ת�����������ʽ���캯�����������������ڼ̳в���н���ָ������õ����ϻ�����ת����������ͨ����̳н�������ת����
        //      �������������ʱ��飬�������ת����һ����������ͣ��ᵼ��δ������Ϊ��
        // dynamic_cast�����ڼ̳в���н���ָ������õ����ϻ�����ת����Ҫ��ת��������ж�̬�ԣ������麯������
        //      �����������ʱ��飬�������ת����һ����������ͣ��᷵�ؿ�ָ����׳��쳣��������ͨ����̳н�������ת��������԰�ȫ����Ч�ʽϵ͡�
        // const_cast����ȥ�������const��volatile���ԡ������ܸı������ĳ����ԣ�ֻ�ܸı�ָ������öԶ���ĳ�������ͼ������޸���һ�������ǳ����Ķ��󣬻ᵼ��δ������Ϊ��
        // bit_cast��C++20���룬�����Խ�һ������λ���Ƶ���һ�������У�������Ŀ�����ͽ��н��͡���Ҫ��Դ�����Ŀ������С��ͬ���������໥�ɸ��Ƶġ�
        
        VkShaderModule shaderModule;
        // VkShaderModule����һ�����ڴ�����ɫ��ģ��Ķ�����������SPIR-V��ʽ����ɫ�������һ��������ڵ�
        // �������Ͼ���һ��������ɫ������Ķ��󣬰��������ɫ����ڵ�ģ�飻
        // ��ڵ���ָGLSL�ļ��е�һ������ĺ�����������ɫ���������ʼ�㣬���ڶ�����ɫ���Ĺ��ܺ������
        // �ڶ�����ɫ����ƬԪ��ɫ���У���ڵ㺯�������Ʊ�����main�����ڼ�����ɫ���У���ڵ㺯�������ƿ����Զ��塣
        // ��ڵ㺯�������в�����Ҳ�����з���ֵ
        
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            // ����vkCreateShaderModule������ʹ��createInfo���豸����device������ɫ��ģ�飬��������洢��shaderModule��
            throw std::runtime_error("failed to create shader module!");
        }

        // ������ɫ��ģ�����
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


    static std::vector<char> readFile(const std::string& filename) {    // ����һ����̬�������ļ���ȫ���ַ���������spv�ļ�·��
        std::ifstream file(filename, std::ios::ate | std::ios::binary); // ����һ�������ļ��������Զ�����ģʽ���ļ��������ļ�λ��ָ���ƶ����ļ�ĩβ
        // std::ios::in����ֻ��ģʽ���ļ����������������
        // std::ios::out����ֻдģʽ���ļ����������������
        // std::ios::app����׷��ģʽ���ļ��������ݻᱻд�뵽�ļ�ĩβ��
        // std::ios::trunc������ļ��Ѵ��ڣ�������ļ����ݣ�Ȼ����д��ģʽ���ļ���
        // std::ios::hex����ʮ������ģʽ���������
        // std::ios::dec����ʮ����ģʽ���������
        // std::ios::oct���԰˽���ģʽ���������
        // std::ios::scientific���Կ�ѧ�����������������
        // std::ios::fixed���Թ̶�С���������������
        
        if (!file.is_open()) {                                          // ����ļ���ʧ�ܣ��׳�һ������ʱ�����쳣
            throw std::runtime_error("failed to open file!");
        }
        
        size_t fileSize = (size_t)file.tellg(); // ��ȡ��ǰ�ļ�λ��ָ���ֵ�����ļ��Ĵ�С���ֽ�����
        std::vector<char> buffer(fileSize);     // ����һ���ַ���������СΪ�ļ���С
        // �ļ�ָ���ǰ����ֽ����ƶ��ģ�ÿ���ֽ�ռ��һ���ֽڵĿռ䣬�����ļ�ָ���ֵ�͵�������ָ����ֽ����ļ��е�ƫ������
        // ��ˣ�����ļ�ֻ��һ���ֽڣ���ô�ļ�ͷ��λ��ָ����0����ʾ�ļ�����ʼλ�ã����ļ�ĩβ��λ��ָ����1����ʾ���ļ�ͷ��ʼƫ����һ���ֽڵ�λ�á�
        
        
        file.seekg(0);                          // ���ļ�λ��ָ�������ƶ����ļ���ͷ
        file.read(buffer.data(), fileSize);     // ���ļ���ͷ�洢ȫ���ַ����洢���ַ��������ַ�������������
        file.close();                           // �ر��ļ�������

        return buffer;                          // �����ַ�����
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