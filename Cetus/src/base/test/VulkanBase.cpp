#include "VulkanBase.h"

VulkanBase::VulkanBase(bool enableValidation)
{
	settings.validation = enableValidation;	// 将类成员变量settings.validation设置为参数值，表示是否启用验证层
	if (this->settings.validation)
	{	// 如果类成员变量settings.validation的值为true，表示启用验证层
		// 调用setupConsole函数，传入"Vulkan example"，设置控制台的标题
		setupConsole("Vulkan example");
	}
	setupDPIAwareness();
}

	void VulkanBase::setupConsole(std::string title)
	{
		// 定义一个函数，参数是一个std::string类型的变量，表示控制台的标题，无返回值
		// 显示错误窗口
		AllocConsole();								// 调用AllocConsole函数，创建一个新的控制台窗口
		AttachConsole(GetCurrentProcessId());		// 调用AttachConsole函数，传入GetCurrentProcessId函数的返回值，将当前进程附加到新的控制台窗口
		FILE* stream;								// 定义一个FILE类型的指针，用于操作文件流
		freopen_s(&stream, "CONIN$", "r", stdin);	// 调用freopen_s函数，传入stream的地址、"CONIN$"、"r"和stdin，将标准输入重定向到控制台的输入
		freopen_s(&stream, "CONOUT$", "w+", stdout);// 调用freopen_s函数，传入stream的地址、"CONOUT$"、"w+"和stdout，将标准输出重定向到控制台的输出
		freopen_s(&stream, "CONOUT$", "w+", stderr);// 调用freopen_s函数，传入stream的地址、"CONOUT$"、"w+"和stderr，将标准错误重定向到控制台的输出
		SetConsoleTitle(TEXT(title.c_str()));		// 调用SetConsoleTitle函数，传入TEXT宏和title的c_str函数的返回值，将控制台的标题设置为title
	}

	void VulkanBase::setupDPIAwareness()
	{
		// DPI感知是指应用程序能够根据用户的DPI设置，自动调整其界面的缩放比例和清晰度，以适应不同的显示设备和分辨率。
		//	DPI感知可以提高应用程序在高DPI设置下的外观和用户体验，避免出现模糊、失真或剪裁的问题。
		// DPI感知的全称是每英寸点数感知，英文是dots per inch awareness。
		//  每英寸点数（DPI）是指显示设备上每英寸长度的像素数，它反映了显示设备的清晰度和细节程度。
		//  DPI感知应用程序可以根据当前的DPI值，使用合适的坐标单位、字体大小、图像资源等，来绘制其界面。

		// 定义一个函数，无参数，无返回值，用于设置DPI感知
		// 定义一个类型别名，表示一个函数指针，指向一个返回HRESULT * 类型，参数为PROCESS_DPI_AWARENESS类型，调用约定为__stdcall的函数
		typedef HRESULT* (__stdcall* SetProcessDpiAwarenessFunc)(PROCESS_DPI_AWARENESS);

		// 调用LoadLibraryA函数，传入"Shcore.dll"，加载Shcore.dll动态链接库，返回值赋值给shCore变量
		HMODULE shCore = LoadLibraryA("Shcore.dll");
		if (shCore)	// 如果shCore变量的值不为NULL，表示加载成功
		{
			// 调用GetProcAddress函数，传入shCore变量的值和"SetProcessDpiAwareness"，获取SetProcessDpiAwareness函数的地址，转换为SetProcessDpiAwarenessFunc类型的函数指针，赋值给setProcessDpiAwareness变量
			SetProcessDpiAwarenessFunc setProcessDpiAwareness =
				(SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwareness");

			if (setProcessDpiAwareness != nullptr)
			{
				// 如果setProcessDpiAwareness变量的值不为nullptr，表示获取成功
				// 调用setProcessDpiAwareness函数，传入PROCESS_PER_MONITOR_DPI_AWARE，设置当前进程为每个监视器DPI感知
				setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
			}
			// 调用FreeLibrary函数，传入shCore变量的值，释放Shcore.dll动态链接库
			FreeLibrary(shCore);
		}
	}

VulkanBase::~VulkanBase()
{
	// Clean up Vulkan resources
	swapChain.cleanup();
	if (descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	}
	destroyCommandBuffers();
	if (renderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(device, renderPass, nullptr);
	}
	for (uint32_t i = 0; i < frameBuffers.size(); i++)
	{
		vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
	}

	for (auto& shaderModule : shaderModules)
	{
		vkDestroyShaderModule(device, shaderModule, nullptr);
	}
	vkDestroyImageView(device, depthStencil.view, nullptr);
	vkDestroyImage(device, depthStencil.image, nullptr);
	vkFreeMemory(device, depthStencil.mem, nullptr);

	vkDestroyPipelineCache(device, pipelineCache, nullptr);

	vkDestroyCommandPool(device, cmdPool, nullptr);

	vkDestroySemaphore(device, semaphores.presentComplete, nullptr);
	vkDestroySemaphore(device, semaphores.renderComplete, nullptr);
	for (auto& fence : waitFences) {
		vkDestroyFence(device, fence, nullptr);
	}

	if (settings.overlay) {
		UIOverlay.freeResources();
	}

	delete vulkanDevice;

	if (settings.validation)
	{
		Cetus::debug::freeDebugCallback(instance);
	}

	vkDestroyInstance(instance, nullptr);

}

bool VulkanBase::initVulkan()
{
	VkResult err;
	// 创建vulkan实例
	err = createInstance(settings.validation);
	if (err) {
		Cetus::tools::exitFatal("Could not create Vulkan instance : \n" + Cetus::tools::errorString(err), err);
		return false;
	}


	// 创建设置好调试工具
	if (settings.validation)
	{
		Cetus::debug::setupDebugging(instance);
	}


	// 创建物理设备
	// 获得设备信息
	uint32_t gpuCount = 0;
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
	if (gpuCount == 0) {
		Cetus::tools::exitFatal("No device with Vulkan support found", -1);
		return false;
	}
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	err = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
	if (err) {
		Cetus::tools::exitFatal("Could not enumerate physical devices : \n" + Cetus::tools::errorString(err), err);
		return false;
	}

	// 选择物理设备
	uint32_t selectedDevice = 0;
	physicalDevice = physicalDevices[selectedDevice];


	// 创建vulkan设备与逻辑设备
	vulkanDevice = new Cetus::VulkanDevice(physicalDevice);
	getEnabledFeatures();
	getEnabledExtensions();
	VkResult res = vulkanDevice->createLogicalDevice(enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
	if (res != VK_SUCCESS) {
		Cetus::tools::exitFatal("Could not create Vulkan device: \n" + Cetus::tools::errorString(res), res);
		return false;
	}
	device = vulkanDevice->logicalDevice;


	// 获得设备图形队列索引
	vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0, &queue);


	// 获得深度模板格式
	VkBool32 validFormat{ false };
	if (settings.requiresStencil) {
		validFormat = Cetus::tools::getSupportedDepthStencilFormat(physicalDevice, &depthFormat);
	}
	else {
		validFormat = Cetus::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
	}
	assert(validFormat);


	// 交换链链接到创建的设备上
	swapChain.connect(instance, physicalDevice, device);


	// 创建队列垂直同步信息
	VkSemaphoreCreateInfo semaphoreCreateInfo = Cetus::initializers::semaphoreCreateInfo();
	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete));
	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete));


	// 设置好图像队列基础渲染等待信息
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo = Cetus::initializers::submitInfo();
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &semaphores.renderComplete;

	return true;
}

	VkResult VulkanBase::createInstance(bool enableValidation)
	{
		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = NULL;

		// app信息
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = windowdata.name.c_str();
		appInfo.pEngineName = windowdata.name.c_str();
		appInfo.apiVersion = VK_API_VERSION_1_0;
		instanceCreateInfo.pApplicationInfo = &appInfo;

		// 添加扩展
		std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };

		uint32_t extCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			std::vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			{
				for (const VkExtensionProperties& extension : extensions)
				{
					supportedInstanceExtensions.push_back(extension.extensionName);
				}
			}
		}
		if (enabledInstanceExtensions.size() > 0)
		{
			for (const char* enabledExtension : enabledInstanceExtensions)
			{
				if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enabledExtension) == supportedInstanceExtensions.end())
				{
					std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
				}
				instanceExtensions.push_back(enabledExtension);
			}
		}
		this->settings.validation = enableValidation;
		if (settings.validation || std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		if (instanceExtensions.size() > 0)
		{
			instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
			instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
		}

		// 添加验证层
		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
		if (settings.validation)
		{
			uint32_t instanceLayerCount;
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
			std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());

			bool validationLayerPresent = false;
			for (const VkLayerProperties& layer : instanceLayerProperties) {
				if (strcmp(layer.layerName, validationLayerName) == 0) {
					validationLayerPresent = true;
					break;
				}
			}
			if (validationLayerPresent) {
				instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
				instanceCreateInfo.enabledLayerCount = 1;
			}
			else {
				std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
			}
		}

		// 创建vulkan实例
		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

		// 开启调试标签
		if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
			Cetus::debugutils::setup(instance);
		}

		return result;
	}

	void VulkanBase::getEnabledFeatures() {}

	void VulkanBase::getEnabledExtensions() {}

