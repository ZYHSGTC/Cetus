#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <algorithm>    //clmap��Ҫ�õ�
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
    // VK_KHR_SWAPCHAIN_EXTENSION_NAME��Vulkan��һ���豸��չ���������㴴���������������ڽ���Ⱦ������ֵ�һ�������ϡ�
    // ������������Ҫ��ʽ�ش�����������Ĭ�ϵĽ�������
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


struct SwapChainSupportDetails {                // ����һ���ṹ�壬���ڴ洢����֧�ֵ���Ϣ
    VkSurfaceCapabilitiesKHR capabilities;      // ��������������������Сͼ����������ߵ�
    // VkSurfaceCapabilitiesKHR�ṹ����������һ������Ļ�������������֧�ֵ�ͼ���������ߴ硢�������任������Alpha����;��1�����ĸ�����Ա�����ĺ������£�
    //  minImageCount��ָ���豸֧�ֵĽ�������ͼ�����С����������Ϊ1��
    //          �������е�ͼ���������ڱ�������ʾ֡�Ļ��������ϡ��������еĵ�һ�����������������ʾ�Ļ�������������̳�Ϊ����;
    //  maxImageCount��ָ���豸֧�ֵĽ�������ͼ���������������Ϊ0����ʾû�����ƣ��������ܵ��ڴ�ʹ�õ�Ӱ�졣
    //  currentExtent�Ǳ��浱ǰ�Ŀ�ߣ�����һ������ֵ(0xFFFFFFFF, 0xFFFFFFFF)��extent��������߷�Χ����˼����ʾ�����С��Ŀ�꽻����������
    //  minImageExtent�Ǳ���֧�ֵĽ�����Χ����ͼ��ߴ磩����Сֵ����߶�������currentExtent������currentExtent������ֵ��
    //  maxImageExtent�Ǳ���֧�ֵĽ�����Χ����ͼ��ߴ磩�����ֵ����߶���С��currentExtent��minImageExtent��
    //          currentExtent��maxImageExtent�Ĺ�ϵ�ǣ�ǰ�߱�ʾʵ��ʹ�õ�ͼ���С�����߱�ʾ�����ܵ�ͼ���С(��Ļ��С)��
    //          Ӧ�ó�����Ը����Լ�������ѡ����ʵ�ͼ���С�������뱣֤����maxImageExtent֮�ڣ����봰�ڱ���һ�»������Ӧ��
    //  maxImageArrayLayers�Ǳ���֧�ֵĳ���ͼ���������������Ϊ1��
    //  supportedTransforms��һ��VkSurfaceTransformFlagBitsKHR��λ���룬��ʾָ���豸֧�ֵı���任��������һ��λ�����á�
    //          �������°˸����ܵ�ֵ1��
    //          VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR������Ҫ�κα任������ԭ����
    //          VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR��˳ʱ����ת90�ȡ�
    //          VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR��˳ʱ����ת180�ȡ�
    //          VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR��˳ʱ����ת270�ȡ�
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR��ˮƽ����
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR��ˮƽ�������˳ʱ����ת90�ȡ�
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR��ˮƽ�������˳ʱ����ת180�ȡ�
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR��ˮƽ�������˳ʱ����ת270�ȡ�
    //  currentTransform��һ��VkSurfaceTransformFlagBitsKHRֵ����ʾ��������ڳ���������Ȼ����ĵ�ǰ�任��
    //          �������vkGetPhysicalDeviceSurfaceCapabilitiesKHR���ص�ֵ��ƥ�䣬����������ڳ��ֲ����ж�ͼ�����ݽ��б任��
    //          ��������supportedTransforms�е�һ��ֵ���������Ҫ�����κα任������ʹ��currentTransform��Ĭ��ֵ�����������Ȼ����
    //          VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR�����ֵ��ʾͼ����Ҫ�κα任������ԭ����
    //          VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR�����ֵ��ʾͼ����Ҫ˳ʱ����ת90�ȡ�
    //          VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR�����ֵ��ʾͼ����Ҫ˳ʱ����ת180�ȡ�
    //          VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR�����ֵ��ʾͼ����Ҫ˳ʱ����ת270�ȡ�
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR�����ֵ��ʾͼ����Ҫˮƽ����
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR�����ֵ��ʾͼ����Ҫˮƽ�������˳ʱ����ת90�ȡ�
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR�����ֵ��ʾͼ����Ҫˮƽ�������˳ʱ����ת180�ȡ�
    //          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR�����ֵ��ʾͼ����Ҫˮƽ�������˳ʱ����ת270�ȡ�
    //  supportedCompositeAlpha��һ��VkCompositeAlphaFlagBitsKHR��λ���룬��ʾָ���豸֧�ֵı��渴��Alphaģʽ��������һ��λ�����á�
    //          supportedCompositeAlpha ��һ�����ý�������ѡ��������˵����ͼ�������һ��ʱ�����ǵ�͸��������ô����ġ��������¼���ѡ��1��
    //          ����Alpha����ͨ��ʹ��û��Alpha������ͼ���ʽ������ȷ�����г���ͼ���е����ض���AlphaֵΪ1.0��ʵ�ֲ�͸�����ϡ�
    //          VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR�����ѡ���ʾͼ��͸��������������û��͸���ȡ�
    //          VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR�����ѡ���ʾͼ���͸���Ȼ�Ӱ����ӵ�Ч��������ͼ�����ɫrbg�Ѿ���͸���Ȼ�Ϲ���(���)��
    //          VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR�����ѡ���ʾͼ���͸���Ȼ�Ӱ����ӵ�Ч��������ͼ�����ɫrbg��û�к�͸���Ȼ�Ϲ�����Ҫ����������������ǡ�
    //          VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR�����ѡ���ʾ����������Լ�������ô����ͼ���͸���ȣ�Vulkan API ��֪������ķ�����
    //              Ӧ�ó�����Ҫ�����������������õ��ӵ�Ч�������û�����ã��ͻ���Ĭ�ϵķ�����
    //  supportedUsageFlags��һ��VkImageUsageFlags��λ���룬��ʾָ���豸֧�ֵı���ͼ����;��
    //          VK_IMAGE_USAGE_TRANSFER_SRC_BIT��ͼ������������������Դ��   SRC��source����˼��Դ����Դ��ԭ��
    //          VK_IMAGE_USAGE_TRANSFER_DST_BIT��ͼ������������������Ŀ�ꡣ  DST��destination����˼��Ŀ�ĵأ�Ŀ�꣬�յ�
    //          VK_IMAGE_USAGE_SAMPLED_BIT��ͼ�����������ɫ���в��������Ķ���
    //          VK_IMAGE_USAGE_STORAGE_BIT��ͼ�����������ɫ���д洢�����Ķ���
    //          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT��ͼ�����������ɫ������
    //          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT��ͼ������������ / ģ�帽����
    //          VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT��ͼ���Ƕ��ݸ�����
    //          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT��ͼ������������븽����

