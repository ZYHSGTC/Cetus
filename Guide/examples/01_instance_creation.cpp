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

    VkInstance instance; // 这是一个私有的成员变量的声明，用于存储Vulkan实例对象的句柄

    void initWindow() { 
        glfwInit(); 
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); 
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr); 
    }

    void initVulkan() {     // 这是一个私有的成员函数的定义，用于初始化Vulkan库，并创建一些Vulkan对象
        createInstance();   // 这是一个函数调用语句，用于调用类内部的createInstance()函数
    }

    void mainLoop() {       
        while (!glfwWindowShouldClose(window)) { 
            glfwPollEvents(); 
        }
    }

    void cleanup() { 
        vkDestroyInstance(instance, nullptr); // 这是一个函数调用语句，用于销毁Vulkan实例对象

        glfwDestroyWindow(window); 

        glfwTerminate(); 
    }

    void createInstance() {                                     // 这个函数的作用是创建一个Vulkan实例对象

        // Vulkan驱动程序是一个软件，用于接收API调用传递过来的指令和数据，并将它们进行转换，使得硬件可以理解。
        // Vulkan库是一个提供了Vulkan API函数的动态链接库，用于让应用程序可以调用Vulkan API。Vulkan驱动程序和Vulkan库不是同一个东西，但是它们都是Vulkan API的一部分。
        
        // VkInstance是一个用于初始化Vulkan库和与Vulkan驱动程序交互的对象。
        // 可以把它想象成一个打开Vulkan库和Vulkan驱动程序之间的通道的钥匙，或者一个让应用程序可以使用Vulkan功能的入口门。
        // 要使用Vulkan功能，你需要先创建一个VkInstance对象，然后通过它来获取其他的Vulkan对象，例如物理设备、逻辑设备、表面、交换链等
        
        // 自我粗浅理解：也就是VkInstance初始化，让我们使用vulkan Api去连接vulkan驱动实现api的功能,
        // 也就是vulkan只是类似一大堆数据结构与接口，还需创建一个实例，使这些数据结构与接口连接到、建立到驱动软件那，才能实现真正的功能；

        // 物理设备是一个表示系统中支持Vulkan的GPU的对象，它可以提供GPU的属性和功能信息。GPU信息的集合
        // Vulkan的逻辑设备是一个表示应用程序与某个特定物理设备相关的资源分配和命令执行的对象，它是应用程序与GPU之间的接口。调用gpu操作的集合
        // 校验层是一个可选的可以用来在Vulkan API函数调用上进行附加操作的组件。校验层常被用来做一些错误检测、资源追踪、日志记录、分析回放等工作。

        VkApplicationInfo appInfo{};                            // 这个结构体变量用于存储自己开发的应用程序或者引擎的信息，可以任意或者符合自己的情况，是VkInstanceCreateInfo一个可选择的信息
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;     // 这个字段必须设置为VK_STRUCTURE_TYPE_APPLICATION_INFO，表示这个结构体变量是一个应用程序信息结构体
        appInfo.pApplicationName = "Hello Triangle";            // 这个字段可以设置为任意字符串，表示应用程序的名称
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);  // 这个字段可以设置为任意整数，表示应用程序的版本号，这里使用了VK_MAKE_VERSION宏来生成一个版本号
        appInfo.pEngineName = "No Engine";                      // 这个字段可以设置为任意字符串，表示引擎的名称，如果没有使用任何引擎，可以设置为"No Engine"
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);       // 这个字段可以设置为任意整数，表示引擎的版本号，这里使用了VK_MAKE_VERSION宏来生成一个版本号
        appInfo.apiVersion = VK_API_VERSION_1_0;                // 这个字段必须设置为应用程序支持的最低的Vulkan API版本号，过新可以获得新性能也会导致兼容性下降

        VkInstanceCreateInfo createInfo{};                          // 这个结构体变量用于存储实例创建所需的信息
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;  // 这个字段必须设置为VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO，表示这个结构体变量是一个实例创建信息结构体
        createInfo.pApplicationInfo = &appInfo;                     // 这个字段可以设置为一个应用程序信息结构体变量的指针，表示提供应用程序或者引擎的信息给Vulkan驱动程序
                                                                    // 如果不想提供，可以设置为nullptr

        uint32_t glfwExtensionCount = 0;                        // 这个变量用于存储GLFW所需的扩展数量
        const char** glfwExtensions;                            // 这个变量用于存储GLFW所需的扩展名称
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); 
        // 这个函数调用用于获取GLFW所需的扩展，并将其数量和名称分别赋值给glfwExtensionCount变量和glfwExtensions变量

        // 存储GLFW所需的扩展数量，就是指在创建Vulkan实例对象时，需要知道GLFW需要多少个扩展，并且将它们的名称传递给Vulkan库。
        // 这样，Vulkan库就可以根据GLFW的需求，启用相应的扩展，使得GLFW可以正常地与Vulkan交互。

        // GLFW的扩展是一些用于与Vulkan交互的特定平台的功能，它们可以让GLFW创建和管理一个Vulkan表面对象，用于将Vulkan渲染的内容显示在窗口上。
        // 1 GLFW的扩展有以下几种：
        //  VK_KHR_surface：这个扩展是所有平台都需要的，它提供了一个通用的接口，用于创建和销毁表面对象，以及查询表面对象的属性和支持情况。2
        //  VK_KHR_win32_surface：这个扩展是Windows平台需要的，它提供了一个用于从Windows窗口句柄创建表面对象的函数。3
        //  VK_KHR_xlib_surface：这个扩展是X11平台需要的，它提供了一个用于从X11窗口句柄创建表面对象的函数。
        //  VK_KHR_xcb_surface：这个扩展也是X11平台需要的，它提供了一个用于从XCB窗口句柄创建表面对象的函数。
        //  VK_KHR_wayland_surface：这个扩展是Wayland平台需要的，它提供了一个用于从Wayland窗口句柄创建表面对象的函数。
        //  VK_KHR_metal_surface：这个扩展是macOS平台需要的，它提供了一个用于从Metal图层句柄创建表面对象的函数。
        // Vulkan的扩展是一些可选的功能，可以增加或修改Vulkan API的行为。 Vulkan的扩展有以下几种类型：
        //  设备扩展：这些扩展是针对特定物理设备或逻辑设备的，它们可以提供一些额外的功能或优化，例如多重采样、几何着色器、深度剪裁等。
        //  实例扩展：这些扩展是针对特定实例或全局范围的，它们可以提供一些额外的功能或优化，例如调试报告、表面格式、交换链等。
        //  层扩展：这些扩展是针对特定校验层或全局范围的，它们可以提供一些额外的功能或优化，例如性能分析、兼容性检查、API覆盖等。

        createInfo.enabledExtensionCount = glfwExtensionCount;  // 这个字段用于设置启用的扩展数量，这里设置为GLFW所需的扩展数量
        createInfo.ppEnabledExtensionNames = glfwExtensions;    // 这个字段用于设置启用的扩展名称，这里设置为GLFW所需的扩展名称

        createInfo.enabledLayerCount = 0;                       // 这个字段用于设置启用的校验层数量，这里设置为0，表示不启用任何校验层
        // 如果想要启用校验层，需要先获取可用的校验层列表，然后选择需要启用的校验层，并将其数量和名称分别赋值给enabledLayerCount和ppEnabledLayerNames字段

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) { 
            // 这个函数调用用于创建一个Vulkan实例对象，并将其句柄赋值给instance变量，如果返回值不等于VK_SUCCESS，表示创建失败
            throw std::runtime_error("failed to create instance!"); // 这个语句用于在创建实例失败时抛出一个运行时错误异常，这个异常会被main函数中的try-catch语句捕获并处理
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