HWND VulkanBase::setupWindow(HINSTANCE hinstance, WNDPROC wndproc)
{
	// 定义一个函数，参数是一个HINSTANCE类型的句柄，表示窗口的实例，
	this->windowInstance = hinstance;

	WNDCLASSEX wndClass;
	wndClass.cbSize = sizeof(WNDCLASSEX);							// 设置窗口类的大小，必须为sizeof(WNDCLASSEX)
	wndClass.style = CS_HREDRAW | CS_VREDRAW;						// 设置窗口类的样式，这里使用CS_HREDRAW和CS_VREDRAW，表示窗口水平或垂直改变大小时重绘
	wndClass.lpfnWndProc = wndproc;									// 设置窗口类的窗口过程，即处理窗口消息的函数
	wndClass.cbClsExtra = 0;										// 设置窗口类的额外字节数，这里为0
	wndClass.cbWndExtra = 0;										// 设置窗口实例的额外字节数，这里为0
	wndClass.hInstance = hinstance;									// 设置窗口类的实例句柄，即包含窗口过程的模块句柄
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);				// 设置窗口类的图标句柄，这里使用系统默认的应用程序图标
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);					// 设置窗口类的光标句柄，这里使用系统默认的箭头光标
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);	// 设置窗口类的背景画刷句柄，这里使用系统默认的黑色画刷
	wndClass.lpszMenuName = NULL;									// 设置窗口类的菜单名称，这里为NULL，表示不使用菜单
	wndClass.lpszClassName = windowdata.name.c_str();							// 设置窗口类的名称，这里使用windowdata.name变量的值，windowdata.name是一个std::string对象
	wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);					// 设置窗口类的小图标句柄，这里使用系统默认的Windows徽标图标
	if (!RegisterClassEx(&wndClass))
	{
		std::cout << "Could not register window class!\n";	// 输出一条错误信息，表示无法注册窗口的类
		fflush(stdout);										// 刷新输出缓冲区
		exit(1);											// 调用exit函数，传入1，退出程序
	}

	int windowWidth = GetSystemMetrics(SM_CXSCREEN);
	int windowHeight = GetSystemMetrics(SM_CYSCREEN);

	// 改变电脑显示率，是和我的窗口长宽适应
	if (settings.fullscreen)
	{
		if ((windowdata.width != (uint32_t)windowWidth) && (windowdata.height != windowHeight))
		{	// 如果类成员变量width和height的值不等于屏幕的宽度和高度，表示需要改变电脑显示分辨率
			DEVMODE dmScreenSettings;								// 创建一个DEVMODE结构体，用于存储显示设备的信息
			memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));	// 调用memset函数，传入结构体的地址、0和结构体的大小，将结构体的内容清零
			dmScreenSettings.dmSize = sizeof(dmScreenSettings);		// 设置显示设置的大小，必须为sizeof(DEVMODE)
			dmScreenSettings.dmPelsWidth = windowdata.width;					// 设置显示设置的水平分辨率，这里使用width变量的值，width是一个uint32_t类型的变量
			dmScreenSettings.dmPelsHeight = windowdata.height;					// 设置显示设置的垂直分辨率，这里使用height变量的值，height是一个uint32_t类型的变量
			dmScreenSettings.dmBitsPerPel = 32;						// 设置显示设置的颜色深度，这里为32位
			dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;// 设置显示设置的有效字段，这里为分辨率和颜色深度
			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{	// 调用ChangeDisplaySettings函数，传入结构体的地址和CDS_FULLSCREEN，改变显示设置为全屏模式，如果返回值不等于DISP_CHANGE_SUCCESSFUL，说明全屏模式不支持
				if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
				{	// 如果更改显示设置失败，弹出消息框询问用户是否切换到窗口模式
					settings.fullscreen = false;
				}
				else
				{
					return nullptr;
				}
			}
		}
	}

	DWORD dwExStyle = WS_EX_APPWINDOW;					// 显示在任务栏
	DWORD dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;	// 表示不绘制窗口的子窗口和同级窗口的重叠部分
	if (settings.fullscreen) {
		dwExStyle |= WS_EX_TOPMOST;						// 显示在最前面
		dwStyle |= WS_POPUP;							// 弹出窗口
	}
	else {
		dwExStyle |= WS_EX_WINDOWEDGE;					// 有边缘
		dwStyle |= WS_OVERLAPPEDWINDOW;
	}
	RECT windowRect = { 0,0,(long)windowdata.width,(long)windowdata.height };	// 定义一个窗口矩形结构体变量，下面函数把它修改为窗口大小
	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

	// 调用CreateWindowEx函数，创建一个窗口，返回一个窗口句柄
	window = CreateWindowEx(
		dwExStyle,									// 扩展窗口样式，这里为0，表示使用默认样式
		windowdata.name.c_str(),								// 窗口类名称，这里使用windowdata.name变量的值
		getWindowTitle().c_str(),					// 窗口标题，这里使用windowTitle变量的值，windowTitle是一个std::string对象
		dwStyle,									// 窗口样式，这里使用dwStyle变量的值，
		0,											// 窗口的初始水平位置，这里为0
		0,											// 窗口的初始垂直位置，这里为0
		windowRect.right - windowRect.left,			// 窗口的宽度，这里使用windowRect变量的值计算
		windowRect.bottom - windowRect.top,			// 窗口的高度，这里使用windowRect变量的值计算
		NULL,										// 窗口的父窗口或拥有者窗口的句柄，这里为NULL，表示没有父窗口或拥有者窗口
		NULL,										// 窗口的菜单句柄或子窗口标识符，这里为NULL，表示没有菜单或子窗口
		hinstance,									// 窗口的实例句柄，这里使用hinstance变量的值
		NULL);										// 指向传递给窗口的值的指针，这里为NULL，表示不传递任何值
	if (!window)
	{
		printf("Could not create window!\n");
		fflush(stdout);
		return nullptr;
	}


	if (!settings.fullscreen)
	{	// 不是全屏的话把窗口移到正中央
		SetWindowPos(window,						// 窗口的句柄，这里使用window变量的值
			0,										// 窗口在Z顺序中的位置，这里为0，表示不改变Z顺序
			(windowWidth - windowRect.right) / 2,	// 窗口的新水平位置
			(windowHeight - windowRect.bottom) / 2,	// 窗口的新垂直位置
			0,										// 窗口的新宽度，这里为0，表示不改变宽度
			0,										// 窗口的新高度，这里为0，表示不改变高度
			SWP_NOZORDER | SWP_NOSIZE);				// 窗口的大小和位置标志，这里使用SWP_NOZORDER和SWP_NOSIZE，表示不改变Z顺序和大小
	}

	ShowWindow(window, SW_SHOW);
	SetForegroundWindow(window);
	SetFocus(window);

	return window;
}