    // vulkan����������ָ����vulkan�������е�ͼ����ʾ�ڴ��ڻ���Ļ�ϵ������Ӳ��������������ǲ���ϵͳ��һ���֣�Ҳ�����Ƕ��������������⡣
    //          ���������ǽ�vulkanͼ��ת��Ϊ������Խ��ܵĸ�ʽ��������һЩ��Ҫ�ı任���ϳɺ�ͬ��������
    //          ��ͬ��ƽ̨���豸�����в�ͬ�ĳ������棬���Ƕ�vulkanͼ��Ĵ���ʽҲ���ܲ�ͬ��
    //          ��ˣ�vulkan�ṩ��һЩ�����ͱ�־λ����Ӧ�ó�����Բ�ѯ��ָ����������֧�ֵ����Ժ�ģʽ���Ա�֤ͼ���ܹ���ȷ����ʾ�ڱ����ϡ�
    std::vector<VkSurfaceFormatKHR> formats;    // ����֧�ֵĸ�ʽ�б�ÿ����ʽ������ɫ�ռ��ɫ�ʸ�ʽ
    // VkSurfaceFormatKHR�ṹ����������һ��֧�ֵĽ�������ʽ-��ɫ�ռ��2�����ĸ�����Ա�����ĺ������£�
    //          format��һ��VkFormatֵ����ʾ��ָ��������ݵĸ�ʽ��
    //              ��ʽ��һ�ֶ������ݵķ�ʽ����ͬ�ĸ�ʽ���Ա�ʾ��ͬ���������ͺͲ��֡�VkFormat�ж�����ܵ�ֵ������
    //              VK_FORMAT_R8G8B8A8_UNORM��ʾһ���ķ������޷��Ź�һ����ʽ��ÿ������ռ8λ��
    //              VK_FORMAT_D16_UNORM��ʾһ�����������޷��Ź�һ����ȸ�ʽ��ռ16λ�ȡ�
    //          colorSpace��һ��VkColorSpaceKHRֵ����ʾ��ָ��������ݵ���ɫ�ռ䡣
    //              ��ɫ�ռ���һ�ֶ�����ɫ�ķ�ʽ����ͬ����ɫ�ռ���Ա�ʾ��ͬ����ɫ��Χ��Ч����VkColorSpaceKHR�ж�����ܵ�ֵ��
    //              VK_COLOR_SPACE_SRGB_NONLINEAR_KHR��ʾsRGB��ɫ�ռ䣬
    //              VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT��ʾDisplay - P3��ɫ�ռ��1
    std::vector<VkPresentModeKHR> presentModes; // ����֧�ֵĳ���ģʽ�б�����FIFO��MAILBOX��   
    // ��������һ�����ڹ������������Ķ���������ʵ��˫�����������ȼ��������ͼ�����ܺ������ȡ�
    //      �����������������͵�ͼ����ɫ����ͼ��ͳ���Ŀ��ͼ��
    //      ��ɫ����ͼ����������Ⱦ���������ͼ�񣬳���Ŀ��ͼ����������ʾ�������ͼ��
    //      ����ģʽ����ָ��������ν���ɫ����ͼ��ת��Ϊ����Ŀ��ͼ�񣬲�������ֵ������ϡ�

    // �����ĳ���ģʽ�����֣�����ģʽ������ģʽ��FIFOģʽ��(�ҵ��豸ֻ֧������ģʽ��FIFOģʽ
    //      ����ģʽ�����������յ�һ���µ���ɫ����ͼ��ʱ�����������滻Ϊ��ǰ�ĳ���Ŀ��ͼ�񣬲�������ֵ������ϡ�
    //          ����ģʽ���ܻᵼ�»���˺�ѣ���Ϊ��ʾ��������ˢ�¹������յ��µ�ͼ��
    //      ����ģʽ�����������յ�һ���µ���ɫ����ͼ��ʱ���������һ�������У�������ֻ�������µ�һ��ͼ��
    //          ����ʾ����Ҫһ���µ�ͼ��ʱ���Ӷ�����ȡ�����µ�һ������������ֵ������ϡ�
    //          ����ģʽ���Ա��⻭��˺�ѣ������ܻᵼ�»����ӳ٣���Ϊ��ʾ�������޷���ʱ��ȡ���µ�ͼ��
    //      FIFOģʽ�����������յ�һ���µ���ɫ����ͼ��ʱ���������һ�������У������п��Ա������ͼ��
    //          ����ʾ����Ҫһ���µ�ͼ��ʱ���Ӷ����а����Ƚ��ȳ���˳��ȡ��һ������������ֵ������ϡ�
    //          ����ģʽҲ���Ա��⻭��˺�ѣ������ܻᵼ�»��濨�٣���Ϊ��ʾ�������޷�������Ⱦ���ߵ��ٶȡ�
    
