#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h> 
#include <iostream> 
#include <stdexcept> 
#include <cstdlib> 

const uint32_t WIDTH = 800; 
const uint32_t HEIGHT = 600; 

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

    VkInstance instance; // ����һ��˽�еĳ�Ա���������������ڴ洢Vulkanʵ������ľ��

    void initWindow() { 
        glfwInit(); 
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); 
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr); 
    }

    void initVulkan() {     // ����һ��˽�еĳ�Ա�����Ķ��壬���ڳ�ʼ��Vulkan�⣬������һЩVulkan����
        createInstance();   // ����һ������������䣬���ڵ������ڲ���createInstance()����
    }

    void mainLoop() {       
        while (!glfwWindowShouldClose(window)) { 
            glfwPollEvents(); 
        }
    }

    void cleanup() { 
        vkDestroyInstance(instance, nullptr); // ����һ������������䣬��������Vulkanʵ������

        glfwDestroyWindow(window); 

        glfwTerminate(); 
    }

    void createInstance() {                                     // ��������������Ǵ���һ��Vulkanʵ������

        // Vulkan����������һ����������ڽ���API���ô��ݹ�����ָ������ݣ��������ǽ���ת����ʹ��Ӳ��������⡣
        // Vulkan����һ���ṩ��Vulkan API�����Ķ�̬���ӿ⣬������Ӧ�ó�����Ե���Vulkan API��Vulkan���������Vulkan�ⲻ��ͬһ���������������Ƕ���Vulkan API��һ���֡�
        
        // VkInstance��һ�����ڳ�ʼ��Vulkan�����Vulkan�������򽻻��Ķ���
        // ���԰��������һ����Vulkan���Vulkan��������֮���ͨ����Կ�ף�����һ����Ӧ�ó������ʹ��Vulkan���ܵ�����š�
        // Ҫʹ��Vulkan���ܣ�����Ҫ�ȴ���һ��VkInstance����Ȼ��ͨ��������ȡ������Vulkan�������������豸���߼��豸�����桢��������
        
        // ���Ҵ�ǳ��⣺Ҳ����VkInstance��ʼ����������ʹ��vulkan Apiȥ����vulkan����ʵ��api�Ĺ���,
        // Ҳ����vulkanֻ������һ������ݽṹ��ӿڣ����贴��һ��ʵ����ʹ��Щ���ݽṹ��ӿ����ӵ�����������������ǣ�����ʵ�������Ĺ��ܣ�

        // �����豸��һ����ʾϵͳ��֧��Vulkan��GPU�Ķ����������ṩGPU�����Ժ͹�����Ϣ��GPU��Ϣ�ļ���
        // Vulkan���߼��豸��һ����ʾӦ�ó�����ĳ���ض������豸��ص���Դ���������ִ�еĶ�������Ӧ�ó�����GPU֮��Ľӿڡ�����gpu�����ļ���
        // У�����һ����ѡ�Ŀ���������Vulkan API���������Ͻ��и��Ӳ����������У��㳣��������һЩ�����⡢��Դ׷�١���־��¼�������طŵȹ�����

        VkApplicationInfo appInfo{};                            // ����ṹ��������ڴ洢�Լ�������Ӧ�ó�������������Ϣ������������߷����Լ����������VkInstanceCreateInfoһ����ѡ�����Ϣ
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;     // ����ֶα�������ΪVK_STRUCTURE_TYPE_APPLICATION_INFO����ʾ����ṹ�������һ��Ӧ�ó�����Ϣ�ṹ��
        appInfo.pApplicationName = "Hello Triangle";            // ����ֶο�������Ϊ�����ַ�������ʾӦ�ó��������
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);  // ����ֶο�������Ϊ������������ʾӦ�ó���İ汾�ţ�����ʹ����VK_MAKE_VERSION��������һ���汾��
        appInfo.pEngineName = "No Engine";                      // ����ֶο�������Ϊ�����ַ�������ʾ��������ƣ����û��ʹ���κ����棬��������Ϊ"No Engine"
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);       // ����ֶο�������Ϊ������������ʾ����İ汾�ţ�����ʹ����VK_MAKE_VERSION��������һ���汾��
        appInfo.apiVersion = VK_API_VERSION_1_0;                // ����ֶα�������ΪӦ�ó���֧�ֵ���͵�Vulkan API�汾�ţ����¿��Ի��������Ҳ�ᵼ�¼������½�

        VkInstanceCreateInfo createInfo{};                          // ����ṹ��������ڴ洢ʵ�������������Ϣ
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;  // ����ֶα�������ΪVK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO����ʾ����ṹ�������һ��ʵ��������Ϣ�ṹ��
        createInfo.pApplicationInfo = &appInfo;                     // ����ֶο�������Ϊһ��Ӧ�ó�����Ϣ�ṹ�������ָ�룬��ʾ�ṩӦ�ó�������������Ϣ��Vulkan��������
                                                                    // ��������ṩ����������Ϊnullptr

        uint32_t glfwExtensionCount = 0;                        // ����������ڴ洢GLFW�������չ����
        const char** glfwExtensions;                            // ����������ڴ洢GLFW�������չ����
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); 
        // ��������������ڻ�ȡGLFW�������չ�����������������Ʒֱ�ֵ��glfwExtensionCount������glfwExtensions����

        // �洢GLFW�������չ����������ָ�ڴ���Vulkanʵ������ʱ����Ҫ֪��GLFW��Ҫ���ٸ���չ�����ҽ����ǵ����ƴ��ݸ�Vulkan�⡣
        // ������Vulkan��Ϳ��Ը���GLFW������������Ӧ����չ��ʹ��GLFW������������Vulkan������

        // GLFW����չ��һЩ������Vulkan�������ض�ƽ̨�Ĺ��ܣ����ǿ�����GLFW�����͹���һ��Vulkan����������ڽ�Vulkan��Ⱦ��������ʾ�ڴ����ϡ�
        // 1 GLFW����չ�����¼��֣�
        //  VK_KHR_surface�������չ������ƽ̨����Ҫ�ģ����ṩ��һ��ͨ�õĽӿڣ����ڴ��������ٱ�������Լ���ѯ�����������Ժ�֧�������2
        //  VK_KHR_win32_surface�������չ��Windowsƽ̨��Ҫ�ģ����ṩ��һ�����ڴ�Windows���ھ�������������ĺ�����3
        //  VK_KHR_xlib_surface�������չ��X11ƽ̨��Ҫ�ģ����ṩ��һ�����ڴ�X11���ھ�������������ĺ�����
        //  VK_KHR_xcb_surface�������չҲ��X11ƽ̨��Ҫ�ģ����ṩ��һ�����ڴ�XCB���ھ�������������ĺ�����
        //  VK_KHR_wayland_surface�������չ��Waylandƽ̨��Ҫ�ģ����ṩ��һ�����ڴ�Wayland���ھ�������������ĺ�����
        //  VK_KHR_metal_surface�������չ��macOSƽ̨��Ҫ�ģ����ṩ��һ�����ڴ�Metalͼ���������������ĺ�����
        // Vulkan����չ��һЩ��ѡ�Ĺ��ܣ��������ӻ��޸�Vulkan API����Ϊ�� Vulkan����չ�����¼������ͣ�
        //  �豸��չ����Щ��չ������ض������豸���߼��豸�ģ����ǿ����ṩһЩ����Ĺ��ܻ��Ż���������ز�����������ɫ������ȼ��õȡ�
        //  ʵ����չ����Щ��չ������ض�ʵ����ȫ�ַ�Χ�ģ����ǿ����ṩһЩ����Ĺ��ܻ��Ż���������Ա��桢�����ʽ���������ȡ�
        //  ����չ����Щ��չ������ض�У����ȫ�ַ�Χ�ģ����ǿ����ṩһЩ����Ĺ��ܻ��Ż����������ܷ����������Լ�顢API���ǵȡ�

        createInfo.enabledExtensionCount = glfwExtensionCount;  // ����ֶ������������õ���չ��������������ΪGLFW�������չ����
        createInfo.ppEnabledExtensionNames = glfwExtensions;    // ����ֶ������������õ���չ���ƣ���������ΪGLFW�������չ����

        createInfo.enabledLayerCount = 0;                       // ����ֶ������������õ�У�����������������Ϊ0����ʾ�������κ�У���
        // �����Ҫ����У��㣬��Ҫ�Ȼ�ȡ���õ�У����б�Ȼ��ѡ����Ҫ���õ�У��㣬���������������Ʒֱ�ֵ��enabledLayerCount��ppEnabledLayerNames�ֶ�

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) { 
            // ��������������ڴ���һ��Vulkanʵ�����󣬲���������ֵ��instance�������������ֵ������VK_SUCCESS����ʾ����ʧ��
            throw std::runtime_error("failed to create instance!"); // �����������ڴ���ʵ��ʧ��ʱ�׳�һ������ʱ�����쳣������쳣�ᱻmain�����е�try-catch��䲶�񲢴���
        }
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