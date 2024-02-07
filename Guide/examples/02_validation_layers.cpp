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
};  // 这是一个常量字符串向量的定义，用于存储需要启用的校验层的名称，这里只启用了一个通用的校验层
    // KURONOS是一个国际非盈利组织，它负责制定和维护一些开放的标准，例如OpenGL和Vulkan等图形API。

#ifdef NDEBUG                               // 这是一个预处理指令，用于判断是否定义了NDEBUG宏，如果定义了，表示编译为发布模式
const bool enableValidationLayers = false;  // 这是一个常量布尔变量的定义，用于设置是否启用校验层，这里设置为false，表示不启用
#else                                       // 这是一个预处理指令，用于判断是否没有定义NDEBUG宏，如果没有定义，表示编译为调试模式
const bool enableValidationLayers = true;   // 这是一个常量布尔变量的定义，用于设置是否启用校验层，这里设置为true，表示启用
#endif                                      // 这是一个预处理指令，用于结束条件编译

// VkResult是一个枚举类型，它用于表示Vulkan API函数调用的返回值，可以是成功的完成码或者运行时的错误码。
// Utils是utility的复数形式，utility是一个英语单词，意思是实用程序或工具。
// EXT是extension的缩写，extension是一个英语单词，意思是扩展或扩充。
// CreateDebugUtilsMessengerEXT是一个自定义的函数，用于创建一个调试信息器对象，它可以用来在开发时检查和报告错误和警告。
// 它的参数分别是：
//  instance：       一个Vulkan实例对象，用于初始化Vulkan库和与Vulkan驱动程序交互。
//  pCreateInfo：    一个指向调试信息器创建信息结构体的指针，用于存储调试信息器创建所需的信息，例如调试信息的严重级别、类型和回调函数等。
//  pAllocator：     一个指向内存分配回调函数的指针，用于自定义内存分配和释放的方式，如果使用默认方式，可以设置为nullptr。
//                  创建调试信息器对象需要一个内存回调函数的指针，是因为这个指针用于自定义内存分配和释放的方式，
//                  也就是一个用于控制调试信息器对象所占用内存空间的函数。
//                  有了这个函数，就可以在创建或销毁调试信息器对象时，根据自己的需求或优化策略来分配或回收内存空间。
//                  如果不需要自定义内存管理方式，可以将这个指针设置为nullptr，表示使用默认的内存分配和释放方式。
//  pDebugMessenger：一个指向调试信息器对象的指针，用于接收创建成功后的调试信息器对象句柄(指针)。
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // PFN是pointer to function的缩写，表示函数指针的类型。
    // ProcAddr是procedure address的缩写，表示函数地址。
    // 它的参数分别是：
    //  instance：一个Vulkan实例对象或者VK_NULL_HANDLE，用于指定获取函数地址的范围，如果为VK_NULL_HANDLE，则只能获取全局范围或实例范围的函数地址；如果为一个有效的实例对象，则还能获取设备范围的函数地址。
    //  pName：一个指向以空字符结尾的字符串的指针，用于指定要获取地址的函数的名称。
    // 这是一个赋值语句，用于获取Vulkan库中的vkCreateDebugUtilsMessengerEXT函数的地址，并转换为PFN_vkCreateDebugUtilsMessengerEXT类型的函数指针。
    // vkCreateDebugUtilsMessengerEXT是一个Vulkan API函数的名称，它用于接收创建的调试信息器对象。
    // 这个函数的名称是一定的，不能随意修改或替换，否则会导致获取函数地址失败或者调用错误的函数。
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        // 这个函数调用会创建一个调试信息器对象，返回的结果是一个VkResult枚举值
        // 它表示是否成功创建了一个调试信息器对象，并将其句柄赋值给pDebugMessenger指针指向的变量。
        // 如果返回值等于VK_SUCCESS，表示创建成功；如果返回值等于VK_ERROR_EXTENSION_NOT_PRESENT，表示扩展不存在；
        // 如果返回值等于其他错误码，表示创建失败。
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