    // VkPresentModeKHRö�������ڱ�ʾһ��֧�ֵĳ���ģʽ3���������¼������ܵ�ֵ��
    //          VK_PRESENT_MODE_IMMEDIATE_KHR������ģʽ�£��������󽻻�����ʾͼ��ʱ�����������滻��ǰ�ı���ͼ�񣬶����ܱ����Ƿ��ڴ�ֱͬ����vertical synchronization��״̬��
    //              ����ģʽ���ܵ���˺������tearing������Ҳ����͵��ӳ١�
    //          VK_PRESENT_MODE_FIFO_KHR������ģʽ�£��������е�ͼ�񱻵���һ���Ƚ��ȳ���FIFO���Ķ��д����������󽻻�����ʾͼ��ʱ��
    //              ���Ὣ�����еĵ�һ��ͼ����ʾ�ڱ����ϣ������µ�ͼ��������β����
    //              ����ģʽ��֤�˴�ֱͬ����������˺�����󣬵�Ҳ���ܵ����ӳٺͿ��١�
    //          VK_PRESENT_MODE_FIFO_RELAXED_KHR������ģʽ�£��������󽻻�����ʾͼ��ʱ���������Ϊ�գ���û�п��õ�ͼ�񣩣�
    //              ���Ὣ�µ�ͼ����ʾ�ڱ����ϣ������ܱ����Ƿ��ڴ�ֱͬ��״̬��������в�Ϊ�գ����ᰴ��FIFOģʽ������
    //              ����ģʽ��Ӧ�ó���֡�ʵ�����ʾ��ˢ����ʱ���ã����Ա��⿨�٣���Ҳ���ܵ���˺������
    //          VK_PRESENT_MODE_MAILBOX_KHR������ģʽ�£��������󽻻�����ʾͼ��ʱ������������������ﵽ�����ͼ����������
    //              ���Ὣ�µ�ͼ���滻�����е����һ��ͼ�񣬲��������еĵ�һ��ͼ����ʾ�ڱ����ϡ�
    //              ����ģʽ����ʵ�����ػ��壨triple buffering�����ڱ�֤��ֱͬ���ͱ���˺�������ͬʱ��Ҳ�нϵ͵��ӳ�
    //          VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR��ʾ���������Ӧ�ó�����Խ�����ͼ��ķ���Ȩ�ޡ�
    //              Ӧ�ó������ͨ��vkAcquireNextImageKHR��ȡһ��ͼ�񣬲���ֻ�е�Ӧ�ó������vkQueuePresentKHRʱ��ͼ��Żᱻ��ʾ��
    //              ���û���µĳ������󣬵�ǰͼ��ᱣ�ֲ��䡣
    //          VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR��ʾ���������Ӧ�ó�����Խ�����ͼ��ķ���Ȩ�ޡ�
    //              Ӧ�ó������ͨ��vkAcquireNextImageKHR��ȡһ��ͼ�񣬲���ֻҪӦ�ó��򱣳ֶԸ�ͼ��ķ���Ȩ�ޣ���ͼ��ͻᱻ��������ʾ��
    //              ���û���µĳ������󣬵�ǰͼ��ᱣ�ֲ��䡣

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
    // ��������һ������ͳ���ͼ�񻺳����Ķ����봰�ڱ���ͳ�����������������ڽ���Ⱦ�����ʾ�ڴ����ϡ�
    //      �������е�ͼ�񻺳�����Vulkanʵ�ֻ�ƽ̨������������������Ҫһ���߼��豸��һ����֮���ݵı��档
    //      Ӧ�ó�����Դӽ������л�ȡһ��ͼ�񻺳��������������Ⱦ������Ȼ�����ύ��������г��֡�
    //      �������������һ��ָ�򽻻���������ͼ�񻺳�����ָ�����飬����ͨ��vkGetSwapchainImagesKHR������ȡ��
    
    // ����������������������Ϊ���Ĺ���ԭ��������Ļ��������screen buffer���ͺ��򻺳�����back buffer��֮�佻��ͼ��
    //          ��Ļ����������ʾ����Ƶ�������ͼ�񣬺��򻺳����Ǵ洢��һ��Ҫ��ʾ��ͼ��ĵط���
    // ��������������������Ϊ������һϵ������֡��������ɵģ�ÿ��֡��������������Ϊ��Ļ����������򻺳�����
    
