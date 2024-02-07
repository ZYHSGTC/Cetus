#include "VulkanBase.h"

VulkanBase::VulkanBase(bool enableValidation)
{
	settings.validation = enableValidation;	// �����Ա����settings.validation����Ϊ����ֵ����ʾ�Ƿ�������֤��
	if (this->settings.validation)
	{	// ������Ա����settings.validation��ֵΪtrue����ʾ������֤��
		// ����setupConsole����������"Vulkan example"�����ÿ���̨�ı���
		setupConsole("Vulkan example");
	}
	setupDPIAwareness();
}

	void VulkanBase::setupConsole(std::string title)
	{
		// ����һ��������������һ��std::string���͵ı�������ʾ����̨�ı��⣬�޷���ֵ
		// ��ʾ���󴰿�
		AllocConsole();								// ����AllocConsole����������һ���µĿ���̨����
		AttachConsole(GetCurrentProcessId());		// ����AttachConsole����������GetCurrentProcessId�����ķ���ֵ������ǰ���̸��ӵ��µĿ���̨����
		FILE* stream;								// ����һ��FILE���͵�ָ�룬���ڲ����ļ���
		freopen_s(&stream, "CONIN$", "r", stdin);	// ����freopen_s����������stream�ĵ�ַ��"CONIN$"��"r"��stdin������׼�����ض��򵽿���̨������
		freopen_s(&stream, "CONOUT$", "w+", stdout);// ����freopen_s����������stream�ĵ�ַ��"CONOUT$"��"w+"��stdout������׼����ض��򵽿���̨�����
		freopen_s(&stream, "CONOUT$", "w+", stderr);// ����freopen_s����������stream�ĵ�ַ��"CONOUT$"��"w+"��stderr������׼�����ض��򵽿���̨�����
		SetConsoleTitle(TEXT(title.c_str()));		// ����SetConsoleTitle����������TEXT���title��c_str�����ķ���ֵ��������̨�ı�������Ϊtitle
	}

	void VulkanBase::setupDPIAwareness()
	{
		// DPI��֪��ָӦ�ó����ܹ������û���DPI���ã��Զ��������������ű����������ȣ�����Ӧ��ͬ����ʾ�豸�ͷֱ��ʡ�
		//	DPI��֪�������Ӧ�ó����ڸ�DPI�����µ���ۺ��û����飬�������ģ����ʧ�����õ����⡣
		// DPI��֪��ȫ����ÿӢ�������֪��Ӣ����dots per inch awareness��
		//  ÿӢ�������DPI����ָ��ʾ�豸��ÿӢ�糤�ȵ�������������ӳ����ʾ�豸�������Ⱥ�ϸ�ڳ̶ȡ�
		//  DPI��֪Ӧ�ó�����Ը��ݵ�ǰ��DPIֵ��ʹ�ú��ʵ����굥λ�������С��ͼ����Դ�ȣ�����������档

		// ����һ���������޲������޷���ֵ����������DPI��֪
		// ����һ�����ͱ�������ʾһ������ָ�룬ָ��һ������HRESULT * ���ͣ�����ΪPROCESS_DPI_AWARENESS���ͣ�����Լ��Ϊ__stdcall�ĺ���
		typedef HRESULT* (__stdcall* SetProcessDpiAwarenessFunc)(PROCESS_DPI_AWARENESS);

		// ����LoadLibraryA����������"Shcore.dll"������Shcore.dll��̬���ӿ⣬����ֵ��ֵ��shCore����
		HMODULE shCore = LoadLibraryA("Shcore.dll");
		if (shCore)	// ���shCore������ֵ��ΪNULL����ʾ���سɹ�
		{
			// ����GetProcAddress����������shCore������ֵ��"SetProcessDpiAwareness"����ȡSetProcessDpiAwareness�����ĵ�ַ��ת��ΪSetProcessDpiAwarenessFunc���͵ĺ���ָ�룬��ֵ��setProcessDpiAwareness����
			SetProcessDpiAwarenessFunc setProcessDpiAwareness =
				(SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwareness");

			if (setProcessDpiAwareness != nullptr)
			{
				// ���setProcessDpiAwareness������ֵ��Ϊnullptr����ʾ��ȡ�ɹ�
				// ����setProcessDpiAwareness����������PROCESS_PER_MONITOR_DPI_AWARE�����õ�ǰ����Ϊÿ��������DPI��֪
				setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
			}
			// ����FreeLibrary����������shCore������ֵ���ͷ�Shcore.dll��̬���ӿ�
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
	// ����vulkanʵ��
	err = createInstance(settings.validation);
	if (err) {
		Cetus::tools::exitFatal("Could not create Vulkan instance : \n" + Cetus::tools::errorString(err), err);
		return false;
	}


	// �������úõ��Թ���
	if (settings.validation)
	{
		Cetus::debug::setupDebugging(instance);
	}


	// ���������豸
	// ����豸��Ϣ
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

	// ѡ�������豸
	uint32_t selectedDevice = 0;
	physicalDevice = physicalDevices[selectedDevice];


	// ����vulkan�豸���߼��豸
	vulkanDevice = new Cetus::VulkanDevice(physicalDevice);
	getEnabledFeatures();
	getEnabledExtensions();
	VkResult res = vulkanDevice->createLogicalDevice(enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
	if (res != VK_SUCCESS) {
		Cetus::tools::exitFatal("Could not create Vulkan device: \n" + Cetus::tools::errorString(res), res);
		return false;
	}
	device = vulkanDevice->logicalDevice;


	// ����豸ͼ�ζ�������
	vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0, &queue);


	// ������ģ���ʽ
	VkBool32 validFormat{ false };
	if (settings.requiresStencil) {
		validFormat = Cetus::tools::getSupportedDepthStencilFormat(physicalDevice, &depthFormat);
	}
	else {
		validFormat = Cetus::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
	}
	assert(validFormat);


	// ���������ӵ��������豸��
	swapChain.connect(instance, physicalDevice, device);


	// �������д�ֱͬ����Ϣ
	VkSemaphoreCreateInfo semaphoreCreateInfo = Cetus::initializers::semaphoreCreateInfo();
	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete));
	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete));


	// ���ú�ͼ����л�����Ⱦ�ȴ���Ϣ
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

		// app��Ϣ
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = windowdata.name.c_str();
		appInfo.pEngineName = windowdata.name.c_str();
		appInfo.apiVersion = VK_API_VERSION_1_0;
		instanceCreateInfo.pApplicationInfo = &appInfo;

		// �����չ
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

		// �����֤��
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

		// ����vulkanʵ��
		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

		// �������Ա�ǩ
		if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
			Cetus::debugutils::setup(instance);
		}

		return result;
	}

	void VulkanBase::getEnabledFeatures() {}

	void VulkanBase::getEnabledExtensions() {}

