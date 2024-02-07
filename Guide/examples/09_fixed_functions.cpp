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

    VkPipelineLayout pipelineLayout;
    //

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
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

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

        // VkPipelineVertexInputStateCreateInfo�ṹ������������ͼ�ι����еĶ�������״̬����Ϣ�������������ֶ�1��
        // sType������ΪVK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO����ʾ�ṹ������͡�
        // pNext������ָ��һ����չ�ṹ�壬�����ṩ�������Ϣ������ΪNULL��
        // flags�������ֶΣ�����Ϊ0��
        // vertexBindingDescriptionCount��ָ���������������������pVertexBindingDescriptions����ĳ��ȡ�
        // pVertexBindingDescriptions��ָ��һ��VkVertexInputBindingDescription�ṹ�����飬��������ÿ�����㻺�����İ󶨺š��������������ʡ�
        //  VkVertexInputBindingDescription�ṹ��������������������󶨵���Ϣ�������������ָ�Ӷ��㻺�����л�ȡ�������ݵķ�ʽ��
        //      ÿ�����㻺�����İ󶨺���ָʹ��binding��Ա��������ʶһ�����㻺��������һ���������ڴ�Ĳ�ͬ���֣��ڴ�������ʱ��Ҫ���Ӧ�Ķ�����ɫ��ƥ��
        //      һ�����߿����ж�������������ÿ��������Ӧһ�����㻺��������һ���������ڴ�Ĳ�ͬ���֡�
        //      ����ṹ��������³�Ա��
        //      binding������ṹ�������İ󶨺ţ������ڶ�����ɫ�������ö������ݡ�
        //      stride�����㻺�����Ĳ�����ָʹ��stride��Ա������ָ��������������Ԫ��(�綥��)֮����ֽڿ�ࡣ
        //      inputRate��һ��VkVertexInputRateö��ֵ��ָ���������Եĵ�ַ�Ǹ��ݶ�����������ʵ������������ġ�
        //          �����������ܵ�ֵ��
        //          VK_VERTEX_INPUT_RATE_VERTEX����ʾ�������Եĵ�ַ�Ǹ��ݶ�������������ģ���ÿ�����㶼���Լ�������ֵ��
        //          VK_VERTEX_INPUT_RATE_INSTANCE����ʾ�������Եĵ�ַ�Ǹ���ʵ������������ģ���ÿ��ʵ�������Լ�������ֵ��
        // vertexAttributeDescriptionCount��ָ������������������������pVertexAttributeDescriptions����ĳ��ȡ�
        // pVertexAttributeDescriptions��ָ��һ��VkVertexInputAttributeDescription�ṹ������;
        // VkVertexInputAttributeDescription�ṹ�����������������������Ե���Ϣ����������������ָ�Ӷ��㻺�����л�ȡ���ĵ����������λ�ã���ɫ��6������ṹ��������³�Ա��
        //      location����������ڶ�����ɫ���ж�Ӧ��location��š�layout(position=0) in vec3 aPos
        //      binding��������Դ��ĸ��������л�ȡ���ݡ�
        //      format������������ݵĴ�С�����ͣ�ʹ��VkFormatö��ֵ��ʾ��
        //      offset�������������ڰ�������Ԫ����ʼλ�õ��ֽ�ƫ������
        // Vulkan�����VkVertexInputAttributeDescription�ṹ�������е�offset��Ա����������ÿ���������Ե�ƫ������
        //      ���ƫ����������ڰ�������Ԫ����ʼλ�õ��ֽ�ƫ������
        //      ���磬���һ�����㻺���������������������ԣ�λ�ú���ɫ��ÿ������ռ��12���ֽڣ���ô��һ�����Ե�ƫ������0���ڶ������Ե�ƫ������12��
        // Vulkan�����VkVertexInputAttributeDescription�ṹ�������е�location��Ա������ȷ��ÿ���������Ե�location��ţ�
        //      �������������ڶ�����ɫ�������ö������ݵ�2��
        //      ���磬���һ�����㻺���������������������ԣ�λ�ú���ɫ����ô����ָ����һ�����Ե�location���Ϊ0���ڶ������Ե�location���Ϊ1��
        // ͬһ�󶨵��VkVertexInputAttributeDescription�������ƫ����Ӧ�õ��ڻ�С����һ�󶨵���������VkVertexInputBindingDescription���stride��
        //      ������Ϊstride��ʾ������������Ԫ��֮����ֽڿ�࣬��offset��ʾÿ�����������Ԫ����ʼλ�õ��ֽ�ƫ������
        //      �����ƫ��������stride����ô�ͻᵼ�������ص�������
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;      //������û�п��ٶ��㻺�������Բ���Ҫ���������
        vertexInputInfo.vertexAttributeDescriptionCount = 0;    //������û�п��ٶ��㻺�������Բ���Ҫ������������


        // VkPipelineInputAssemblyStateCreateInfo�ṹ��������ָ���´����Ĺ�������װ��״̬�Ĳ���1�����������³�Ա2��
        //  sType������ΪVK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
        //  pNext��NULL����һ��ָ����չ����ṹ���ָ�롣
        //  flags������δ��ʹ�õı�־λ��
        //  topology��һ��VkPrimitiveTopologyֵ��������ͼԪ���ˣ���������ܵġ�
        //  ��������ö��ֵ4��
        //      VK_PRIMITIVE_TOPOLOGY_POINT_LIST����ʾһϵ�ж����ĵ�ͼԪ��
        //      VK_PRIMITIVE_TOPOLOGY_LINE_LIST����ʾһϵ�ж�������ͼԪ��
        //      VK_PRIMITIVE_TOPOLOGY_LINE_STRIP����ʾһϵ�����ӵ���ͼԪ���������߹���һ�����㡣
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST����ʾһϵ�ж�����������ͼԪ��
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP����ʾһϵ�����ӵ�������ͼԪ�������������ι���һ���ߡ�
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN����ʾһϵ�����ӵ�������ͼԪ�����е������ι���һ���������㡣
        //      VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY����ʾһϵ�ж�������ͼԪ�������ڽ���Ϣ��
        //      VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY����ʾһϵ�����ӵ���ͼԪ�������ڽ���Ϣ��������ͼԪ�����������㡣
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY����ʾһϵ�ж�����������ͼԪ�������ڽ���Ϣ��
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY����ʾһϵ�����ӵ�������ͼԪ�������ڽ���Ϣ�������������ι���һ���ߡ�
        //      VK_PRIMITIVE_TOPOLOGY_PATCH_LIST����ʾһϵ�ж����Ĳ���ͼԪ��
        //  primitiveRestartEnable�������Ƿ�ʹ��һ������Ķ�������ֵ������ͼԪ��װ�䡣
        //      ���ѡ��ֻ�������������ƣ�vkCmdDrawIndexed��vkCmdDrawMultiIndexedEXT��vkCmdDrawIndexedIndirect����
        // ͼԪװ��ʽ��Ⱦ������һ�̶ֹ����ܵ���Ⱦ��ʽ������һ��Ԥ����ĵ�·���㷨�����ͼ�δ���ĸ����׶Ρ�
        // �����ֹ����У�����Աֻ��ͨ������һЩ������������ȾЧ�����������Զ�������׶εĴ����߼���
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // �ӿ���ü����ο��Կ���vulkan���ָ�ϡ�ͼƬ���
        // �����ӿ���ָ����Ⱦ�����У���������ͼ������Ļ�ϵ���ʾ��Χ��λ��
        // �ӿ��൱�ڴ����ڵ�����ʾ���ڣ��ü��Ƕ�֡���嵽�ӿڽ��вü�
        // VkPipelineViewportStateCreateInfo�ṹ��������ָ���´����Ĺ����ӿ�״̬�Ĳ���1�����������³�Ա2��
        //  sType������ΪVK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
        //  pNext��NULL����һ��ָ����չ����ṹ���ָ�롣
        //  flags������δ��ʹ�õı�־λ��
        //  viewportCount��ʹ�õ��ӿڵ�������
        //  pViewports��һ��ָ��VkViewport�ṹ�������ָ�룬�������ӿڱ任��
        //      ����ӿ�״̬�Ƕ�̬�ģ�Ҳ�����ӿ��ڻ�������֮ǰ���ã������Ա�ᱻ���ԡ�
        //      VkViewport�ṹ�����������ӿ��������������³�Ա��
        //          x, y���ӿ����Ͻǵ����ꡣ
        //          width, height���ӿڵĿ�Ⱥ͸߶ȡ�
        //          minDepth, maxDepth���ӿڵ���ȷ�Χ��framebuffer����������ʹ�ù̶���򸡵�����ʾ�������� / ģ�帽���и�������ȷ���������ʹ�ø�������ʾ��
        //  scissorCount���ü����ε�������������ӿ�������ͬ��
        //  pScissors��һ��ָ��VkRect2D�ṹ�������ָ�룬�����˶�Ӧ�ӿڵĲü����η�Χ������ü�״̬�Ƕ�̬�ģ������Ա�ᱻ���ԡ�
        //  VkRect2D�ṹ����������һ����ά������5�����������³�Ա��
        //          offset��һ��VkOffset2D�ṹ�壬ָ������ƫ������
        //          extent��һ��VkExtent2D�ṹ�壬ָ�����η�Χ��
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        //VkPipelineRasterizationStateCreateInfo�ṹ��������ָ���´����Ĺ��߹�դ��״̬�Ĳ��������������³�Ա��
        // sType������ΪVK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
        // pNext��NULL����һ��ָ����չ����ṹ���ָ�롣
        // flags������δ��ʹ�õı�־λ��
        // depthClampEnable�������Ƿ�Ƭ�ε����ֵ��������Ȳ��Է�Χ�ڡ�
        //      depthClampEnableΪ��Ļ�����ô���ֵ����[0,1]��Χ��Ƭ�ν��������������Ǳ��ü�
        // rasterizerDiscardEnable�������Ƿ��ڹ�դ���׶�֮ǰ����ͼԪ��
        //      ��դ���׶�֮ǰ����ͼԪ��ζ����ͼԪ��ת��Ϊ����֮ǰ������һЩ�����ж��Ƿ���Ҫ�������ǡ�
        // polygonMode��polygon�Ƕ���ε���˼��ȷ���������Ⱦģʽ�����������ģʽ���߿�ģʽ���ģʽ��
        //      �������¼������ܵ�ֵ��
        //      VK_POLYGON_MODE_FILL������ε��ڲ��ͱ�Ե���ᱻ�����ɫ��
        //      VK_POLYGON_MODE_LINE�������ֻ����Ʊ�Ե���γ��߿�ģʽ��
        //          �ڹ�դ�������У�ֻ��λ�ڶ���α��ϵ�ƬԪ/���ػᱻ�����ɫ����������ڲ���ƬԪ/���ػᱻ���ԡ�
        //      VK_POLYGON_MODE_POINT�������ֻ����ƶ��㣬�γɵ���ģʽ��
        //          �ڹ�դ�������У�ֻ��λ�ڶ���ζ����ϵ�ƬԪ/���ػᱻ�����ɫ��������λ�õ�ƬԪ/���ػᱻ���ԡ�
        //      VK_POLYGON_MODE_FILL_RECTANGLE_NV������λ�ʹ���޸ĺ�Ĺ�դ������ֻҪһ���������������ε�������Χ���ڣ�����Ϊ���ڶ�����ڲ�������һ����NVidia�ṩ����չģʽ��
        // cullMode������ͼԪ�޳��������γ��򣬿��������桢�����˫�档
        //      �������¼������ܵ�ֵ��
        //      VK_CULL_MODE_NONE����ʾ������ͼԪ�޳������������е�ͼԪ��
        //      VK_CULL_MODE_FRONT_BIT����ʾ�޳����泯���ͼԪ����ֻ�������泯���ͼԪ��
        //      VK_CULL_MODE_BACK_BIT����ʾ�޳����泯���ͼԪ����ֻ�������泯���ͼԪ��
        //      VK_CULL_MODE_FRONT_AND_BACK����ʾ�޳����е�ͼԪ�����������κ�ͼԪ��
        // frontFace��ָ�����������εķ��򣬿�����˳ʱ�����ʱ�롣
        //      ���������������ܵ�ֵ��
        //      VK_FRONT_FACE_COUNTER_CLOCKWISE����ʾ��ʱ�뷽���������Ϊ���泯��
        //      VK_FRONT_FACE_CLOCKWISE����ʾ˳ʱ�뷽���������Ϊ���泯��
        // depthBiasEnable��Bias��ƫ�Ƶ���˼�������Ƿ��Ƭ�ε����ֵ����ƫ�ơ�
        //      ���ֵƫ����һ�����ڽ����Ȼ������е���ȳ�ͻ����ļ�����
        //      ��ȳ�ͻ��ָ����������������ͬһλ�û�ǳ��ӽ���λ����Ⱦʱ��
        //      ������Ȼ������ľ������ޣ�����������������˸��˺�ѵ�����
        //      ���ֵƫ�Ƶ�ԭ���Ƕ���������ֵ����һ���̶ȵ�ƫ�ƣ�
        //      ʹ�ò�ͬ����֮����һ������Ȳ��죬�Ӷ�������ȳ�ͻ
        // depthBiasConstantFactor��һ���������ӣ�����ÿ��Ƭ����ӵĳ������ƫ������
        // depthBiasClamp��һ����󣨻���С�����ƫ��������������Ƭ�ε����ƫ������
        // depthBiasSlopeFactor��һ���������ӡ������Ƭ�ε�б����ˣ��õ�һ�����ƫ����
        //      Ƭ�ε�б����ָƬ������ƽ�����ƽ��ļнǵ�����ֵ��
        //      �����ƫ�Ƶ�ʱ��ͨ����ʹ��depthBiasConstantFactor��depthBiasSlopeFactor��������������ƫ�����Ĵ�С��
        //      ������������ֱ��ÿ��Ƭ�Σ�fragment�������ֵ��ӣ��õ����յ����ƫ����
        // lineWidth����դ���߶εĿ�ȡ�����һ��ʱ�����Ļ�ϵ����ص�һ����;
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.lineWidth = 1.0f;

        // ���ز�����һ��ʹ�ø�����ɫ����Ⱥ�ģ����Ϣ����������ͼԪ���㣬ֱ�ߣ�����Σ�λͼ��ͼ�񣩽��з�����(�����)����ļ�����
        //      ��ͨ����һ�������Ͻ��ж�β�����μ��㲢���ջ��� (Resolve to single-sample)����ʹ���Ƶ�ͼ���Ե����ƽ��
        //      ��դ���׶�һ����Ҫ�Ĺ������ǲ�ֵ����(Interpolation�������Զ��ز������õ�����׶���Ҫ�Ƕ��ز�ֵ��
        // ���ز����Ľ׶η�����ͼ����Ⱦ���ߵ����²��֣�
        //      ��ȡ���ò�����������׶����ڴ���ͼ�ι���֮ǰ���еġ�
        //          ��ѯ�����豸�����ԣ���ȡ��ɫ����Ȼ�����֧�ֵ�����������ȡ��Сֵ��Ϊ���ò�������
        //      ������ȾĿ�꣺����׶����ڴ���ͼ�ι���֮ǰ���еġ�
        //          ����һ�����ز�����������Ϊ���������ڴ洢ÿ�����صĶ���������ݡ�
        //          �����������Ҫָ������������ʽ����;�Ȳ������������ڴ�ʹ���ͼ����ͼ��
        //          ���ز����������뽻����ͼ����֡�������ĳߴ�͸�ʽ��ͬ������ÿ�����ؿ��Դ洢���������
        //          ���ز������������Կ������֡���壬ÿ�㻺�崢��һ��������
        //          ����Ⱦ�����󣬶��ز����������ᱻ����Ϊ������ͼ��Ȼ����ֵ���Ļ�ϡ�
        //      ����¸���������׶����ڴ���ͼ�ι���ʱ���еġ�
        //          ����Ⱦͨ�������һ�����ز����������������ã����ڴ洢���ز��������������ݡ�
        //          ͬʱ���޸���ɫ��������;Ϊת��Դ�������һ��������ͨ�������ڽ����ز�������������Ϊ֡��������
        //      ��������������׶����ڴ���ͼ�ι���ʱ���еġ�
        //          �ڹ��ߴ�����Ϣ��ָ�����ز���״̬�����������������֡����ǲ��ԵȲ�������Щ��������Ӱ����ز��������������ܡ�
        //      ���ز������ԣ�����׶����ڹ�դ��֮����еġ�
        //          ��Ҫ�������ֺ͸��ǲ�����ȷ����Щ����������������
        //          ���ֲ��ԣ�������һ��λ���룬����ָ����Щλƽ��(�ǲ�֡����/���ز�������)������ԣ�ÿ��λƽ���Ӧһ��������
        //          ���ֲ������ɳ���Ա�����ģ��������豸�����ġ�����Ա�����ڴ���ͼ�ι���ʱ��������pSampleMask��
        //          ���ֲ��Ե�Ŀ���Ǹ������ֵ�������������Щ����������������
        //              λ������һ���������������ڱ�ʾһ�鿪�ص�״̬��ÿ��������λ��Ӧһ�����أ�1��ʾ����0��ʾ�ء�
        //              0x0F��ʾ00001111����ʾ���ĸ����ض��ǿ��ģ�
        //              ���������Ϊ0x0F����ô��ʾ4��������������ԣ�
        //              0x03��ʾ00000011����ʾ�����������ǿ��ģ�����Ķ��ǹص�
        //              ���������Ϊ0x03����ô��ʾֻ��ǰ��������������ԣ�������������������
        //              ���������Ϊ0x09��Ҳ����00001001��������ֻ�е�һ�����������ᱻ�����������Ķ��ᱻ������
        //          ���ǲ��ԣ����ǲ�����һ���Ż�����������������Щ����ȫ���ǵ����ء�
        //          ���ǲ��Ե�Ŀ���Ǹ���Ƭ�ε����ֵ����Ȼ������е�ֵ���ж�һ�������Ƿ���ȫ���ǡ�
        //              ���һ�����ر���ȫ���ǣ���ô������������������Ҫ������ɫ�����Խ�ʡ������Դ��
        //              ���һ������û�б���ȫ���ǣ���ô���Ĳ��ֻ�ȫ��������Ҫ������ɫ��
        //      ���ز�������������׶�����ƬԪ��ɫ��֮����еġ�
        //          ���ز�������������ɫֵ��ÿ����Ч�Ĳ����㶼��һ����ɫֵ����Щ��ɫֵ�ᱻд����ز���������
        //          ��Ҫ�����ز����������е�ÿ�����صĶ���������ݺϲ�Ϊһ����һ����ɫֵ����д��֡�������С�
        //          ��������ͨ���Ƕ�ÿ��������������Ч�Ĳ�������ɫֵ����ƽ�����Ȩƽ�����õ����������յ���ɫֵ��
        //          ���ز��������ķ����в�ͬ��ѡ������ƽ������average������󷨣�max������С����min���ȡ�
        //  �ṹ��VkPipelineMultisampleStateCreateInfo������ָ���´����Ĺ��߶��ز���״̬�Ĳ��������������³�Ա��
        //  sType������ΪVK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
        //  pNext��NULL����һ��ָ����չ����ṹ���ָ�롣
        //  flags������δ��ʹ�õı�־λ��
        //  rasterizationSamples��ָ����դ���׶�ʹ�õĲ����������ֵ�����˶��ز��������������ܡ�
        //      ���ز�����������ģ����Ǹ���һ���Ĺ���ֲ��������ڲ�����ͬ�Ĳ�������Ӧ��ͬ�Ĳ���ģʽ��
        //      4������ͨ��ʹ��������������ĸ�������Ϊ�����㣬8������ͨ��ʹ��������������ĸ�������ĸ��е���Ϊ������
        //      VkSampleCountFlagBits��һ��ö�����ͣ����ڱ�ʾͼ���֡��������ÿ�����ص�����������������ö��ֵ�ͺ��壺
        //      VK_SAMPLE_COUNT_1_BIT��ָ��һ��������һ������
        //      VK_SAMPLE_COUNT_2_BIT��ָ��һ����������������
        //      VK_SAMPLE_COUNT_4_BIT��ָ��һ���������ĸ�����
        //      VK_SAMPLE_COUNT_8_BIT��ָ��һ�������а˸�����
        //      VK_SAMPLE_COUNT_16_BIT��ָ��һ��������ʮ��������
        //      VK_SAMPLE_COUNT_32_BIT��ָ��һ����������ʮ��������
        //      VK_SAMPLE_COUNT_64_BIT��ָ��һ����������ʮ�ĸ�����
        //  sampleShadingEnable��һ��VkBool32ֵ�������Ƿ�����������ɫ��sample shading����
        //  minSampleShading��һ����������ָ����С��������ɫ������
        //      ���sampleShadingEnableΪVK_TRUE����ôÿ��Ƭ����������minSampleShading �� rasterizationSamples��������ᱻ������ɫ��
        //      ��ָ����ÿ�������ڲ�������ɫ����С�����ʡ��������ֵΪ1.0����ôÿ�������㶼�ᱻ������ɫ��
        //      �������ֵΪ0.0����ôֻ��һ��������ᱻ��ɫ��Ȼ���Ƹ����������㡣
        //      �������ֵ��0.0��1.0֮�䣬��ôֻ��һ���ֲ�����ᱻ��ɫ��������Щ��������ʵ�־�����
        // pSampleMask��һ��ָ��VkSampleMaskֵ�����ָ�룬�����ڲ������ֲ��ԣ�sample mask test���й��˵�һЩ�����㡣
        //      ���ֲ��ԻὫƬ����ɫ�������SampleMask���դ���׶����ɵĸ������루coverage mask�������߼������㣬�õ����յĸ������롣
        //      ֻ�����ո���������Ϊ1��λ��Ӧ�Ĳ�����Żᱻд����ɫ��������
        // alphaToCoverageEnable���������û����alpha��������ת����Ҳ���Ǹ���Ƭ����ɫ�������alphaֵ������ÿ�������ĸ����ʡ�
        //      ��������������������ôƬ����ɫ�������alphaֵ�����ԭʼ�ĸ����ʣ��õ��µĸ����ʡ�
        //      ��������ʵ��һ��������͸����ϵ�Ч�������ǲ���Ҫ�������������
        //      ��������һ��������ڱ�ʾһ���������ж��ٸ����������ǵı�����
        //          �����ʵķ�Χ��0��1��0��ʾû���κ����������ǣ�1��ʾ���������������ǡ�
        //          �����ʿ����ɹ�դ���׶μ��������Ҳ������alpha��������ת�����޸�1��
        //      �����ʿ���ʵ������͸����ϵ�Ч��������Ϊ�����Կ���ÿ����������ɫ�����Ȩ�ء�
        //          ���һ���������ж�����������ǣ���ô��Щ��������ɫֵ��������ǵĸ����������м�Ȩƽ�����õ����յ�������ɫ��
        //          �����Ϳ���ģ�����͸�������Ч�����ú��������͸��ǰ������忴����
        // alphaToOneEnable���������û����alpha��һת����Ҳ���ǽ�Ƭ����ɫ�������alphaֵǿ������Ϊ1.0��
        //      ��������������������ôƬ����ɫ�������alphaֵ�ᱻ���ԣ�����Ӱ�츲���ʡ�
        //      �������Ա���һЩ����Ҫ͸���ȵ����屻����Ϊ�ǰ�͸���ġ�
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // VkPipelineColorBlendAttachmentState�ṹ��������������ɫ��ϵ�һ���ṹ�壬�����Զ�ÿ���󶨵�֡������е�������ɫ�������1�������������ֶ�2��
        //  blendEnable��һ������ֵ���������û������ɫ��ϡ�
        //  srcColorBlendFactor��һ��VkBlendFactorö��ֵ������ָ��Դ��ɫ���ӣ�Ҳ������Ƭ�ε���ɫֵ��Ȩ�ء�
        //  dstColorBlendFactor��һ��VkBlendFactorö��ֵ������ָ��Ŀ����ɫ���ӣ�Ҳ����֡�����е���ɫֵ��Ȩ�ء�
        //  colorBlendOp��һ��VkBlendOpö��ֵ������ָ��Դ��ɫ��Ŀ����ɫ�Ļ�ϲ����������Ǽӷ��������������������Сֵ�����ֵ��
        //  srcAlphaBlendFactor��һ��VkBlendFactorö��ֵ������ָ��Դalpha���ӣ�Ҳ������Ƭ�ε�alphaֵ��Ȩ�ء�
        //  dstAlphaBlendFactor��һ��VkBlendFactorö��ֵ������ָ��Ŀ��alpha���ӣ�Ҳ����֡�����е�alphaֵ��Ȩ�ء�
        //  alphaBlendOp��һ��VkBlendOpö��ֵ������ָ��Դalpha��Ŀ��alpha�Ļ�ϲ����������Ǽӷ��������������������Сֵ�����ֵ��
        //  colorWriteMask��һ����λλ���룬����ָ����Щ��ɫ������д��֡���塣������R��G��B��A�������ǵ���ϡ�
        //  
        // VkBlendFactor������ֵ��
        //  VK_BLEND_FACTOR_ZERO����ʾȨ��Ϊ0���൱�ں��Ը�ֵ��
        //  VK_BLEND_FACTOR_ONE����ʾȨ��Ϊ1���൱�ڱ�����ֵ��
        //  VK_BLEND_FACTOR_SRC_COLOR����ʾȨ��ΪԴ��ɫ��Ҳ������Ƭ�ε���ɫ��
        //  VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR����ʾȨ��Ϊ1��ȥԴ��ɫ��Ҳ���Ƿ�ת��Ƭ�ε���ɫ��
        //  VK_BLEND_FACTOR_DST_COLOR����ʾȨ��ΪĿ����ɫ��Ҳ����֡�����е���ɫ��
        //  VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR����ʾȨ��Ϊ1��ȥĿ����ɫ��Ҳ���Ƿ�ת֡�����е���ɫ��
        //  VK_BLEND_FACTOR_SRC_ALPHA����ʾȨ��ΪԴalpha��Ҳ������Ƭ�ε�alphaֵ��
        //  VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA����ʾȨ��Ϊ1��ȥԴalpha��Ҳ���Ƿ�ת��Ƭ�ε�alphaֵ��
        //  VK_BLEND_FACTOR_DST_ALPHA����ʾȨ��ΪĿ��alpha��Ҳ����֡�����е�alphaֵ��
        //  VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA����ʾȨ��Ϊ1��ȥĿ��alpha��Ҳ���Ƿ�ת֡�����е�alphaֵ��
        //  VK_BLEND_FACTOR_CONSTANT_COLOR����ʾȨ��Ϊһ��������ɫ������ͨ��VkPipelineColorBlendStateCreateInfo�ṹ���е�blendConstants�ֶ������á�
        //  VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR����ʾȨ��Ϊ1��ȥһ��������ɫ��
        //  VK_BLEND_FACTOR_CONSTANT_ALPHA����ʾȨ��Ϊһ������alpha������ͨ��VkPipelineColorBlendStateCreateInfo�ṹ���е�blendConstants�ֶ������á�
        //  VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA����ʾȨ��Ϊ1��ȥһ������alpha��
        //  VK_BLEND_FACTOR_SRC_ALPHA_SATURATE����ʾȨ��ΪԴalpha��1��ȥĿ��alpha�н�С��һ���������ʵ��һ��Ԥ��alpha��ϡ�

        // VkBlendOp������ֵ��
        //  VK_BLEND_OP_ADD����ʾ������ֵ��ӣ�������õĻ�ϲ�����
        //  VK_BLEND_OP_SUBTRACT����ʾ��Ŀ����ɫ��Դ��ɫ�м�ȥ��
        //  VK_BLEND_OP_REVERSE_SUBTRACT����ʾ��Դ��ɫ��Ŀ����ɫ�м�ȥ��
        //  VK_BLEND_OP_MIN����ʾȡ����ֵ�н�С��һ����
        //  VK_BLEND_OP_MAX����ʾȡ����ֵ�нϴ��һ����
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        // VkPipelineColorBlendStateCreateInfo�ṹ�����������ù�����ɫ�����ص����õ�һ���ṹ�壬�����Զ�ÿ���󶨵�֡������е�������ɫ�������1�������������ֶ�2��
        //  sType��һ��VkStructureTypeֵ�����ڱ�ʶ����ṹ������͡�
        //  pNext��һ��ָ�룬����ָ��һ����չ����ṹ��������ṹ�壬����ΪNULL��
        //  flags��һ��λ���룬����ָ���������ɫ�����Ϣ��Ŀǰû�ж����κ���Ч��ֵ��
        //  logicOpEnable��һ������ֵ���������û�����߼��������߼�������һ�ֻ���λ�������ɫ��Ϸ�ʽ��
        //  logicOp��һ��VkLogicOpö��ֵ������ѡ��ҪӦ�õ��߼�������ֻ����logicOpEnableΪVK_TRUEʱ��Ч��
        //  attachmentCount��һ������������ָ��pAttachments�����е�VkPipelineColorBlendAttachmentState�ṹ���������������Ⱦͨ���е���ɫ����������ͬ��
        //  pAttachments��һ��ָ�룬����ָ��һ��VkPipelineColorBlendAttachmentState�ṹ�����飬������ÿ����ɫ��������ɫ������á�
        //      pAttachments��һ�����飬����Ϊ��Ⱦͨ���п����ж��֡���帽����ÿ�����������в�ͬ�Ļ�Ϸ�ʽ����������ʵ�ָ����͸��ӵ���ȾЧ����
        //      pAttachments��ÿһ��Ԫ�ض�Ӧһ����ɫ������һ��֡��������ж����ɫ������Ҳ���Ƕ��ͼ����ͼ�����ڴ洢��ͬ����Ⱦ�����
        //      ���磬һ����ɫ�������������洢��������գ���һ����ɫ�������������洢����߹⣬����ٰ����ǻ������
        //  blendConstants��һ��ָ�룬����ָ��һ�������ĸ������������飬�ֱ��ʾR��G��B��A�����Ļ�ϳ��������ǿ�����ΪԴ���ӻ�Ŀ�����ӵ�һ���֡�
        //      ������ΪVkPipelineColorBlendAttachmentState�ó������Ȩ�أ�
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

        // VkDynamicState����ȡ����ֵ֮һ��
        //  VK_DYNAMIC_STATE_VIEWPORT����ʾ�ӿ�״̬�Ƕ�̬�ģ�����ʹ��vkCmdSetViewport�����������ӿڵ�������λ�úͳߴ硣
        //  VK_DYNAMIC_STATE_SCISSOR����ʾ�ü�����״̬�Ƕ�̬�ģ�����ʹ��vkCmdSetScissor���������òü����ε�������λ�úͳߴ硣
        //  VK_DYNAMIC_STATE_LINE_WIDTH����ʾ��դ���߿�״̬�Ƕ�̬�ģ�����ʹ��vkCmdSetLineWidth�����������߿��ֵ��
        //  VK_DYNAMIC_STATE_DEPTH_BIAS����ʾ���ƫ��״̬�Ƕ�̬�ģ�����ʹ��vkCmdSetDepthBias�������������ƫ�Ƶĳ������ӡ�б�����Ӻͽض�ֵ��
        //  VK_DYNAMIC_STATE_BLEND_CONSTANTS����ʾ��ϳ���״̬�Ƕ�̬�ģ�����ʹ��vkCmdSetBlendConstants���������û�ϳ�����R��G��B��A������
        //  VK_DYNAMIC_STATE_DEPTH_BOUNDS����ʾ��ȷ�Χ״̬�Ƕ�̬�ģ�����ʹ��vkCmdSetDepthBounds������������ȷ�Χ����Сֵ�����ֵ��
        //  VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK����ʾģ��Ƚ�����״̬�Ƕ�̬�ģ�����ʹ��vkCmdSetStencilCompareMask����������ģ��Ƚ�������沿��ֵ��
        //  VK_DYNAMIC_STATE_STENCIL_WRITE_MASK����ʾģ��д������״̬�Ƕ�̬�ģ�����ʹ��vkCmdSetStencilWriteMask����������ģ��д��������沿��ֵ��
        //  VK_DYNAMIC_STATE_STENCIL_REFERENCE����ʾģ��ο�ֵ״̬�Ƕ�̬�ģ�����ʹ��vkCmdSetStencilReference����������ģ��ο�ֵ���沿��ֵ��
        // VkPipelineDynamicStateCreateInfo�ṹ�����������ù��߶�̬״̬��ص����õ�һ���ṹ�壬������ָ����Щ����״̬�ǴӶ�̬״̬�����л�ȡ�ģ������Ǵӹ��ߴ�����Ϣ�л�ȡ��1�������������ֶ�2��
        //  sType��һ��VkStructureTypeֵ�����ڱ�ʶ����ṹ������ͣ�������VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO��
        //  pNext��һ��ָ�룬����ָ��һ����չ����ṹ��������ṹ�壬����ΪNULL��
        //  flags��һ��λ���룬����ָ������Ĺ��߶�̬״̬��Ϣ��Ŀǰû�ж����κ���Ч��ֵ������Ϊ0��
        //  dynamicStateCount��һ������������ָ��pDynamicStates�����е�VkDynamicStateֵ��������
        //  pDynamicStates��һ��ָ�룬����ָ��һ��VkDynamicStateֵ���飬ָ����Щ����״̬�Ƕ�̬�ģ����ԴӶ�̬״̬�����л�ȡ��
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // VkPipelineLayoutCreateInfo�ṹ�����������ù��߲�����ص����õ�һ���ṹ�壬������ָ��������ʹ�õ������������ֺ����ͳ�����Χ��
        //  sType��һ��VkStructureTypeֵ�����ڱ�ʶ����ṹ������ͣ�������VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO��
        //  pNext��һ��ָ�룬����ָ��һ����չ����ṹ��������ṹ�壬����ΪNULL��
        //  flags��һ��λ���룬����ָ������Ĺ��߲�����Ϣ��Ŀǰû�ж����κ���Ч��ֵ������Ϊ0��
        //  setLayoutCount��һ������������ָ��pSetLayouts�����е�VkDescriptorSetLayoutֵ��������
        //  pSetLayouts��һ��ָ�룬����ָ��һ��VkDescriptorSetLayout��������飬ָ��������ʹ�õ������������֡�
        //      ÿ�������������ֿ��԰�������������󶨵㣬���ڴ洢��ͬ���ͺ���������Դ��
        //      VkDescriptorSetLayout��������Ҫ����ϢVkDescriptorSetLayoutCreateInfo�ṹ��:
        //      sType��һ��VkStructureTypeֵ�����ڱ�ʶ����ṹ������ͣ�������VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO��
        //      pNext��һ��ָ�룬����ָ��һ����չ����ṹ��������ṹ�壬����ΪNULL��
        //      flags��һ��λ���룬����ָ���������������������Ϣ��Ŀǰû�ж����κ���Ч��ֵ��
        //      bindingCount��һ������������ָ��pBindings�����е�VkDescriptorSetLayoutBinding�ṹ���������
        //      pBindings��һ��ָ�룬����ָ��һ��VkDescriptorSetLayoutBinding�ṹ�����飬������ÿ���������󶨵����Ϣ��
        //          VkDescriptorSetLayoutBinding�ṹ�壺
        //          binding��һ������������ָ������󶨵�ı�ţ�������ɫ����ʹ�õİ󶨺����Ӧ��
        //          descriptorType��һ��VkDescriptorTypeö��ֵ������ָ������󶨵�����������ͣ��绺�塢ͼ�񡢲������ȡ�
        //          descriptorCount��һ������������ָ������󶨵���������������������1�����ʾ����󶨵���һ�����飬��������ɫ����ʹ���±������ʡ�
        //          stageFlags��һ��λ���룬����ָ������󶨵���Ա���Щ��ɫ���׶η��ʣ��綥�㡢Ƭ�Ρ�����ȡ�
        //          pImmutableSamplers��һ��ָ�룬����ָ������󶨵�Ĳ��ɱ���������飬�������󶨵�������������ǲ������������ͼ��������������ʹ������ֶ����̶����������󣬶�����Ҫ�����������и��¡�
        //  pushConstantRangeCount��һ������������ָ��pPushConstantRanges�����е�VkPushConstantRange�ṹ���������
        //  pPushConstantRanges��һ��ָ�룬����ָ��һ��VkPushConstantRange�ṹ�����飬ָ��������ʹ�õ����ͳ�����Χ��
        //      ÿ�����ͳ�����Χ���Զ���һ���ڴ�ռ䣬���ڴ洢һЩ�������ݣ�����������ʱͨ��vkCmdPushConstants���������¡�
        //      VkPushConstantRange�ṹ��:
        //      stageFlags��һ��λ���룬����ָ��������ͳ�����Χ���Ա���Щ��ɫ���׶η��ʡ�
        //      offset��һ������������ָ��������ͳ�����Χ�ڹ��߲����е�ƫ��������λΪ�ֽڡ�
        //      size��һ������������ָ��������ͳ�����Χ�ڹ��߲����еĴ�С����λΪ�ֽڡ�
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