void VulkanBase::prepare()
{
	initSwapchain();
	setupSwapChain();
	setupRenderPass();
	createPipelineCache();
	setupDepthStencil();
	setupFrameBuffer();
	createCommandPool();
	createCommandBuffers();
	createSynchronizationPrimitives();
	if (settings.overlay) {
		UIOverlay.device = vulkanDevice;
		UIOverlay.queue = queue;
		UIOverlay.shaders = {
			loadShader("../Cetus/shaders/base/uioverlay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader("../Cetus/shaders/base/uioverlay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
		};
		UIOverlay.prepareResources();
		UIOverlay.preparePipeline(pipelineCache, renderPass, swapChain.colorFormat, depthFormat);
	}
}

	void VulkanBase::initSwapchain()
	{	// 创建表面，选择一个支持显示与图形的队列，选择颜色格式
		swapChain.initSurface(windowInstance, window);
	}

	void VulkanBase::setupSwapChain()
	{	// 更新创建交换链
		swapChain.create(&windowdata.width, &windowdata.height, settings.vsync, settings.fullscreen);
	}

	void VulkanBase::setupRenderPass() {
		// 创建一个包含两个VkAttachmentDescription结构的数组
		std::array<VkAttachmentDescription, 2> attachments = {};

		// 第一个附件描述：颜色附件
		attachments[0].format = swapChain.colorFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// 清除操作，清除颜色附件内容
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// 存储操作，保存颜色附件内容
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// 不关心模板加载操作
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// 不关心模板存储操作
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// 图像布局未定义
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// 最终图像布局为用于呈现的布局

		// 第二个附件描述：深度附件
		attachments[1].format = depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// 清除操作，清除深度附件内容
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// 存储操作，保存深度附件内容
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;			// 清除操作，清除模板附件内容
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// 不关心模板存储操作
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// 图像布局未定义
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // 最终图像布局为深度模板附件的最优布局

		// 颜色附件的VkAttachmentReference
		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;  // 附件索引
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;			// 图像布局为颜色附件的最优布局

		// 深度附件的VkAttachmentReference
		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;  // 附件索引
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // 图像布局为深度模板附件的最优布局

		// 子通道描述
		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;		// 绑定到图形管线
		subpassDescription.colorAttachmentCount = 1;								// 有一个颜色附件
		subpassDescription.pColorAttachments = &colorReference;						// 颜色附件引用
		subpassDescription.pDepthStencilAttachment = &depthReference;				// 深度模板附件引用
		subpassDescription.inputAttachmentCount = 0;								// 无输入附件
		subpassDescription.pInputAttachments = nullptr;								// 输入附件为空
		subpassDescription.preserveAttachmentCount = 0;								// 无保留附件
		subpassDescription.pPreserveAttachments = nullptr;							// 保留附件为空
		subpassDescription.pResolveAttachments = nullptr;							// 解析附件为空

		// 依赖关系数组
		std::array<VkSubpassDependency, 2> dependencies;

		// 第一个依赖关系：深度模板附件的依赖关系
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;  // 外部子通道
		dependencies[0].dstSubpass = 0;  // 目标子通道
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;  // 提前和晚期片段测试阶段
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;  // 提前和晚期片段测试阶段
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;  // 深度模板附件写入访问
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;  // 深度模板附件写入和读取访问
		dependencies[0].dependencyFlags = 0;  // 依赖标志为0

		// 第二个依赖关系：颜色附件的依赖关系
		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;  // 外部子通道
		dependencies[1].dstSubpass = 0;  // 目标子通道
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // 颜色附件输出阶段
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // 颜色附件输出阶段
		dependencies[1].srcAccessMask = 0;  // 无访问掩码
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;  // 颜色附件写入和读取访问
		dependencies[1].dependencyFlags = 0;  // 依赖标志为0

		// 渲染通道创建信息
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;  // 结构类型
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());  // 附件数量
		renderPassInfo.pAttachments = attachments.data();  // 附件数组
		renderPassInfo.subpassCount = 1;  // 子通道数量
		renderPassInfo.pSubpasses = &subpassDescription;  // 子通道描述
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());  // 依赖关系数量
		renderPassInfo.pDependencies = dependencies.data();  // 依赖关系数组

		// 创建渲染通道
		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
	}

	void VulkanBase::createPipelineCache()
	{	// 创建Pipeline Cache，用于存储和重用管线状态数据
		// 定义包含创建信息的结构体
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		// 创建Pipeline Cache
		VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
	}

	void VulkanBase::setupDepthStencil()
	{
		VkImageCreateInfo imageCI{};						//创建一个VkImageCreateInfo结构体，用于指定图像的创建参数
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;//设置结构体的类型
		imageCI.imageType = VK_IMAGE_TYPE_2D;				//设置图像的类型为二维图像
		imageCI.format = depthFormat;						//设置图像的格式为深度模板格式，例如VK_FORMAT_D32_SFLOAT_S8_UINT
		imageCI.extent = { windowdata.width, windowdata.height, 1 };				//设置图像的尺寸，宽度，高度和深度
		imageCI.mipLevels = 1;								//设置图像的mipmap层级为1，即只有基本层
		imageCI.arrayLayers = 1;							//设置图像的数组层为1，即只有一个图像
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;			//设置图像的采样数为1，即不使用多重采样
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;			//设置图像的排列方式为最优化，即由驱动程序决定
		imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; //设置图像的用途为深度模板附件，即用于渲染管线的深度测试和模板测试

		VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image)); //调用vkCreateImage函数，根据指定的参数创建一个VkImage对象，并将其赋值给depthStencil.image
		VkMemoryRequirements memReqs{};													//创建一个VkMemoryRequirements结构体，用于获取图像的内存需求
		vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);				//调用vkGetImageMemoryRequirements函数，根据指定的图像获取其内存需求，并将其赋值给memReqs

		VkMemoryAllocateInfo memAllloc{};												//创建一个VkMemoryAllocateInfo结构体，用于指定内存的分配参数
		memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;						//设置结构体的类型
		memAllloc.allocationSize = memReqs.size;										//设置内存的分配大小，等于图像的内存需求大小
		memAllloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); //设置内存的类型索引，通过调用vulkanDevice->getMemoryType函数，根据图像的内存需求类型和内存属性（设备本地）获取合适的内存类型索引
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAllloc, nullptr, &depthStencil.mem)); //调用vkAllocateMemory函数，根据指定的参数分配一块VkDeviceMemory对象，并将其赋值给depthStencil.mem
		VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0)); //调用vkBindImageMemory函数，将指定的图像和内存绑定在一起，偏移量为0

		VkImageViewCreateInfo imageViewCI{};							//创建一个VkImageViewCreateInfo结构体，用于指定图像视图的创建参数
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;	//设置结构体的类型
		imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;					//设置图像视图的类型为二维图像视图
		imageViewCI.image = depthStencil.image;							//设置图像视图的图像为之前创建的图像
		imageViewCI.format = depthFormat;								//设置图像视图的格式为深度模板格式，与图像的格式一致
		imageViewCI.subresourceRange.baseMipLevel = 0;					//设置图像视图的子资源范围的基本mipmap层级为0，即基本层
		imageViewCI.subresourceRange.levelCount = 1;					//设置图像视图的子资源范围的mipmap层级数为1，即只有基本层
		imageViewCI.subresourceRange.baseArrayLayer = 0;				//设置图像视图的子资源范围的基本数组层为0，即第一个图像
		imageViewCI.subresourceRange.layerCount = 1;					//设置图像视图的子资源范围的数组层数为1，即只有一个图像
		imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; //设置图像视图的子资源范围的方面掩码为深度方面，即只关心深度数据
		if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {				//如果为深度模板格式类型
			imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT; //则将方面掩码加上模板方面，即同时关心深度和模板数据
		}
		VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &depthStencil.view)); //调用vkCreateImageView函数，根据指定的参数创建一个VkImageView对象，并将其赋值给depthStencil.view
	}

	void VulkanBase::setupFrameBuffer()
	{
		VkImageView attachments[2];							//创建一个VkImageView数组，用于指定帧缓冲区的附件

		attachments[1] = depthStencil.view;					//将深度模板视图赋值给数组的第二个元素，即深度模板附件，它对于所有的帧缓冲区都是相同的

		VkFramebufferCreateInfo frameBufferCreateInfo = {}; //创建一个VkFramebufferCreateInfo结构体，用于指定帧缓冲区的创建参数
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO; //设置结构体的类型
		frameBufferCreateInfo.pNext = NULL;					//设置结构体的下一个扩展指针为NULL，即没有扩展
		frameBufferCreateInfo.renderPass = renderPass;		//设置帧缓冲区的渲染通道为之前创建的渲染通道
		frameBufferCreateInfo.attachmentCount = 2;			//设置帧缓冲区的附件数量为2，即颜色附件和深度模板附件
		frameBufferCreateInfo.pAttachments = attachments;	//设置帧缓冲区的附件指针为之前创建的数组
		frameBufferCreateInfo.width = windowdata.width;		//设置帧缓冲区的宽度，与窗口的宽度一致
		frameBufferCreateInfo.height = windowdata.height;	//设置帧缓冲区的高度，与窗口的高度一致
		frameBufferCreateInfo.layers = 1;					//设置帧缓冲区的层数为1，即只有一层

		frameBuffers.resize(swapChain.imageCount);			//调整frameBuffers向量的大小，与交换链的图像数量一致
		for (uint32_t i = 0; i < frameBuffers.size(); i++)	//遍历frameBuffers向量的每个元素
		{
			attachments[0] = swapChain.buffers[i].view;		//将交换链的第i个图像视图赋值给数组的第一个元素，即颜色附件
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i])); //调用vkCreateFramebuffer函数，根据指定的参数创建一个VkFramebuffer对象，并将其赋值给frameBuffers向量的第i个元素
		}
	}

	void VulkanBase::createCommandPool()
	{	// 创建Vulkan命令池的函数
		// 创建VkCommandPoolCreateInfo结构体并初始化
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;			// 指定队列族的索引
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;// 指定命令池的标志，允许重置命令缓冲区

		// 使用Vulkan API创建命令池
		VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
	}

	void VulkanBase::createCommandBuffers()
	{	// 创建Vulkan命令缓冲区的函数
		// 调整绘制命令缓冲区的大小，与交换链图像数量一致
		// 立即呈现模式的交换链有三个图像也就是三缓冲
		drawCmdBuffers.resize(swapChain.imageCount);

		// 创建VkCommandBufferAllocateInfo结构体，使用自定义的初始化辅助函数进行初始化
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			Cetus::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,				// 主命令缓冲区
				static_cast<uint32_t>(drawCmdBuffers.size()));	// 分配的命令缓冲区数量

		// 使用Vulkan API分配命令缓冲区
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()));
	}

	void VulkanBase::createSynchronizationPrimitives() {	// 创建Vulkan同步原语的函数
		// 使用自定义初始化辅助函数创建VkFenceCreateInfo结构体，初始状态为已 signaled
		VkFenceCreateInfo fenceCreateInfo = Cetus::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

		// 调整等待Fence的数组大小，确保它与绘制命令缓冲区的数量相匹配
		waitFences.resize(drawCmdBuffers.size());

		// 遍历等待Fence的数组，为每个Fence创建Vulkan Fence对象
		for (auto& fence : waitFences) {
			// 使用Vulkan API创建Fence
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
		}
	}