HWND VulkanBase::setupWindow(HINSTANCE hinstance, WNDPROC wndproc)
{
	// ����һ��������������һ��HINSTANCE���͵ľ������ʾ���ڵ�ʵ����
	this->windowInstance = hinstance;

	WNDCLASSEX wndClass;
	wndClass.cbSize = sizeof(WNDCLASSEX);							// ���ô�����Ĵ�С������Ϊsizeof(WNDCLASSEX)
	wndClass.style = CS_HREDRAW | CS_VREDRAW;						// ���ô��������ʽ������ʹ��CS_HREDRAW��CS_VREDRAW����ʾ����ˮƽ��ֱ�ı��Сʱ�ػ�
	wndClass.lpfnWndProc = wndproc;									// ���ô�����Ĵ��ڹ��̣�����������Ϣ�ĺ���
	wndClass.cbClsExtra = 0;										// ���ô�����Ķ����ֽ���������Ϊ0
	wndClass.cbWndExtra = 0;										// ���ô���ʵ���Ķ����ֽ���������Ϊ0
	wndClass.hInstance = hinstance;									// ���ô������ʵ����������������ڹ��̵�ģ����
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);				// ���ô������ͼ����������ʹ��ϵͳĬ�ϵ�Ӧ�ó���ͼ��
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);					// ���ô�����Ĺ����������ʹ��ϵͳĬ�ϵļ�ͷ���
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);	// ���ô�����ı�����ˢ���������ʹ��ϵͳĬ�ϵĺ�ɫ��ˢ
	wndClass.lpszMenuName = NULL;									// ���ô�����Ĳ˵����ƣ�����ΪNULL����ʾ��ʹ�ò˵�
	wndClass.lpszClassName = windowdata.name.c_str();							// ���ô���������ƣ�����ʹ��windowdata.name������ֵ��windowdata.name��һ��std::string����
	wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);					// ���ô������Сͼ����������ʹ��ϵͳĬ�ϵ�Windows�ձ�ͼ��
	if (!RegisterClassEx(&wndClass))
	{
		std::cout << "Could not register window class!\n";	// ���һ��������Ϣ����ʾ�޷�ע�ᴰ�ڵ���
		fflush(stdout);										// ˢ�����������
		exit(1);											// ����exit����������1���˳�����
	}

	int windowWidth = GetSystemMetrics(SM_CXSCREEN);
	int windowHeight = GetSystemMetrics(SM_CYSCREEN);

	// �ı������ʾ�ʣ��Ǻ��ҵĴ��ڳ�����Ӧ
	if (settings.fullscreen)
	{
		if ((windowdata.width != (uint32_t)windowWidth) && (windowdata.height != windowHeight))
		{	// ������Ա����width��height��ֵ��������Ļ�Ŀ�Ⱥ͸߶ȣ���ʾ��Ҫ�ı������ʾ�ֱ���
			DEVMODE dmScreenSettings;								// ����һ��DEVMODE�ṹ�壬���ڴ洢��ʾ�豸����Ϣ
			memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));	// ����memset����������ṹ��ĵ�ַ��0�ͽṹ��Ĵ�С�����ṹ�����������
			dmScreenSettings.dmSize = sizeof(dmScreenSettings);		// ������ʾ���õĴ�С������Ϊsizeof(DEVMODE)
			dmScreenSettings.dmPelsWidth = windowdata.width;					// ������ʾ���õ�ˮƽ�ֱ��ʣ�����ʹ��width������ֵ��width��һ��uint32_t���͵ı���
			dmScreenSettings.dmPelsHeight = windowdata.height;					// ������ʾ���õĴ�ֱ�ֱ��ʣ�����ʹ��height������ֵ��height��һ��uint32_t���͵ı���
			dmScreenSettings.dmBitsPerPel = 32;						// ������ʾ���õ���ɫ��ȣ�����Ϊ32λ
			dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;// ������ʾ���õ���Ч�ֶΣ�����Ϊ�ֱ��ʺ���ɫ���
			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{	// ����ChangeDisplaySettings����������ṹ��ĵ�ַ��CDS_FULLSCREEN���ı���ʾ����Ϊȫ��ģʽ���������ֵ������DISP_CHANGE_SUCCESSFUL��˵��ȫ��ģʽ��֧��
				if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
				{	// ���������ʾ����ʧ�ܣ�������Ϣ��ѯ���û��Ƿ��л�������ģʽ
					settings.fullscreen = false;
				}
				else
				{
					return nullptr;
				}
			}
		}
	}

	DWORD dwExStyle = WS_EX_APPWINDOW;					// ��ʾ��������
	DWORD dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;	// ��ʾ�����ƴ��ڵ��Ӵ��ں�ͬ�����ڵ��ص�����
	if (settings.fullscreen) {
		dwExStyle |= WS_EX_TOPMOST;						// ��ʾ����ǰ��
		dwStyle |= WS_POPUP;							// ��������
	}
	else {
		dwExStyle |= WS_EX_WINDOWEDGE;					// �б�Ե
		dwStyle |= WS_OVERLAPPEDWINDOW;
	}
	RECT windowRect = { 0,0,(long)windowdata.width,(long)windowdata.height };	// ����һ�����ھ��νṹ����������溯�������޸�Ϊ���ڴ�С
	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

	// ����CreateWindowEx����������һ�����ڣ�����һ�����ھ��
	window = CreateWindowEx(
		dwExStyle,									// ��չ������ʽ������Ϊ0����ʾʹ��Ĭ����ʽ
		windowdata.name.c_str(),								// ���������ƣ�����ʹ��windowdata.name������ֵ
		getWindowTitle().c_str(),					// ���ڱ��⣬����ʹ��windowTitle������ֵ��windowTitle��һ��std::string����
		dwStyle,									// ������ʽ������ʹ��dwStyle������ֵ��
		0,											// ���ڵĳ�ʼˮƽλ�ã�����Ϊ0
		0,											// ���ڵĳ�ʼ��ֱλ�ã�����Ϊ0
		windowRect.right - windowRect.left,			// ���ڵĿ�ȣ�����ʹ��windowRect������ֵ����
		windowRect.bottom - windowRect.top,			// ���ڵĸ߶ȣ�����ʹ��windowRect������ֵ����
		NULL,										// ���ڵĸ����ڻ�ӵ���ߴ��ڵľ��������ΪNULL����ʾû�и����ڻ�ӵ���ߴ���
		NULL,										// ���ڵĲ˵�������Ӵ��ڱ�ʶ��������ΪNULL����ʾû�в˵����Ӵ���
		hinstance,									// ���ڵ�ʵ�����������ʹ��hinstance������ֵ
		NULL);										// ָ�򴫵ݸ����ڵ�ֵ��ָ�룬����ΪNULL����ʾ�������κ�ֵ
	if (!window)
	{
		printf("Could not create window!\n");
		fflush(stdout);
		return nullptr;
	}


	if (!settings.fullscreen)
	{	// ����ȫ���Ļ��Ѵ����Ƶ�������
		SetWindowPos(window,						// ���ڵľ��������ʹ��window������ֵ
			0,										// ������Z˳���е�λ�ã�����Ϊ0����ʾ���ı�Z˳��
			(windowWidth - windowRect.right) / 2,	// ���ڵ���ˮƽλ��
			(windowHeight - windowRect.bottom) / 2,	// ���ڵ��´�ֱλ��
			0,										// ���ڵ��¿�ȣ�����Ϊ0����ʾ���ı���
			0,										// ���ڵ��¸߶ȣ�����Ϊ0����ʾ���ı�߶�
			SWP_NOZORDER | SWP_NOSIZE);				// ���ڵĴ�С��λ�ñ�־������ʹ��SWP_NOZORDER��SWP_NOSIZE����ʾ���ı�Z˳��ʹ�С
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
	{	// �������棬ѡ��һ��֧����ʾ��ͼ�εĶ��У�ѡ����ɫ��ʽ
		swapChain.initSurface(windowInstance, window);
	}

	void VulkanBase::setupSwapChain()
	{	// ���´���������
		swapChain.create(&windowdata.width, &windowdata.height, settings.vsync, settings.fullscreen);
	}

	void VulkanBase::setupRenderPass() {
		// ����һ����������VkAttachmentDescription�ṹ������
		std::array<VkAttachmentDescription, 2> attachments = {};

		// ��һ��������������ɫ����
		attachments[0].format = swapChain.colorFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// ��������������ɫ��������
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// �洢������������ɫ��������
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// ������ģ����ز���
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// ������ģ��洢����
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// ͼ�񲼾�δ����
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// ����ͼ�񲼾�Ϊ���ڳ��ֵĲ���

		// �ڶ���������������ȸ���
		attachments[1].format = depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// ��������������ȸ�������
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// �洢������������ȸ�������
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;			// ������������ģ�帽������
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// ������ģ��洢����
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// ͼ�񲼾�δ����
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // ����ͼ�񲼾�Ϊ���ģ�帽�������Ų���

		// ��ɫ������VkAttachmentReference
		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;  // ��������
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;			// ͼ�񲼾�Ϊ��ɫ���������Ų���

		// ��ȸ�����VkAttachmentReference
		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;  // ��������
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // ͼ�񲼾�Ϊ���ģ�帽�������Ų���

		// ��ͨ������
		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;		// �󶨵�ͼ�ι���
		subpassDescription.colorAttachmentCount = 1;								// ��һ����ɫ����
		subpassDescription.pColorAttachments = &colorReference;						// ��ɫ��������
		subpassDescription.pDepthStencilAttachment = &depthReference;				// ���ģ�帽������
		subpassDescription.inputAttachmentCount = 0;								// �����븽��
		subpassDescription.pInputAttachments = nullptr;								// ���븽��Ϊ��
		subpassDescription.preserveAttachmentCount = 0;								// �ޱ�������
		subpassDescription.pPreserveAttachments = nullptr;							// ��������Ϊ��
		subpassDescription.pResolveAttachments = nullptr;							// ��������Ϊ��

		// ������ϵ����
		std::array<VkSubpassDependency, 2> dependencies;

		// ��һ��������ϵ�����ģ�帽����������ϵ
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;  // �ⲿ��ͨ��
		dependencies[0].dstSubpass = 0;  // Ŀ����ͨ��
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;  // ��ǰ������Ƭ�β��Խ׶�
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;  // ��ǰ������Ƭ�β��Խ׶�
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;  // ���ģ�帽��д�����
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;  // ���ģ�帽��д��Ͷ�ȡ����
		dependencies[0].dependencyFlags = 0;  // ������־Ϊ0

		// �ڶ���������ϵ����ɫ������������ϵ
		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;  // �ⲿ��ͨ��
		dependencies[1].dstSubpass = 0;  // Ŀ����ͨ��
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // ��ɫ��������׶�
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // ��ɫ��������׶�
		dependencies[1].srcAccessMask = 0;  // �޷�������
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;  // ��ɫ����д��Ͷ�ȡ����
		dependencies[1].dependencyFlags = 0;  // ������־Ϊ0

		// ��Ⱦͨ��������Ϣ
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;  // �ṹ����
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());  // ��������
		renderPassInfo.pAttachments = attachments.data();  // ��������
		renderPassInfo.subpassCount = 1;  // ��ͨ������
		renderPassInfo.pSubpasses = &subpassDescription;  // ��ͨ������
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());  // ������ϵ����
		renderPassInfo.pDependencies = dependencies.data();  // ������ϵ����

		// ������Ⱦͨ��
		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
	}

	void VulkanBase::createPipelineCache()
	{	// ����Pipeline Cache�����ڴ洢�����ù���״̬����
		// �������������Ϣ�Ľṹ��
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		// ����Pipeline Cache
		VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
	}

	void VulkanBase::setupDepthStencil()
	{
		VkImageCreateInfo imageCI{};						//����һ��VkImageCreateInfo�ṹ�壬����ָ��ͼ��Ĵ�������
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;//���ýṹ�������
		imageCI.imageType = VK_IMAGE_TYPE_2D;				//����ͼ�������Ϊ��άͼ��
		imageCI.format = depthFormat;						//����ͼ��ĸ�ʽΪ���ģ���ʽ������VK_FORMAT_D32_SFLOAT_S8_UINT
		imageCI.extent = { windowdata.width, windowdata.height, 1 };				//����ͼ��ĳߴ磬��ȣ��߶Ⱥ����
		imageCI.mipLevels = 1;								//����ͼ���mipmap�㼶Ϊ1����ֻ�л�����
		imageCI.arrayLayers = 1;							//����ͼ��������Ϊ1����ֻ��һ��ͼ��
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;			//����ͼ��Ĳ�����Ϊ1������ʹ�ö��ز���
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;			//����ͼ������з�ʽΪ���Ż������������������
		imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; //����ͼ�����;Ϊ���ģ�帽������������Ⱦ���ߵ���Ȳ��Ժ�ģ�����

		VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image)); //����vkCreateImage����������ָ���Ĳ�������һ��VkImage���󣬲����丳ֵ��depthStencil.image
		VkMemoryRequirements memReqs{};													//����һ��VkMemoryRequirements�ṹ�壬���ڻ�ȡͼ����ڴ�����
		vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);				//����vkGetImageMemoryRequirements����������ָ����ͼ���ȡ���ڴ����󣬲����丳ֵ��memReqs

		VkMemoryAllocateInfo memAllloc{};												//����һ��VkMemoryAllocateInfo�ṹ�壬����ָ���ڴ�ķ������
		memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;						//���ýṹ�������
		memAllloc.allocationSize = memReqs.size;										//�����ڴ�ķ����С������ͼ����ڴ������С
		memAllloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); //�����ڴ������������ͨ������vulkanDevice->getMemoryType����������ͼ����ڴ��������ͺ��ڴ����ԣ��豸���أ���ȡ���ʵ��ڴ���������
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAllloc, nullptr, &depthStencil.mem)); //����vkAllocateMemory����������ָ���Ĳ�������һ��VkDeviceMemory���󣬲����丳ֵ��depthStencil.mem
		VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0)); //����vkBindImageMemory��������ָ����ͼ����ڴ����һ��ƫ����Ϊ0

		VkImageViewCreateInfo imageViewCI{};							//����һ��VkImageViewCreateInfo�ṹ�壬����ָ��ͼ����ͼ�Ĵ�������
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;	//���ýṹ�������
		imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;					//����ͼ����ͼ������Ϊ��άͼ����ͼ
		imageViewCI.image = depthStencil.image;							//����ͼ����ͼ��ͼ��Ϊ֮ǰ������ͼ��
		imageViewCI.format = depthFormat;								//����ͼ����ͼ�ĸ�ʽΪ���ģ���ʽ����ͼ��ĸ�ʽһ��
		imageViewCI.subresourceRange.baseMipLevel = 0;					//����ͼ����ͼ������Դ��Χ�Ļ���mipmap�㼶Ϊ0����������
		imageViewCI.subresourceRange.levelCount = 1;					//����ͼ����ͼ������Դ��Χ��mipmap�㼶��Ϊ1����ֻ�л�����
		imageViewCI.subresourceRange.baseArrayLayer = 0;				//����ͼ����ͼ������Դ��Χ�Ļ��������Ϊ0������һ��ͼ��
		imageViewCI.subresourceRange.layerCount = 1;					//����ͼ����ͼ������Դ��Χ���������Ϊ1����ֻ��һ��ͼ��
		imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; //����ͼ����ͼ������Դ��Χ�ķ�������Ϊ��ȷ��棬��ֻ�����������
		if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {				//���Ϊ���ģ���ʽ����
			imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT; //�򽫷����������ģ�巽�棬��ͬʱ������Ⱥ�ģ������
		}
		VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &depthStencil.view)); //����vkCreateImageView����������ָ���Ĳ�������һ��VkImageView���󣬲����丳ֵ��depthStencil.view
	}

	void VulkanBase::setupFrameBuffer()
	{
		VkImageView attachments[2];							//����һ��VkImageView���飬����ָ��֡�������ĸ���

		attachments[1] = depthStencil.view;					//�����ģ����ͼ��ֵ������ĵڶ���Ԫ�أ������ģ�帽�������������е�֡������������ͬ��

		VkFramebufferCreateInfo frameBufferCreateInfo = {}; //����һ��VkFramebufferCreateInfo�ṹ�壬����ָ��֡�������Ĵ�������
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO; //���ýṹ�������
		frameBufferCreateInfo.pNext = NULL;					//���ýṹ�����һ����չָ��ΪNULL����û����չ
		frameBufferCreateInfo.renderPass = renderPass;		//����֡����������Ⱦͨ��Ϊ֮ǰ��������Ⱦͨ��
		frameBufferCreateInfo.attachmentCount = 2;			//����֡�������ĸ�������Ϊ2������ɫ���������ģ�帽��
		frameBufferCreateInfo.pAttachments = attachments;	//����֡�������ĸ���ָ��Ϊ֮ǰ����������
		frameBufferCreateInfo.width = windowdata.width;		//����֡�������Ŀ�ȣ��봰�ڵĿ��һ��
		frameBufferCreateInfo.height = windowdata.height;	//����֡�������ĸ߶ȣ��봰�ڵĸ߶�һ��
		frameBufferCreateInfo.layers = 1;					//����֡�������Ĳ���Ϊ1����ֻ��һ��

		frameBuffers.resize(swapChain.imageCount);			//����frameBuffers�����Ĵ�С���뽻������ͼ������һ��
		for (uint32_t i = 0; i < frameBuffers.size(); i++)	//����frameBuffers������ÿ��Ԫ��
		{
			attachments[0] = swapChain.buffers[i].view;		//���������ĵ�i��ͼ����ͼ��ֵ������ĵ�һ��Ԫ�أ�����ɫ����
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i])); //����vkCreateFramebuffer����������ָ���Ĳ�������һ��VkFramebuffer���󣬲����丳ֵ��frameBuffers�����ĵ�i��Ԫ��
		}
	}

	void VulkanBase::createCommandPool()
	{	// ����Vulkan����صĺ���
		// ����VkCommandPoolCreateInfo�ṹ�岢��ʼ��
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;			// ָ�������������
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;// ָ������صı�־�����������������

		// ʹ��Vulkan API���������
		VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
	}

	void VulkanBase::createCommandBuffers()
	{	// ����Vulkan��������ĺ���
		// ����������������Ĵ�С���뽻����ͼ������һ��
		// ��������ģʽ�Ľ�����������ͼ��Ҳ����������
		drawCmdBuffers.resize(swapChain.imageCount);

		// ����VkCommandBufferAllocateInfo�ṹ�壬ʹ���Զ���ĳ�ʼ�������������г�ʼ��
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			Cetus::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,				// ���������
				static_cast<uint32_t>(drawCmdBuffers.size()));	// ����������������

		// ʹ��Vulkan API�����������
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()));
	}

	void VulkanBase::createSynchronizationPrimitives() {	// ����Vulkanͬ��ԭ��ĺ���
		// ʹ���Զ����ʼ��������������VkFenceCreateInfo�ṹ�壬��ʼ״̬Ϊ�� signaled
		VkFenceCreateInfo fenceCreateInfo = Cetus::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

		// �����ȴ�Fence�������С��ȷ������������������������ƥ��
		waitFences.resize(drawCmdBuffers.size());

		// �����ȴ�Fence�����飬Ϊÿ��Fence����Vulkan Fence����
		for (auto& fence : waitFences) {
			// ʹ��Vulkan API����Fence
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
		}
	}