//  这是一个自定义的函数的声明，用于销毁一个调试信息器对象，参数分别为：Vulkan实例对象、要销毁的调试信息器对象、自定义内存分配回调函数指针；
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    // 这是一个赋值语句，用于获取Vulkan库中的vkDestroyDebugUtilsMessengerEXT函数的地址，并转换为PFN_vkDestroyDebugUtilsMessengerEXT类型的函数指针
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
    // 调试器是一种类似回调函数，用于接收和处理来自调试层的事件
    // 它们可以在运行时注入到vulkan命令的调用链中，检查是否存在任何错误、警告或者性能问题。
    // 我们可以在创建vulkan实例时启用调试层，并指定要启用的验证层的名称。
    // 我们可以通过vkCreateDebugReportCallbackEXT函数来创建调试器，并传入一个用户自定义的函数指针和一组感兴趣的事件标志。
    // 事件标志可以是以下几种之一或者组合：VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_DEBUG_BIT_EXT。
    // 每当有一个符合标志条件的事件发生时，就会触发回调函数，
    // 并将事件的类型、信息、对象等参数传递给用户自定义的函数。
    // 用户可以在这个函数中打印、记录或者处理这些事件。

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
        }// 要先于vkDestroyInstance执行

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }// 如果不支持校验层，那么checkValidationLayerSupport会返回false

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

        // 接下来定义扩展项
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());// vector.size() 返回的是一个 size_t 类型的值，它是一个无符号整数类型
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};   // 这个变量用来存储创建调试消息器对象所需的信息
        if (enableValidationLayers) {   // 检查是否启用了验证层。如果if，else消去，程序一样运行，但是不会在开始与结束建立一个临时调试器，获取创建与结束时候的调试信息
            // 如果启用了验证层，那么就设置 createInfo 结构体中的相关字段。
            // 设置 createInfo 结构体中的 enabledLayerCount 字段为 validationLayers 数组的大小。
            // validationLayers 数组是一个字符串向量，存储了我们想要使用的验证层名称
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            // 调用 populateDebugMessengerCreateInfo 函数，传入 debugCreateInfo 变量的引用作为参数。
            // 这个函数会根据我们想要使用的调试功能来填充 debugCreateInfo 结构体中的字段
            populateDebugMessengerCreateInfo(debugCreateInfo);
            // 设置 createInfo 结构体中的 pNext 字段为 debugCreateInfo 变量的地址，并强制转换为 VkDebugUtilsMessengerCreateInfoEXT 类型的指针。
            // 这个字段表示我们想要附加到 Vulkan 实例上的扩展结构体
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
            // 如果在vkCreateInstance函数中将pCreateInfo的pNext成员设定为一个VkDebugUtilsMessengerCreateInfoEXT结构体
            // 那么它会在创建实例对象的同时创建一个临时的调试消息器对象。
            // 这个调试消息器对象只在vkCreateInstance和vkDestroyInstance函数的执行期间有效，用于捕获在创建或销毁实例对象时发生的事件。如开始程序的时候调用回调函数输出各种信息。
            // 如果想要创建一个持久的调试消息器对象，您可以使用vkCreateDebugUtilsMessengerEXT函数,也就是setupDebugMessenger做的事情
        }
        else {
            // 如果没有启用验证层，那么就设置 createInfo 结构体中的 enabledLayerCount 字段为0，表示我们不需要启用任何验证层
            createInfo.enabledLayerCount = 0;
            // 设置 createInfo 结构体中的 pNext 字段为 nullptr，表示我们不需要附加任何扩展结构体
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    // 返回值类型是void，作用是创建并设置调试消息器对象
    void setupDebugMessenger() {
        //定义一个条件语句，用来检查是否启用了验证层。
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        // 这个函数会根据createInfo结构体中的信息来创建一个调试消息器对象，并将其赋值给debugMessenger变量。
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        // populate这里是填充的意思
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        // VkDebugUtilsMessengerCreateInfoEXT数据类型是一个结构体，用来存储创建调试消息器对象所需的信息。
        // 调试消息器对象可以在发生感兴趣的事件时触发一个调试回调函数，用来检测和记录Vulkan API的错误和警告。
        // 这个结构体包含以下字段：
        // sType：一个VkStructureType值，表示这个结构体的类型。它必须是VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT。
        // pNext：一个指针，表示附加到这个结构体的扩展信息。它可以是NULL或者指向其他结构体。
        // flags：一个VkDebugUtilsMessengerCreateFlagsEXT值，表示保留的标志位。它必须是0。
        // messageSeverity：一个VkDebugUtilsMessageSeverityFlagBitsEXT的位掩码，表示哪些级别的事件会触发回调函数。它可以是以下值的组合2：
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT：表示详细的事件，通常用于诊断目的。
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT：表示信息性的事件，通常用于通知目的。
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT：表示警告性的事件，通常用于提示可能的问题。
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT：表示错误性的事件，通常用于报告错误。
        // messageType：一个VkDebugUtilsMessageTypeFlagBitsEXT的位掩码，表示哪些类型的事件会触发回调函数。它可以是以下值的组合2：
        //      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT：表示一般性的事件，通常与Vulkan API无关。
        //      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT：表示验证性的事件，通常与Vulkan API的正确使用有关。
        //      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT：表示性能性的事件，通常与Vulkan API的效率有关。
        // pfnUserCallback：一个PFN_vkDebugUtilsMessengerCallbackEXT类型的函数指针，表示用户定义的回调函数。这个函数会在每次触发事件时被调用，并传入相应的参数。
        // pUserData：一个指针，表示用户自定义的数据。这个数据会被传递给回调函数。
    }

    // 定义一个函数，名字是getRequiredExtensions，返回值类型是字符串数组，作用是获取Vulkan实例所需的扩展名
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;    // 定义一个无符号整数变量，名字是glfwExtensionCount，初始化为0，用来存储GLFW库所需的扩展数量
        const char** glfwExtensions;        // 定义一个指向字符串指针的指针变量，名字是glfwExtensions，用来存储GLFW库所需的扩展名
            //char**是字符串的数组
            //char* names[] = { "Alice", "Bob", "Charlie" }; // 定义一个字符串数组
            //char** p = names; // 定义一个指向字符串数组的指针
            //cout << p[0] << endl; // 输出 Alice
            //cout << p[1] << endl; // 输出 Bob
            //cout << p[2] << endl; // 输出 Charlie

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        // 调用GLFW库提供的一个函数，这个函数会返回GLFW库所需的Vulkan实例扩展名，并更新glfwExtensionCount的值
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        // 定义一个字符串数组变量，名字是extensions，并使用glfwExtensions和glfwExtensionCount初始化它。
        // 这个变量用来存储所有Vulkan实例所需的扩展名，包括GLFW库所需的和其他可能需要的
        // const char* 是一个指向常量字符的指针，也就是说，它不能修改所指向的字符的值。
        //      const char* arr[] = {"one", "two", "three"}; // 定义一个字符串数组
        //      std::vector<const char*> vec(arr, arr + 3); // 使用数组的首地址和尾地址作为迭代器来初始化容器
        //      cout << vec[0] << endl; // 输出 one
        //      cout << vec[1] << endl; // 输出 two
        //      cout << vec[2] << endl; // 输出 three

        if (enableValidationLayers) { // 如果启用了验证层，那么增加一个扩展
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); 
            // VK_EXT_DEBUG_UTILS_EXTENSION_NAME 是一个宏定义，它的值是一个字符串常量“VK_EXT_debug_utils”
            // 在extensions数组中添加一个元素，表示Vulkan调试工具扩展的名称。
        }

        return extensions;
    }

    bool checkValidationLayerSupport() {    // 作用是检查Vulkan API是否支持验证层
        uint32_t layerCount;                // 用来存储可用的验证层数量
        // 调用Vulkan API提供的一个函数，传入layerCount的地址和空指针作为参数。这个函数会返回可用的验证层数量，并更新layerCount的值
        // Enumerate是枚举
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        // 我的vulkan可以支持11层的检验层；
        std::cout << layerCount << std::endl;

        // 定义一个VkLayerProperties类型的数组变量，使用layerCount初始化它。这个变量用来存储可用的验证层属性
        // VkLayerProperties是一个结构体类型，用来存储Vulkan API3中可用的验证层的属性。
        //  layerName：一个字符数组，表示验证层的名称。
        //  specVersion：一个无符号整数，表示验证层所遵循的Vulkan规范的版本号。
        //  implementationVersion：一个无符号整数，表示验证层的实现版本号。
        //  description：一个字符数组，表示验证层的描述信息。
        std::vector<VkLayerProperties> availableLayers(layerCount);
        // 再次调用Vulkan API提供的一个函数，传入layerCount的地址和availableLayers的数据指针作为参数。
        // 这个函数会返回可用的验证层属性，并更新availableLayers的值vector.data()返回的是一个指针。
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // 定义一个循环语句，遍历validationLayers数组中的每一个元素。
        for (const char* layerName : validationLayers) {
            // 定义一个布尔值变量，名字是layerFound，并初始化为false。这个变量用来标记我们想要使用的验证层是否在可用的验证层中找到
            bool layerFound = false;
            
            // vailableLayers数组是一个VkLayerProperties类型的数组，存储了可用的验证层属性
            for (const auto& layerProperties : availableLayers) {
                // 使用strcmp函数比较当前遍历到的验证层名称和可用验证层属性中的名称是否相同
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            // 如果在遍历完所有可用验证层后，layerFound变量仍然为false，
            // 那么说明我们想要使用的验证层不可用，那么就返回false作为函数的结果，并结束函数执行
            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        // 定义一个静态函数，名字是 debugCallback
        // 当发生一个事件时，Vulkan API会把与事件相关的信息存储在一个VkDebugUtilsMessengerCallbackDataEXT结构体中，并作为参数传递给回调函数。
        // 返回值类型是 VkBool32，一个32位无符号整数类型，用来表示布尔值。它可以是VK_TRUE或VK_FALSE。    
        // VKAPI_ATTR全写是Vulkan Application Programming Interface Attribute，表示Vulkan应用程序接口属性。
        // VKAPI_CALL：放在函数声明中的返回类型之后。用于MSVC样式的调用约定语法3。VKAPI_CALL是一个宏定义，用来控制函数的调用约定，比如stdcall、cdecl等。
        // 它们的作用是让不同的编译器和平台能够正确地调用Vulkan API的函数。它们的具体机制是根据编译器和平台的类型，展开为不同的关键字或符号，用来修饰函数的声明。
        // 这一函数使用 VKAPI_ATTR 和 VKAPI_CALL 定义，确保它可以被 Vulkan 库调用。
        
        // VKAPI_ATTR可以展开为以下关键字之一：空或
        // __declspec(dllexport)：一个函数属性，用于指示编译器将该函数导出为动态链接库（DLL）的一个符号，这样其他程序就可以通过动态加载DLL来调用该函数。
        // __attribute__((visibility(“default”))：一个函数属性，用于指示编译器将该函数导出为共享对象（SO）的一个符号，这样其他程序就可以通过动态加载SO来调用该函数。
        // VKAPI_CALL可以展开为以下关键字之一：空或
        // __stdcall：用于MSVC风格的编译器，表示一种调用约定，即由被调用函数清理堆栈，并且函数名前加上下划线后加上参数字节数作为后缀。
        // __cdecl：用于MSVC风格的编译器，表示一种调用约定，即由调用者清理堆栈，并且函数名前只加上下划线作为前缀。
        // MSVC是Microsoft Visual C++的缩写，表示微软的一个C++编译器和集成开发环境
  
        // 参数1：VkDebugUtilsMessageSeverityFlagBitsEXT：一个枚举类型，表示事件的严重性级别。它可以是以下值之一或者它们的组合：
        //  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT：表示详细的事件，通常用于诊断目的。
        //  VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT：表示信息性的事件，通常用于通知目的。
        //  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT：表示警告性的事件，通常用于提示可能的问题。
        //  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT：表示错误性的事件，通常用于报告错误。参数类型是
        // 参数2：VkDebugUtilsMessageTypeFlagsEXT：一个位掩码类型，表示事件的类型。它可以是以下值之一或者它们的组合：
        //  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT：表示一般性的事件，通常与Vulkan API无关。
        //  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT：表示验证性的事件，通常与Vulkan API的正确使用有关。
        //  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT：表示性能性的事件，通常与Vulkan API的效率有关。
        // 参数3：VkDebugUtilsMessengerCallbackDataEXT：一个结构体类型，表示调试消息的数据。它包含以下字段：
        //  sType：一个VkStructureType值，表示这个结构体的类型。它必须是VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT。
        //  pNext：一个指针，表示附加到这个结构体的扩展信息。它可以是NULL或者指向其他结构体。
        //  flags：一个保留的标志位。它必须是0。
        //  pMessageIdName：一个指向字符串的指针，表示消息ID名称。它可以是NULL或者指向一个以空字符结尾的字符串。
        //  messageIdNumber：一个32位整数，表示消息ID号码。它可以是0或者任意值。
        //  pMessage：一个指向字符串的指针，表示消息内容。它不能是NULL，并且必须指向一个以空字符结尾的字符串。
        //  queueLabelCount：一个无符号整数，表示队列标签数组中元素数量。它可以是0或者任意值。
        //  pQueueLabels：一个指向VkDebugUtilsLabelEXT结构体数组的指针，表示队列标签数组。它可以是NULL或者指向有效内存地址。
        //  cmdBufLabelCount：一个无符号整数，表示命令缓冲区标签数组中元素数量。它可以是0或者任意值。
        //  pCmdBufLabels：一个指向VkDebugUtilsLabelEXT结构体数组的指针，表示命令缓冲区标签数组。它可以是NULL或者指向有效内存地址。
        //  objectCount：一个无符号整数，表示对象数组中元素数量。它可以是0或者任意值。
        //  pObjects：一个指向VkDebugUtilsObjectNameInfoEXT结构体数组的指针，表示对象数组。它可以是NULL或者指向有效内存地址。
        
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        // 使用 std::cerr 对象输出一条错误信息，包括验证层的名称和调试消息的内容。
        // pCallbackData 是一个指向 VkDebugUtilsMessengerCallbackDataEXT 结构体的指针，表示调试消息的数据。
        // pCallbackData->pMessage 是一个指向字符串的指针，表示调试消息的内容
        return VK_FALSE;
    }
};

int main() {
    HelloTriangleApplication app;

    // 定义一个无符号整数变量，用来存储Loader的版本号
    uint32_t loaderVersion = 0;
    // 调用Vulkan API提供的一个函数，传入NULL和loaderVersion的地址作为参数。这个函数会返回Loader的版本号，并更新loaderVersion的值
    vkEnumerateInstanceVersion(&loaderVersion);
    // 定义三个无符号整数变量，用来存储Loader的主版本号、次版本号和补丁版本号
    uint32_t major = VK_VERSION_MAJOR(loaderVersion);
    uint32_t minor = VK_VERSION_MINOR(loaderVersion);
    uint32_t patch = VK_VERSION_PATCH(loaderVersion);
    // 打印Loader的版本号信息
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