void VulkanBase::renderLoop()
{	// 渲染循环函数
	lastTimestamp = std::chrono::high_resolution_clock::now();

	MSG msg;
	bool quitMessageReceived = false;
	while (!quitMessageReceived) {// 主渲染循环
		// 处理Windows消息队列，处理用户输入等、
		// PeekMessage 的功能主要用于轮询消息队列，以检查是否有新的消息到达，而不会阻塞程序的执行。
		// 通常，此函数用于处理窗口消息，例如键盘输入、鼠标事件等。
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				quitMessageReceived = true;
				break;
			}
		}

		// 如果准备好并且窗口没有最小化，则执行下一帧的渲染
		if (renderingAllowed && !IsIconic(window)) {
			nextFrame();
		}
	}

	// 刷新设备，确保所有资源都能被释放
	if (device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(device);
	}
}

	void VulkanBase::nextFrame(){	// 执行下一帧渲染的函数
	// 记录开始时间
	auto tStart = std::chrono::high_resolution_clock::now();

	// 如果视图已更新，则进行视图变换
	if (cameraViewUpdated) {
		cameraViewUpdated = false;
		viewChanged();
	}

	// 执行渲染
	render();

	// 记录渲染帧数，计算帧时间
	FPSCounter++;
	auto tEnd = std::chrono::high_resolution_clock::now();

	// 计算帧时间并更新摄像机
	frameTimer = (float)std::chrono::duration<double, std::milli>(tEnd - tStart).count() / 1000.0f;
	camera.update(frameTimer);

	// 如果摄像机移动了，则视图已更新
	if (camera.moving()) {
		cameraViewUpdated = true;
	}

	// 计算FPS并更新窗口标题
	float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - lastTimestamp).count());
	if (fpsTimer > 1000.0f) {
		FPS = static_cast<uint32_t>((float)FPSCounter * (1000.0f / fpsTimer));
		SetWindowText(window, getWindowTitle().c_str());
		FPSCounter = 0;		// 重置帧数
		lastTimestamp = tEnd;
	}

	updateOverlay();
}

	// 上传UI的相关信息，以便UIOverlay::update可以获得UI相关的顶点，设定UI的位置
	void VulkanBase::updateOverlay() {
		if (!settings.overlay)							// 检查是否启用了覆盖（overlay）
			return;

		ImGuiIO& io = ImGui::GetIO();					// 获取 ImGui 的输入/输出（IO）对象
		// 设置 ImGui 的显示大小和帧间隔
		io.DisplaySize = ImVec2((float)windowdata.width, (float)windowdata.height);
		io.DeltaTime = frameTimer;

		// 设置鼠标位置和按键状态
		io.MousePos = ImVec2(mousePos.x, mousePos.y);
		io.MouseDown[0] = mouseButtons.left && UIOverlay.visible;
		io.MouseDown[1] = mouseButtons.right && UIOverlay.visible;
		io.MouseDown[2] = mouseButtons.middle && UIOverlay.visible;

		// 开始新的 ImGui 帧
		ImGui::NewFrame();

		// 配置 ImGui 窗口样式
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		// 这一行代码使用 ImGui 函数 PushStyleVar 来修改 ImGui 窗口的样式变量。具体来说，它将窗口的 WindowRounding 变量设置为 0，表示没有圆角，使得窗口的边角更为直角。
		ImGui::SetNextWindowPos(ImVec2(10 * UIOverlay.scale, 10 * UIOverlay.scale));
		// 这一行代码设置下一个 ImGui 窗口的位置。它使用 SetNextWindowPos 函数，传入一个 ImVec2 类型的参数，表示窗口左上角的位置。在这里，位置被设置为(10 * UIOverlay.scale, 10 * UIOverlay.scale)，其中 UIOverlay.scale 是一个缩放因子，可能用于调整窗口位置。
		ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		// 这一行代码设置下一个 ImGui 窗口的大小。它使用 SetNextWindowSize 函数，传入一个 ImVec2 类型的参数，表示窗口的大小。在这里，大小被设置为(0, 0)，这表示 ImGui 将根据窗口内容的大小自动调整窗口的大小。ImGuiCond_FirstUseEver 参数表示这个设置只会在第一次使用窗口时生效。
		
		// 创建一个 ImGui 窗口
		ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		//ImGui::Begin 函数用于开始一个新的 ImGui 窗口。它接受三个参数：
		//	第一个参数是窗口的标题，这里设置为 "Vulkan Example"。
		//  第二个参数是窗口的 ID，通常为 nullptr 表示没有特定的 ID。
		//	第三个参数是窗口的标志，这里使用了位掩码将三个标志组合在一起：
		//		ImGuiWindowFlags_AlwaysAutoResize：使窗口始终自动调整大小，以适应其内容。
		//		ImGuiWindowFlags_NoResize：禁用窗口的调整大小功能。
		//		ImGuiWindowFlags_NoMove：禁用窗口的移动功能。
		ImGui::TextUnformatted("pipelines");
		// ImGui::TextUnformatted 函数用于在 ImGui 窗口中显示文本，不带格式。这里显示了 windowdata.name 变量的内容，该变量可能包含窗口的名称。
		// ImGui::TextUnformatted(vulkanDevice->properties.deviceName);
		// 类似于上一行，这里显示 Vulkan 设备的名称，使用了 vulkanDevice->properties.deviceName。
		ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / FPS), FPS);
		// ImGui::Text 函数用于格式化输出，类似于 printf。这里显示了每帧的时间和帧率信息。
		//	% .2f ms / frame：每帧的时间，保留两位小数。
		//	% .1f fps：帧率，保留一位小数。
		//	(1000.0f / FPS)：计算帧率，其中 FPS 可能是帧率的变量。

		// 配置 ImGui 项目宽度
		ImGui::PushItemWidth(110.0f * UIOverlay.scale);

		// 调用 OnUpdateUIOverlay 函数，可能是用户自定义的 UI 更新函数
		OnUpdateUIOverlay(&UIOverlay);

		// 恢复 ImGui 项目宽度设置
		ImGui::PopItemWidth();

		// 结束 ImGui 窗口
		ImGui::End();

		// 恢复 ImGui 窗口样式设置
		ImGui::PopStyleVar();

		// 渲染 ImGui 图形
		ImGui::Render();

		// 如果 UIOverlay 需要更新或者已经更新，则重新构建命令缓冲区
		if (UIOverlay.update() || UIOverlay.updated) {
			recordCommandBuffers();
			UIOverlay.updated = false;
		}
	}

	void VulkanBase::OnUpdateUIOverlay(Cetus::UIOverlay* overlay) {}

	void VulkanBase::prepareFrame()
	{
		VkResult result = swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer);
		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
			if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				windowResize();
			}
			return;
		}
		else {
			VK_CHECK_RESULT(result);
		}
	}

	void VulkanBase::submitFrame()
	{
		VkResult result = swapChain.queuePresent(queue, currentBuffer, semaphores.renderComplete);
		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
			windowResize();
			if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				return;
			}
		}
		else {
			VK_CHECK_RESULT(result);
		}
		VK_CHECK_RESULT(vkQueueWaitIdle(queue));		// 确保在获取交换链图像之前，这个图像信息已经完全呈现
	}