void VulkanBase::renderLoop()
{	// ��Ⱦѭ������
	lastTimestamp = std::chrono::high_resolution_clock::now();

	MSG msg;
	bool quitMessageReceived = false;
	while (!quitMessageReceived) {// ����Ⱦѭ��
		// ����Windows��Ϣ���У������û�����ȡ�
		// PeekMessage �Ĺ�����Ҫ������ѯ��Ϣ���У��Լ���Ƿ����µ���Ϣ������������������ִ�С�
		// ͨ�����˺������ڴ�������Ϣ������������롢����¼��ȡ�
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				quitMessageReceived = true;
				break;
			}
		}

		// ���׼���ò��Ҵ���û����С������ִ����һ֡����Ⱦ
		if (renderingAllowed && !IsIconic(window)) {
			nextFrame();
		}
	}

	// ˢ���豸��ȷ��������Դ���ܱ��ͷ�
	if (device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(device);
	}
}

	void VulkanBase::nextFrame(){	// ִ����һ֡��Ⱦ�ĺ���
	// ��¼��ʼʱ��
	auto tStart = std::chrono::high_resolution_clock::now();

	// �����ͼ�Ѹ��£��������ͼ�任
	if (cameraViewUpdated) {
		cameraViewUpdated = false;
		viewChanged();
	}

	// ִ����Ⱦ
	render();

	// ��¼��Ⱦ֡��������֡ʱ��
	FPSCounter++;
	auto tEnd = std::chrono::high_resolution_clock::now();

	// ����֡ʱ�䲢���������
	frameTimer = (float)std::chrono::duration<double, std::milli>(tEnd - tStart).count() / 1000.0f;
	camera.update(frameTimer);

	// ���������ƶ��ˣ�����ͼ�Ѹ���
	if (camera.moving()) {
		cameraViewUpdated = true;
	}

	// ����FPS�����´��ڱ���
	float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - lastTimestamp).count());
	if (fpsTimer > 1000.0f) {
		FPS = static_cast<uint32_t>((float)FPSCounter * (1000.0f / fpsTimer));
		SetWindowText(window, getWindowTitle().c_str());
		FPSCounter = 0;		// ����֡��
		lastTimestamp = tEnd;
	}

	updateOverlay();
}

	// �ϴ�UI�������Ϣ���Ա�UIOverlay::update���Ի��UI��صĶ��㣬�趨UI��λ��
	void VulkanBase::updateOverlay() {
		if (!settings.overlay)							// ����Ƿ������˸��ǣ�overlay��
			return;

		ImGuiIO& io = ImGui::GetIO();					// ��ȡ ImGui ������/�����IO������
		// ���� ImGui ����ʾ��С��֡���
		io.DisplaySize = ImVec2((float)windowdata.width, (float)windowdata.height);
		io.DeltaTime = frameTimer;

		// �������λ�úͰ���״̬
		io.MousePos = ImVec2(mousePos.x, mousePos.y);
		io.MouseDown[0] = mouseButtons.left && UIOverlay.visible;
		io.MouseDown[1] = mouseButtons.right && UIOverlay.visible;
		io.MouseDown[2] = mouseButtons.middle && UIOverlay.visible;

		// ��ʼ�µ� ImGui ֡
		ImGui::NewFrame();

		// ���� ImGui ������ʽ
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		// ��һ�д���ʹ�� ImGui ���� PushStyleVar ���޸� ImGui ���ڵ���ʽ������������˵���������ڵ� WindowRounding ��������Ϊ 0����ʾû��Բ�ǣ�ʹ�ô��ڵı߽Ǹ�Ϊֱ�ǡ�
		ImGui::SetNextWindowPos(ImVec2(10 * UIOverlay.scale, 10 * UIOverlay.scale));
		// ��һ�д���������һ�� ImGui ���ڵ�λ�á���ʹ�� SetNextWindowPos ����������һ�� ImVec2 ���͵Ĳ�������ʾ�������Ͻǵ�λ�á������λ�ñ�����Ϊ(10 * UIOverlay.scale, 10 * UIOverlay.scale)������ UIOverlay.scale ��һ���������ӣ��������ڵ�������λ�á�
		ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		// ��һ�д���������һ�� ImGui ���ڵĴ�С����ʹ�� SetNextWindowSize ����������һ�� ImVec2 ���͵Ĳ�������ʾ���ڵĴ�С���������С������Ϊ(0, 0)�����ʾ ImGui �����ݴ������ݵĴ�С�Զ��������ڵĴ�С��ImGuiCond_FirstUseEver ������ʾ�������ֻ���ڵ�һ��ʹ�ô���ʱ��Ч��
		
		// ����һ�� ImGui ����
		ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		//ImGui::Begin �������ڿ�ʼһ���µ� ImGui ���ڡ�����������������
		//	��һ�������Ǵ��ڵı��⣬��������Ϊ "Vulkan Example"��
		//  �ڶ��������Ǵ��ڵ� ID��ͨ��Ϊ nullptr ��ʾû���ض��� ID��
		//	�����������Ǵ��ڵı�־������ʹ����λ���뽫������־�����һ��
		//		ImGuiWindowFlags_AlwaysAutoResize��ʹ����ʼ���Զ�������С������Ӧ�����ݡ�
		//		ImGuiWindowFlags_NoResize�����ô��ڵĵ�����С���ܡ�
		//		ImGuiWindowFlags_NoMove�����ô��ڵ��ƶ����ܡ�
		ImGui::TextUnformatted("pipelines");
		// ImGui::TextUnformatted ���������� ImGui ��������ʾ�ı���������ʽ��������ʾ�� windowdata.name ���������ݣ��ñ������ܰ������ڵ����ơ�
		// ImGui::TextUnformatted(vulkanDevice->properties.deviceName);
		// ��������һ�У�������ʾ Vulkan �豸�����ƣ�ʹ���� vulkanDevice->properties.deviceName��
		ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / FPS), FPS);
		// ImGui::Text �������ڸ�ʽ������������� printf��������ʾ��ÿ֡��ʱ���֡����Ϣ��
		//	% .2f ms / frame��ÿ֡��ʱ�䣬������λС����
		//	% .1f fps��֡�ʣ�����һλС����
		//	(1000.0f / FPS)������֡�ʣ����� FPS ������֡�ʵı�����

		// ���� ImGui ��Ŀ���
		ImGui::PushItemWidth(110.0f * UIOverlay.scale);

		// ���� OnUpdateUIOverlay �������������û��Զ���� UI ���º���
		OnUpdateUIOverlay(&UIOverlay);

		// �ָ� ImGui ��Ŀ�������
		ImGui::PopItemWidth();

		// ���� ImGui ����
		ImGui::End();

		// �ָ� ImGui ������ʽ����
		ImGui::PopStyleVar();

		// ��Ⱦ ImGui ͼ��
		ImGui::Render();

		// ��� UIOverlay ��Ҫ���»����Ѿ����£������¹����������
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
		VK_CHECK_RESULT(vkQueueWaitIdle(queue));		// ȷ���ڻ�ȡ������ͼ��֮ǰ�����ͼ����Ϣ�Ѿ���ȫ����
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
		ValidateRect(window, NULL);	//  �������������ϵͳ��������ڲ���Ҫ�ػ��ˣ��Ͳ����յ� WM_PAINT ����Ϣ�ˡ�
		break;
	}
	// ������Ϣ
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
				 // ��갴����Ϣ
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
					 // ���������ƶ���Ϣ
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
					 // �����ڴ�С�ı����Ϣ
	case WM_SIZE: {
		// ����Ƿ���׼�����Ҵ���û����С��
		if ((renderingAllowed) && (wParam != SIZE_MINIMIZED)) {
			// ����Ƿ����ڵ������ڴ�С�����ߴ�������󻯻�ԭ
			if ((windowResizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED))) {
				// ��ȡ�µĴ��ڿ�Ⱥ͸߶�
				windowdata.width = LOWORD(lParam);
				windowdata.height = HIWORD(lParam);

				// ���ô��ڴ�С�ı�ʱ�Ĵ�����
				windowResize();
			}
		}
		break;
	}
				// �����ȡ��С/�����Ϣ����Ϣ
	case WM_GETMINMAXINFO: {
		// �� lParam ת��Ϊ LPMINMAXINFO ����ָ��
		LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;

		// ���ô��ڵ���С�ɵ�����С
		minMaxInfo->ptMinTrackSize.x = 64;
		minMaxInfo->ptMinTrackSize.y = 64;
		break;
	}
						 // �����ڽ��������С״̬����Ϣ
	case WM_ENTERSIZEMOVE: {
		// �� windowResizing ��־����Ϊ true����ʾ�������ڵ�����С
		windowResizing = true;
		break;
	}
						 // �������˳�������С״̬����Ϣ
	case WM_EXITSIZEMOVE: {
		// �� windowResizing ��־����Ϊ false����ʾ���ڲ��ٵ�����С
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
	{	// ���ڴ�С�ı�ʱ�Ĵ�����
		if (!renderingAllowed)				// �����δ׼���ã�ֱ�ӷ���
		{
			return;
		}
		renderingAllowed = false;			// ���Ϊδ׼����׼������������Ⱦ����


		vkDeviceWaitIdle(device);	// �ȴ��豸����
		setupSwapChain();			// �������ý�����

		// �������ģ�������Դ�������������ģ�建��
		vkDestroyImageView(device, depthStencil.view, nullptr);
		vkDestroyImage(device, depthStencil.image, nullptr);
		vkFreeMemory(device, depthStencil.mem, nullptr);
		setupDepthStencil();

		// ����֡����������������֡������
		for (uint32_t i = 0; i < frameBuffers.size(); i++) {
			vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
		}
		setupFrameBuffer();

		if ((windowdata.width > 0.0f) && (windowdata.height > 0.0f)) {
			if (settings.overlay) {
				UIOverlay.resize(windowdata.width, windowdata.height);
			}
		}

		// �����ƺ��ǿ��п��޵�
		destroyCommandBuffers();	// ����֮ǰ���������
		createCommandBuffers();		// �����µ��������
		recordCommandBuffers();		// ���������������
		for (auto& fence : waitFences) {
			vkDestroyFence(device, fence, nullptr);
		}							// ����֮ǰ��ͬ��ԭ������µ�ͬ��ԭ��
		createSynchronizationPrimitives();

		// �ȴ��豸����
		vkDeviceWaitIdle(device);

		// ������������ݺ�ȣ�ȷ����Ⱦ��ȷ
		if ((windowdata.width > 0.0f) && (windowdata.height > 0.0f)) {
			camera.updateAspectRatio((float)windowdata.width / (float)windowdata.height);
		}

		windowResized();			// ���ô��ڴ�С�ı�ʱ�Ļص�����
		viewChanged();				// ������ͼ�ı�ʱ�Ļص�����

		renderingAllowed = true;			// ���Ϊ��׼����
	}

	void VulkanBase::destroyCommandBuffers()
	{	// ������������ĺ���
		// �ͷ���������ڴ�
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