    // ������ͼ��
    //      ������ͼ����ͼ�񻺳���һ��һ�Ķ�Ӧ��ϵ��ÿ��������ͼ����һ����Ӧ��ͼ�񻺳��������ڴ洢ͼ�����ݡ�
    //      ������ͼ�����������͵�ͼ����ɫ����ͼ��ͳ���Ŀ��ͼ��
    //          ��ɫ����ͼ����������Ⱦ���������ͼ�񣬳���Ŀ��ͼ����������ʾ�������ͼ��
    //      ������ͼ��ķ��ʣ���Ҫ�ӽ�������ȡһ�����õ�ͼ��������Ȼ��ʹ��һ����֮���ݵĶ��������ύ������Ⱦ��������������command buffer����
    //          ����Ⱦ��ɺ�����Ҫ��ͼ�񷵻ظ��������������󽻻�����ͼ����ʾ�ڱ����ϡ�
    //          ��ͬ�Ķ�������Է��ʽ������е�ͼ�񣬵���Ҫ����һЩ����������֧�ֱ���ĳ���������ʹ�û����ź�����semaphore������ͬ���ȡ�
    // ������ͼ�������Ѳ��������ڣ�������ͼ����ָ���ǽ������а�����ͼ�񻺳���������������ͼ�񻺳����еĲ�����layer count��û�й�ϵ��
    //          ������ָͼ�񻺳����п��Դ洢���ٸ���άͼ��������ʵ��һЩ�����Ч�����������Ӿ���stereoscopic vision������ӽ���Ⱦ��multiview rendering����
    //          ��������ͼ�񻺳����Ĵ�������ָ���ģ��뽻�����޹ء�

   
    // ����������ʵ��˫�����������ȼ��������ͼ�����ܺ�������
    // ��������opengl���ж�Ӧ��opengl��Ҳ�������ڽ������Ļ��ƣ���Ϊ˫���壨double buffering����
    //          ˫����Ҳ��ʹ������֡�������������ȾЧ�ʺ��ȶ��ԣ�һ����Ϊ��Ļ��������һ����Ϊ���򻺳�����
    //          opengl��ʹ��glSwapBuffers������ʵ��֡�������Ľ�����

    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

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
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
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
        // ���ڴ�������������
        // ��ѯ�����豸�ͱ���Ľ�����֧��ϸ�ڣ������������ԡ���ʽ�ͳ���ģʽ
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        // �ӽ�����֧�ֵĸ�ʽ�б���ѡ��һ�����ʵı����ʽ���������ظ�ʽ����ɫ�ռ�
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        // �ӽ�����֧�ֵĳ���ģʽ�б���ѡ��һ�����ʵĳ���ģʽ��������ͼ������Ļ����ʾ�ķ�ʽ
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        // �ӽ�����֧�ֵı���������ѡ��һ�����ʵķֱ��ʣ�������ͼ�񻺳����Ĵ�С
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // ���ݽ�����֧�ֵ���Сͼ��������һ��ȷ����������ͼ�񻺳�����������������������ͼ����������ȡ���ֵ
        // ���ݽ�����֧�ֵ���Сͼ��������һ��ȷ����������ͼ�񻺳����������������Ա�֤������һ��ͼ�񻺳�������������Ⱦ��
        //      ����һ��ͼ�񻺳����������ڳ��֡������Ϳ���ʵ��˫���壬�����ȾЧ�ʺ��ȶ��ԡ�
        // ������������ͼ����������ȡ���ֵ����Ϊ��������ͼ����С�����ܳ�������֧�ֵ�ͼ������
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        /*VkSwapchainCreateInfoKHR{
        * 1
            VkStructureType                  sType;
            const void* pNext;
            VkSwapchainCreateFlagsKHR        flags;
            VkSurfaceKHR                     surface;
        * 2
            uint32_t                         minImageCount;
            VkFormat                         imageFormat;
            VkColorSpaceKHR                  imageColorSpace;
            VkExtent2D                       imageExtent;
            uint32_t                         imageArrayLayers;
            VkImageUsageFlags                imageUsage;
        * 3
            VkSharingMode                    imageSharingMode;
            uint32_t                         queueFamilyIndexCount;
            const uint32_t* pQueueFamilyIndices;
        * 4
            VkSurfaceTransformFlagBitsKHR    preTransform;
            VkCompositeAlphaFlagBitsKHR      compositeAlpha;
            VkPresentModeKHR                 presentMode;
            VkBool32                         clipped;
            VkSwapchainKHR                   oldSwapchain;
        }�����������ʼ������*/
        // ����һ��VkSwapchainCreateInfoKHR�ṹ�壬���ڴ洢��������������Ĳ���������֮���ܸı䣡
        VkSwapchainCreateInfoKHR createInfo{};

        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR; // ���ýṹ�����ͱ���ΪVK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR
        createInfo.surface = surface;                                   // ����Ҫ�󶨵��������ı������

        createInfo.minImageCount = imageCount;                          // ���ý�����ͼ�񻺳��������ͼ����
        createInfo.imageFormat = surfaceFormat.format;                  // ���ý�����ͼ�񻺳�������ɫ��ʽΪ�Լ�ѡ��һ��������ɫ��ʽ
        createInfo.imageColorSpace = surfaceFormat.colorSpace;          // ���ý�����ͼ�񻺳�������ɫ��ʽΪ�Լ�ѡ��һ��������ɫ�ռ�
        createInfo.imageExtent = extent;                                // ���ý�����ͼ�񻺳����ķֱ��������һ��
        createInfo.imageArrayLayers = 1;                                // ���ý�����ͼ�񻺳����Ĳ���
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;    // ���ý�����ͼ�񻺳�������;����ɫ����

        
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice); // ��ѯ�����豸֧�ֵĶ���������������ͼ�ζ�����ͳ��ֶ�����
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
        // imageSharingMode��һ��ö�����ͣ�������ָ��һ��Vulkan��Դ����ͼ�񻺳�������α���ͬ�Ķ����壨queue family�����ʺ͹���
        // imageSharingMode���������ܵ�ֵ��
        //      VK_SHARING_MODE_EXCLUSIVE����ʾ����Դ��ͬһʱ��ֻ�ܱ�һ���������ռ���ʡ�
        //          ���Ҫ�ڲ�ͬ�Ķ�����֮���л����ʣ�����ִ��һ������������Ȩת�ƣ�queue family ownership transfer��������
        //      VK_SHARING_MODE_CONCURRENT����ʾ����Դ���Ա���������岢�����ʣ�����ִ������Ȩת�Ʋ��������ǣ�����ģʽ���ܻή�ͷ������ܡ�
        if (indices.graphicsFamily != indices.presentFamily) {
            // ���ͼ�ζ�����ͳ��ֶ����岻ͬ�������ý�����ͼ�񻺳����Ĺ���ģʽΪ����ģʽ����ָ����������������
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;   
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {// �������ý�����ͼ�񻺳����Ĺ���ģʽΪ��ռģʽ
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;   // ���ý�����ͼ�񻺳����ı任��ʽΪ��ǰ����֧�ֵı任��ʽ��ͨ��Ϊ�ޱ任
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;              // ���ý�����ͼ�񻺳����Ļ�Ϸ�ʽΪ��͸����ϣ�������alphaͨ��
        createInfo.presentMode = presentMode;                                       // ���ý�����ͼ�񻺳����ĳ���ģʽΪ֮ǰѡ��ĳ���ģʽ
        createInfo.clipped = VK_TRUE;                                               // ���ý�����ͼ�񻺳����Ĳü���ʽΪ�ü��������Ա������ڵ�������
        createInfo.oldSwapchain = VK_NULL_HANDLE;                                   // ���ý������ľɽ���������ΪVK_NULL_HANDLE����ʾ����Ҫ�ؽ�������

        // ����vkCreateSwapchainKHR������ʹ�ô�����Ϣ�ṹ����߼��豸���󣬴������������󣬲��洢��swapChain������
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        // �ٴε���vkGetSwapchainImagesKHR������ʹ���߼��豸����ͽ��������󣬻�ȡ��������ͼ�񻺳����ľ�������洢��swapChainImages������
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);   // ��ѯ��������ͼ�񻺳������������������3
        swapChainImages.resize(imageCount);                                 // ����ͼ�񻺳���������������swapChainImages�����Ĵ�С
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        // ��������ͼ�񻺳����ĸ�ʽ�ͷֱ��ʴ洢��swapChainImageFormat��swapChainExtent�����У��Ա����ʹ��
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        // ����豸�Ƿ����Լ�����Ҫ����ɫ��ʽ���������Լ���Ҫ�ı����ʽ�ṹ��VkSurfaceFormatKHR��
        // ʹ����õ���ɫ��ʽsrbg(int)��srbg��������ɫ�ռ�
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        // ����豸�Ƿ����Լ���֧�ֵĳ��ָ�ʽ���������Լ���Ҫ�ĳ���ģʽö��ֵVkPresentModeKHR��
        // Ĭ��ʹ�õ�����ģʽ�ĳ���ģʽ������ʹ�ö���������Ľ������������ҵ��豸����֧�֣�
        // �����ҵ���ֻ֧��˫�ػ��壬��֧�����ػ��壻
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                std::cout << "you" << std::endl;
                return availablePresentMode;
            }
        }

        // ��Ϊ�ҵ��豸��֧������ģʽ������ʹ��FIFOģʽ��
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        // ����ѡ�񽻻����ķֱ���
        // currentExtentΪһ������ֵ(0xFFFFFFFF, 0xFFFFFFFF)
        // Ҳ����std::numeric_limits<uint32_t>::max()),����ζ�ű���֧������ֱ���
        // std::numeric_limits��һ��ģ���࣬���ṩ��һЩ��̬��Ա�����ͳ��������ڲ�ѯ��ͬ��ֵ���͵����Ժ�����
        // ���磬std::numeric_limits<int>::min()����һ��int���͵���Сֵ��std::numeric_limits<double>::epsilon()����һ��double���͵Ļ������ȡ�
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            // �������֧������ֱ��ʣ���ôʹ�ô��ڵĵ�ǰ�ֱ���
            return capabilities.currentExtent;
        }
        else {
            // ���򣬻�ȡ���ڵ�֡�����С
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            // ����һ��VkExtent2D�ṹ�壬���ڴ洢���ڵķֱ���
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            // �����ڵķֱ��������ڱ���֧�ֵ���С�����ֱ���֮��
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            // ����ѡ��ķֱ���
            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        // ��������豸���������߱���������Ϣ�������������ԣ�����ĳ�����ɫ��ʽ�����ģʽ��������ṹ�壻
        /*struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        }*/
        SwapChainSupportDetails details;
        // ��������豸��������
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
        // �ҵ��Ե������ǣ� 
        //  minImageCount           2
        //  maxImageCount           64
        //  currentExtent           Ŀǰ���ڴ�С
        //  minImageExtent          glfw���õĴ�С
        //  maxImageExtent          glfw���õĴ�С
        //  maxImageArrayLayers     2048
        //  supportedTransforms     1
        //      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR��0001
        //      VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR��0010
        //      VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR��0100
        //      VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR��1000
        //      VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR��00010000
        //      VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR��00100000
        //      VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR��01000000
        //      VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR��10000000
        //      VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR��100000000
        //      ��ζ�ű���ֻ֧��һ�ֱ任��ʽ��VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,��ʾ�������κα任������ԭʼ����
        //  currentTransform        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        //  supportedCompositeAlpha 9
        //      Vulkan֧���������ֻ�Ϸ�ʽ2��
        //      (1).VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR����ʾ����alpha��������ͼ����Ϊ��͸���ġ�����ֵΪ0x00000001��������Ϊ0001��
        //      (2).VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR����ʾ����alpha����������ͼ���Ѿ���alpha����Ԥ�˹�������ֵΪ0x00000002��������Ϊ0010��
        //      (3).VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR����ʾ����alpha����������ͼ��û�б�alpha����Ԥ�˹�������ֵΪ0x00000004��������Ϊ0100��
        //      (4).VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR����ʾ��ȷ��alpha�����Ĵ���ʽ����Ӧ�ó���ͨ�����ش���ϵͳ���������û�Ϸ�ʽ������ֵΪ0x00000008��������Ϊ1000��
        //      supportedCompositeAlpha����9����ô��ζ����ı���֧�����ֻ�Ϸ�ʽ(1)��(4)����Ϊ9�Ķ����Ʊ�ʾΪ1001
        //  supportedUsageFlags     31
        //      (1).VK_IMAGE_USAGE_TRANSFER_SRC_BIT��00000001    
        //      (2).VK_IMAGE_USAGE_TRANSFER_DST_BIT��00000010
        //      (3).VK_IMAGE_USAGE_SAMPLED_BIT��00000100
        //      (4).VK_IMAGE_USAGE_STORAGE_BIT��00001000
        //      (5).VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT��00010000
        //      (6).VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT��00100000
        //      (7).VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT��01000000
        //      (8).VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT��10000000
        //      31�Ķ����Ʊ�ʾΪ11111����ζ�Ž�����ͼ�񻺳���֧��������;:(1) (2) (3) (4) (5);

        // ����豸֧�ֵı�����ɫ��ʽ���� 4��
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        // ����豸֧�ֵı�����ɫ��ʽ��Ϣ 
        // {format=VK_FORMAT_B8G8R8A8_UNORM (44) colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (0) }
        // {format=VK_FORMAT_B8G8R8A8_SRGB (50) colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (0) }
        // {format=VK_FORMAT_R8G8B8A8_UNORM (37) colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (0) }
        // {format=VK_FORMAT_R8G8B8A8_SRGB (43) colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (0) }
        if (formatCount != 0) {
            details.formats.resize(formatCount);    // Ԥ�ȷ���format vector�Ĵ�С����������
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // ����豸֧�ֵĳ���ģʽ���� 2��
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        // ����豸֧�ֵĳ���ģʽ��Ϣ��
        // VK_PRESENT_MODE_IMMEDIATE_KHR ֧������ģʽ
        // VK_PRESENT_MODE_FIFO_KHR      ֧��FIFOģʽ
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }
        // Ϊʲô�ҵ���ֻ֧��˫���壬�����ұ������֧�ֵ�ͼ������64
        //      һ�������������г���2��3��ͼ�񻺳�������ȡ�����ڴ���������ʱָ����ͼ�񻺳�����������
        //      �������ģʽ����ֱ��Ӱ�콻�����ж��ٸ�����������������Ӱ�콻�����е�ͼ�񻺳�����α����ֵ������ϡ�
        //      ˫�������������ָ������������������������Ŀ��ͼ��(����)��������������ʾ����Ļ�ϡ�
        // ���ر��豸��SwapChainSupportDetails�ṹ�壬����������ϸ��Ϣ��
        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {// ���������豸��ʱ�򻹼��һ���豸�ǲ���֧�ֽ�����
        QueueFamilyIndices indices = findQueueFamilies(device);

        // ����豸�Ƿ�������Ҫ����չ��deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME} 
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            // ��������豸���������߱���������Ϣ�������������ԣ�����ĳ�����ɫ��ʽ�����ģʽ��
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            // ����豸����֧��һ��������ɫ��ʽ�����ģʽ����ô���������ã�
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        // ����豸�����Լ���Ҫ�Ķ����壬��֧�ֽ�����������չ��ͬʱ�豸֧�ֽ���������ͼ����Ļ��(����豸����֧��һ��������ɫ��ʽ�����ģʽ)���򷵻��棻
        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) { // ����豸�Ƿ�����������Ҫ����չdeviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME} 
        uint32_t extensionCount; 
        // ͨ�������豸����ҵ�����֧�ֵ�ȫ����չ��
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        // VkExtensionProperties��һ��vulkan��չ��Ϣ�ṹ�壬������չ�����ƺͰ汾�š�
        // ����ҵĵ�����֧�ֵ�57����չ��������availableExtensions��չ����vector���棻
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // �����Լ���Ҫ��vulkan��չ��
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        // ����ȫ����vulkan�汾�ţ������������Ҫ����չ����ô��Ӧ����requiredExtensions(��Ҫ��չ)��ɾȥ��Ӧ��չ
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        // �������������������Ҫ����չ���У���ôrequiredExtensionsΪ�գ�����true�������Ϊ�գ�����false��
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