void VulkanBase::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE: {
		renderingAllowed = false;
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	}
	case WM_PAINT: {
		ValidateRect(window, NULL);	//  这个函数来告诉系统，这个窗口不需要重画了，就不会收到 WM_PAINT 的消息了。
		break;
	}
	// 键盘消息
	case WM_KEYDOWN: {
		switch (wParam)
		{
		case KEY_F1:{
			UIOverlay.visible = !UIOverlay.visible;
			UIOverlay.updated = true;
			break; 
		}
		case KEY_ESCAPE:
			PostQuitMessage(0);
			break;
		}
		if (camera.type == Camera::firstperson)
		{
			switch (wParam)
			{
			case KEY_W:
				camera.keys.up = true;
				break;
			case KEY_S:
				camera.keys.down = true;
				break;
			case KEY_A:
				camera.keys.left = true;
				break;
			case KEY_D:
				camera.keys.right = true;
				break;
			}
		}

		keyPressed((uint32_t)wParam);
		break;
	}
	case WM_KEYUP: {
		if (camera.type == Camera::firstperson)
		{
			switch (wParam)
			{
			case KEY_W:
				camera.keys.up = false;
				break;
			case KEY_S:
				camera.keys.down = false;
				break;
			case KEY_A:
				camera.keys.left = false;
				break;
			case KEY_D:
				camera.keys.right = false;
				break;
			}
		}
		break;
	}
				 // 鼠标按键消息
	case WM_LBUTTONDOWN: {
		mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		mouseButtons.left = true;
		break;
	}
	case WM_RBUTTONDOWN: {
		mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		mouseButtons.right = true;
		break;
	}
	case WM_MBUTTONDOWN: {
		mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		mouseButtons.middle = true;
		break;
	}
	case WM_LBUTTONUP: {
		mouseButtons.left = false;
		break;
	}
	case WM_RBUTTONUP: {
		mouseButtons.right = false;
		break;
	}
	case WM_MBUTTONUP: {
		mouseButtons.middle = false;
		break;
	}
					 // 鼠标滚轮与移动消息
	case WM_MOUSEWHEEL: {
		short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		camera.translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f));
		cameraViewUpdated = true;
		break;
	}
	case WM_MOUSEMOVE: {
		handleMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	}
					 // 处理窗口大小改变的消息
	case WM_SIZE: {
		// 检查是否已准备好且窗口没有最小化
		if ((renderingAllowed) && (wParam != SIZE_MINIMIZED)) {
			// 检查是否正在调整窗口大小，或者窗口已最大化或还原
			if ((windowResizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED))) {
				// 获取新的窗口宽度和高度
				windowdata.width = LOWORD(lParam);
				windowdata.height = HIWORD(lParam);

				// 调用窗口大小改变时的处理函数
				windowResize();
			}
		}
		break;
	}
				// 处理获取最小/最大信息的消息
	case WM_GETMINMAXINFO: {
		// 将 lParam 转换为 LPMINMAXINFO 类型指针
		LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;

		// 设置窗口的最小可调整大小
		minMaxInfo->ptMinTrackSize.x = 64;
		minMaxInfo->ptMinTrackSize.y = 64;
		break;
	}
						 // 处理窗口进入调整大小状态的消息
	case WM_ENTERSIZEMOVE: {
		// 将 windowResizing 标志设置为 true，表示窗口正在调整大小
		windowResizing = true;
		break;
	}
						 // 处理窗口退出调整大小状态的消息
	case WM_EXITSIZEMOVE: {
		// 将 windowResizing 标志设置为 false，表示窗口不再调整大小
		windowResizing = false;
		break;
	}
	}

	OnHandleMessage(hWnd, uMsg, wParam, lParam);
}

	void VulkanBase::keyPressed(uint32_t) {}

	void VulkanBase::handleMouseMove(int32_t x, int32_t y)
	{
		int32_t dx = (int32_t)mousePos.x - x;
		int32_t dy = (int32_t)mousePos.y - y;

		bool handled = false;
		
		if (settings.overlay) {
			ImGuiIO& io = ImGui::GetIO();
			handled = io.WantCaptureMouse && UIOverlay.visible;
		}
		mouseMoved((float)x, (float)y, handled);

		if (handled) {
			mousePos = glm::vec2((float)x, (float)y);
			return;
		}

		if (mouseButtons.left) {
			camera.rotate(glm::vec3(dy * camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));
			cameraViewUpdated = true;
		}
		if (mouseButtons.right) {
			camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f));
			cameraViewUpdated = true;
		}
		if (mouseButtons.middle) {
			camera.translate(glm::vec3(-dx * 0.005f, -dy * 0.005f, 0.0f));
			cameraViewUpdated = true;
		}
		mousePos = glm::vec2((float)x, (float)y);
	}

	void VulkanBase::mouseMoved(double x, double y, bool& handled) {}

	void VulkanBase::windowResize()
	{	// 窗口大小改变时的处理函数
		if (!renderingAllowed)				// 如果尚未准备好，直接返回
		{
			return;
		}
		renderingAllowed = false;			// 标记为未准备，准备重新设置渲染环境


		vkDeviceWaitIdle(device);	// 等待设备空闲
		setupSwapChain();			// 重新设置交换链

		// 销毁深度模板相关资源，重新设置深度模板缓冲
		vkDestroyImageView(device, depthStencil.view, nullptr);
		vkDestroyImage(device, depthStencil.image, nullptr);
		vkFreeMemory(device, depthStencil.mem, nullptr);
		setupDepthStencil();

		// 销毁帧缓冲区，重新设置帧缓冲区
		for (uint32_t i = 0; i < frameBuffers.size(); i++) {
			vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
		}
		setupFrameBuffer();

		if ((windowdata.width > 0.0f) && (windowdata.height > 0.0f)) {
			if (settings.overlay) {
				UIOverlay.resize(windowdata.width, windowdata.height);
			}
		}

		// 下面似乎是可有可无的
		destroyCommandBuffers();	// 销毁之前的命令缓冲区
		createCommandBuffers();		// 创建新的命令缓冲区
		recordCommandBuffers();		// 构建命令缓冲区内容
		for (auto& fence : waitFences) {
			vkDestroyFence(device, fence, nullptr);
		}							// 销毁之前的同步原语，创建新的同步原语
		createSynchronizationPrimitives();

		// 等待设备空闲
		vkDeviceWaitIdle(device);

		// 更新摄像机的纵横比，确保渲染正确
		if ((windowdata.width > 0.0f) && (windowdata.height > 0.0f)) {
			camera.updateAspectRatio((float)windowdata.width / (float)windowdata.height);
		}

		windowResized();			// 调用窗口大小改变时的回调函数
		viewChanged();				// 调用视图改变时的回调函数

		renderingAllowed = true;			// 标记为已准备好
	}

	void VulkanBase::destroyCommandBuffers()
	{	// 销毁命令缓冲区的函数
		// 释放命令缓冲区内存
		vkFreeCommandBuffers(device, cmdPool, static_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());
	}

	void VulkanBase::recordCommandBuffers() {}

	void VulkanBase::windowResized() {}

	void VulkanBase::viewChanged() {}

	void VulkanBase::OnHandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {};

void VulkanBase::drawUI(const VkCommandBuffer commandBuffer)
{
	if (settings.overlay && UIOverlay.visible) {
		const VkViewport viewport = Cetus::initializers::viewport((float)windowdata.width, (float)windowdata.height, 0.0f, 1.0f);
		const VkRect2D scissor = Cetus::initializers::rect2D(windowdata.width, windowdata.height, 0, 0);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		UIOverlay.draw(commandBuffer);
	}
}

std::string VulkanBase::getWindowTitle()
{
	std::string device(vulkanDevice->properties.deviceName);
	std::string windowTitle;
	windowTitle = windowdata.name + " - " + device;
	windowTitle += " - " + std::to_string(FPS) + " fps";
	return windowTitle;
}

VkPipelineShaderStageCreateInfo VulkanBase::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = Cetus::tools::loadShader(fileName, device);
	shaderStage.pName = "main";
	assert(shaderStage.module != VK_NULL_HANDLE);
	shaderModules.push_back(shaderStage.module);
	return shaderStage;
}
