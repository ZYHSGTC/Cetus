#define GLFW_INCLUDE_VULKAN     // ����һ��Ԥ����ָ����ڸ��߱���������Vulkan��ͷ�ļ�
// ��һ����һ��Ԥ����ָ�������������GLFW���Զ�����Vulkan��ͷ�ļ���Ҳ����<vulkan/vulkan.h>��
// ��������Ͳ���Ҫ�ٵ�������Vulkan��ͷ�ļ������ҿ��Ա���һЩǱ�ڵĳ�ͻ�����
// ����㲻ʹ�����Ԥ����ָ���ô����Ҫ�ڰ���GLFW��ͷ�ļ�֮ǰ���Ȱ���Vulkan��ͷ�ļ�����������
// #include <vulkan/vulkan.h>
// #include <GLFW/glfw3.h>

#include <GLFW/glfw3.h>         
#include <iostream> 
#include <stdexcept>            // ����һ��ͷ�ļ������������׼�쳣��
#include <cstdlib>              // ����һ��ͷ�ļ������������׼ͨ�ÿ�
// cstdlib��C++�����һ�����ú����⣬�ȼ���C�е�< stdlib.h >��1 stdlib.h�����ṩһЩ��������ų������������£�
// ���� ISO��׼ ��stdlib.h�ṩ�������ͣ�size_t, wchar_t, div_t, ldiv_t, lldiv_t2
// ������NULL, EXIT_FAILURE, EXIT_SUCCESS, RAND_MAX, MB_CUR_MAX2
// ������atof, atoi, atol, strtod, strtof, strtols, strtol, strtoll, strtoul, strtoull, rand, srand, calloc, free, malloc, realloc,
// abort, atexit, exit, getenv, system, bsearch, qsort, abs, div, labs, ldiv, llabs, tlldiv, mblen, mbtowc, wctomb, mbstowcs, wcstombs2

const uint32_t WIDTH = 800;     // ����һ�����������ڶ��崰�ڵĿ��Ϊ800����
const uint32_t HEIGHT = 600;    // ����һ�����������ڶ��崰�ڵĸ߶�Ϊ600����
//uint32_t��int�����������ͣ�������������������
//  uint32_t���޷��ŵ�32λ������Ҳ����˵��ֻ�ܱ�ʾ0��4294967295֮��ķǸ�������
//  ����ÿһλ�����ڴ洢��ֵ��û�з���λ��
//  int���з��ŵ����������Ĵ�С�ڲ�ͬ��ƽ̨�ͱ������Ͽ���������ͬ����ͨ����32λ��64λ��
//  �����Ա�ʾ�������������㣬�������λ���ڱ�ʾ���ţ�0��ʾ������1��ʾ������
// 
//����������������ʹ��uint32_t��
//  ������Ҫȷ���������͵Ĵ�С�ͷ�Χ�ڲ�ͬ��ƽ̨��һ��ʱ�������ڿ�ƽ̨�ı�̻����ݴ����С�
//  ������Ҫ����һЩ�ض��Ķ����Ʋ������㷨ʱ������λ���㡢��ϣ���������ܵȡ�
//  ������Ҫ�����޷�������������ʱ�������޷��������ѭ����λ�ȡ�

class HelloTriangleApplication { // ����һ����Ķ��壬���ڷ�װ������Ӧ�ó�����߼�������
public: 
    void run() { 
        initWindow(); 
        initVulkan(); 
        mainLoop(); 
        cleanup(); 
    }

private: 
    GLFWwindow* window; 

    void initWindow() {                                 // ����һ��˽�еĳ�Ա�����Ķ��壬���ڳ�ʼ��GLFW�⣬������һ������
        glfwInit();                                     // ����һ������������䣬���ڳ�ʼ��GLFW��
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);   // ����һ������������䣬�������ô�����ʾΪ��ʹ���κοͻ���API����Vulkan��
        // glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); ���������Ǹ���GLFW�ⲻҪΪ���ڴ����κοͻ���API�������ģ�����OpenGL��OpenGL ES
        // ��������ԭ��������Ҫʹ��Vulkan API����Ⱦ���ڣ���Vulkan API����ҪGLFW���ṩ�������ģ�������Ҫ�����Լ�����һ��Vulkanʵ����һ��Vulkan���档
        // ��������֮�󣬲�����ζ�Ų���ʹ��Vulkan API��ǡǡ�෴����������֮�����ʹ��Vulkan API��
        // ������ǲ�������䣬��ôGLFW���Ĭ��Ϊ���ڴ���һ��OpenGL�����ģ������ͻ��Vulkan API��ͻ�������޷���ȷ��Ⱦ���ڡ�
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);     // ����һ������������䣬�������ô�����ʾΪ���ɵ�����С
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr); 
        // ����һ����ֵ��䣬���ڴ���һ�����ڣ�������ָ�븳ֵ��window����
        // 4:monitor�����ڵ�ȫ��ģʽ��ʹ�õ���ʾ�������ΪNULL�����ʾ�����Ǵ��ڻ��ġ�
        // 5:share�����ڹ��������Ķ������һ�����ڣ����ΪNULL�����ʾ������
    }

    void initVulkan() { // ����һ��˽�еĳ�Ա�����Ķ��壬���ڳ�ʼ��Vulkan�⣬������һЩVulkan����

    }

    void mainLoop() {   // ����һ��˽�еĳ�Ա�����Ķ��壬����ѭ����Ⱦ�����Σ����������¼�
        while (!glfwWindowShouldClose(window)) { // ����һ��ѭ����䣬�����жϴ����Ƿ�Ӧ�ùر�
            glfwPollEvents(); // ����һ������������䣬���ڴ������¼�
        }
    }

    void cleanup() { // ����һ��˽�еĳ�Ա�����Ķ��壬�����ͷ����г�ʼ��ʱ��������Դ������ֹGLFW��
        glfwDestroyWindow(window); // ����һ������������䣬�������ٴ��ڶ���
        glfwTerminate(); // ����һ������������䣬������ֹGLFW��
    }
};

int main() { // ���ǳ������ڵ㣬Ҳ���ǳ���ʼִ�еĵط�
    HelloTriangleApplication app; // ����һ��������Ӧ�ó�����󣬽��г�ʼ��

    try {           // ����һ���쳣�������Ŀ�ʼ�����ڳ�������������Ӧ�ó���
        app.run();  // ����һ������������䣬���ڵ���������Ӧ�ó�������run()����
    }
    catch (const std::exception& e) {       // ����һ���쳣�������Ľ��������ڲ�����ܷ������쳣
        std::cerr << e.what() << std::endl; // ����һ�������䣬���ڽ��쳣����Ϣ�������׼������
        return EXIT_FAILURE;                // ����һ��������䣬�����ڷ����쳣ʱ����һ��ʧ�ܵ�״̬�������ϵͳ
    }

    return EXIT_SUCCESS; // ����һ��������䣬��������������ʱ����һ���ɹ���״̬�������ϵͳ
}
