#define GLFW_INCLUDE_VULKAN     // 这是一个预处理指令，用于告诉编译器包含Vulkan的头文件
// 这一句是一个预处理指令，它的作用是让GLFW库自动包含Vulkan的头文件，也就是<vulkan/vulkan.h>。
// 这样，你就不需要再单独包含Vulkan的头文件，而且可以避免一些潜在的冲突或错误。
// 如果你不使用这个预处理指令，那么你需要在包含GLFW的头文件之前，先包含Vulkan的头文件，像这样：
// #include <vulkan/vulkan.h>
// #include <GLFW/glfw3.h>

#include <GLFW/glfw3.h>         
#include <iostream> 
#include <stdexcept>            // 这是一个头文件，用于引入标准异常库
#include <cstdlib>              // 这是一个头文件，用于引入标准通用库
// cstdlib是C++里面的一个常用函数库，等价于C中的< stdlib.h >。1 stdlib.h可以提供一些函数与符号常量，具体如下：
// 根据 ISO标准 ，stdlib.h提供以下类型：size_t, wchar_t, div_t, ldiv_t, lldiv_t2
// 常量：NULL, EXIT_FAILURE, EXIT_SUCCESS, RAND_MAX, MB_CUR_MAX2
// 函数：atof, atoi, atol, strtod, strtof, strtols, strtol, strtoll, strtoul, strtoull, rand, srand, calloc, free, malloc, realloc,
// abort, atexit, exit, getenv, system, bsearch, qsort, abs, div, labs, ldiv, llabs, tlldiv, mblen, mbtowc, wctomb, mbstowcs, wcstombs2

const uint32_t WIDTH = 800;     // 这是一个常量，用于定义窗口的宽度为800像素
const uint32_t HEIGHT = 600;    // 这是一个常量，用于定义窗口的高度为600像素
//uint32_t和int都是整数类型，但是它们有以下区别：
//  uint32_t是无符号的32位整数，也就是说它只能表示0到4294967295之间的非负整数。
//  它的每一位都用于存储数值，没有符号位。
//  int是有符号的整数，它的大小在不同的平台和编译器上可能有所不同，但通常是32位或64位。
//  它可以表示正数、负数和零，它的最高位用于表示符号，0表示正数，1表示负数。
// 
//你可以在以下情况下使用uint32_t：
//  当你需要确保整数类型的大小和范围在不同的平台上一致时，例如在跨平台的编程或数据传输中。
//  当你需要处理一些特定的二进制操作或算法时，例如位运算、哈希函数、加密等。
//  当你需要利用无符号整数的特性时，例如无符号溢出、循环移位等。

class HelloTriangleApplication { // 这是一个类的定义，用于封装三角形应用程序的逻辑和数据
public: 
    void run() { 
        initWindow(); 
        initVulkan(); 
        mainLoop(); 
        cleanup(); 
    }

private: 
    GLFWwindow* window; 

    void initWindow() {                                 // 这是一个私有的成员函数的定义，用于初始化GLFW库，并创建一个窗口
        glfwInit();                                     // 这是一个函数调用语句，用于初始化GLFW库
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);   // 这是一个函数调用语句，用于设置窗口提示为不使用任何客户端API（即Vulkan）
        // glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 这句的作用是告诉GLFW库不要为窗口创建任何客户端API的上下文，例如OpenGL或OpenGL ES
        // 这样做的原因是我们要使用Vulkan API来渲染窗口，而Vulkan API不需要GLFW库提供的上下文，而是需要我们自己创建一个Vulkan实例和一个Vulkan表面。
        // 这样设置之后，并不意味着不能使用Vulkan API，恰恰相反，这样设置之后才能使用Vulkan API。
        // 如果我们不设置这句，那么GLFW库会默认为窗口创建一个OpenGL上下文，这样就会和Vulkan API冲突，导致无法正确渲染窗口。
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);     // 这是一个函数调用语句，用于设置窗口提示为不可调整大小
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr); 
        // 这是一个赋值语句，用于创建一个窗口，并将其指针赋值给window变量
        // 4:monitor：窗口的全屏模式所使用的显示器，如果为NULL，则表示窗口是窗口化的。
        // 5:share：用于共享上下文对象的另一个窗口，如果为NULL，则表示不共享。
    }

    void initVulkan() { // 这是一个私有的成员函数的定义，用于初始化Vulkan库，并创建一些Vulkan对象

    }

    void mainLoop() {   // 这是一个私有的成员函数的定义，用于循环渲染三角形，并处理窗口事件
        while (!glfwWindowShouldClose(window)) { // 这是一个循环语句，用于判断窗口是否应该关闭
            glfwPollEvents(); // 这是一个函数调用语句，用于处理窗口事件
        }
    }

    void cleanup() { // 这是一个私有的成员函数的定义，用于释放所有初始化时创建的资源，并终止GLFW库
        glfwDestroyWindow(window); // 这是一个函数调用语句，用于销毁窗口对象
        glfwTerminate(); // 这是一个函数调用语句，用于终止GLFW库
    }
};

int main() { // 这是程序的入口点，也就是程序开始执行的地方
    HelloTriangleApplication app; // 创建一个三角形应用程序对象，进行初始化

    try {           // 这是一个异常处理语句的开始，用于尝试运行三角形应用程序
        app.run();  // 这是一个函数调用语句，用于调用三角形应用程序对象的run()函数
    }
    catch (const std::exception& e) {       // 这是一个异常处理语句的结束，用于捕获可能发生的异常
        std::cerr << e.what() << std::endl; // 这是一个输出语句，用于将异常的信息输出到标准错误流
        return EXIT_FAILURE;                // 这是一个返回语句，用于在发生异常时返回一个失败的状态码给操作系统
    }

    return EXIT_SUCCESS; // 这是一个返回语句，用于在正常结束时返回一个成功的状态码给操作系统
}
