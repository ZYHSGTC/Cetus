#include "base/VulkanDevice.h"
#include <unordered_set>

namespace Cetus
{	

	VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice)
	{
		assert(physicalDevice);
		/* 宏展开后为：
		if (!(physicalDevice)) {
			fprintf(stderr, "Assertion failed: (%s), function %s, file %s, line %d.\n", "physicalDevice", __func__, __FILE__, __LINE__);
			abort();
		}这段代码的作用是检查physicalDevice是否为非空指针，如果是空指针，就向标准错误输出一条断言失败的信息，包括表达式、函数名、文件名和行号，然后调用abort()函数终止程序运行。*/

		this->physicalDevice = physicalDevice;

		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &features);
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		assert(queueFamilyCount > 0);
		queueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			std::vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			{
				for (const VkExtensionProperties& ext : extensions)
				{
					supportedExtensions.push_back(ext.extensionName);
				}
			}
		}
	}

	VulkanDevice::~VulkanDevice()
	{
		if (commandPool)
		{
			vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
		}
		if (logicalDevice)
		{
			vkDestroyDevice(logicalDevice, nullptr);
		}
	}

	uint32_t VulkanDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound) const
	{
		// 定义一个函数，接受三个参数：
		// typeBits是一个32位的整数，表示需要的内存类型的位掩码；
		// properties是一个枚举类型的标志，表示需要的内存属性，如可读、可写、可映射等；
		// memTypeFound是一个布尔类型的指针，用于返回是否找到匹配的内存类型
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)			// 用位运算和1进行与操作，判断当前的内存类型是否是需要的内存类型，如果是，就继续判断内存属性是否匹配
			{
				if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{								// 用位运算和properties进行与操作，判断当前的内存类型的属性是否包含了需要的属性，如果是，就返回找到匹配的内存类型的标志和索引
					if (memTypeFound)
					{
						*memTypeFound = true;	// 如果memTypeFound不是空指针，就把它指向的值设为true，表示找到了匹配的内存类型
					}
					return i;
				}
			}
			typeBits >>= 1;
		}
		// 如果循环结束后还没有返回，说明没有找到匹配的内存类型
		// 如果memTypeFound不是空指针，就把它指向的值设为false，表示没有找到匹配的内存类型，并返回0作为默认值
		if (memTypeFound)
		{
			*memTypeFound = false;
			return 0;
		}
		else
		{
			throw std::runtime_error("Could not find a matching memory type");
		}
	}

	uint32_t VulkanDevice::getQueueFamilyIndex(VkQueueFlags queueFlags) const
	{
		// 定义一个函数，接受一个参数：queueFlags是一个枚举类型的标志，表示需要的队列的特性，如图形、计算、传输等
		// 如果需要的队列只有计算特性，就优先寻找一个只有计算特性的队列族，这样可以避免和图形队列共享资源
		if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
			{
				// 用位运算和VK_QUEUE_COMPUTE_BIT进行与操作，判断当前的队列族是否有计算特性;
				// 然后用位运算和VK_QUEUE_GRAPHICS_BIT进行与操作，判断当前的队列族是否没有图形特性，如果都满足，就返回当前的队列族的索引
				if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					return i;
				}
			}
		}

		// 如果需要的队列只有传输特性，就优先寻找一个只有传输特性的队列族，这样可以避免和图形或计算队列共享资源
		if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
			{
				// 用位运算和VK_QUEUE_TRANSFER_BIT进行与操作，判断当前的队列族是否有传输特性;
				// 然后用位运算和VK_QUEUE_GRAPHICS_BIT和VK_QUEUE_COMPUTE_BIT进行与操作，判断当前的队列族是否没有图形或计算特性，如果都满足，就返回当前的队列族的索引
				if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					return i;
				}
			}
		}

		// 如果没有找到只有计算或传输特性的队列族，就寻找一个包含了所有需要的特性的队列族，这样可以满足最低的需求
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
		{
			if ((queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags)
			{
				return i;
			}
		}

		// 如果循环结束后还没有返回，说明没有找到匹配的队列族，就抛出一个运行时错误，表示无法找到匹配的队列族索引
		throw std::runtime_error("Could not find a matching queue family index");
	}

	VkResult VulkanDevice::createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char*> enabledDeviceExtensions
		, void* pNextChain, bool useSwapChain, VkQueueFlags requestedQueueTypes)
	{			
		// 定义一个函数，接受五个参数：
		// enabledFeatures是一个结构体，表示需要启用的物理设备特性，如几何着色器、多重采样等(光栅化)；
		// enabledExtensions是一个字符串的向量，表示需要启用的设备扩展，如表面、交换链等；
		// pNextChain是一个指针，表示需要附加到设备创建信息的pNext链，用于启用一些高级的特性或功能(光线追踪，几何细分)；
		// useSwapChain是一个布尔值，表示是否需要使用交换链，如果是，就会自动添加交换链扩展到设备扩展中；
		// requestedQueueTypes是一个枚举类型的标志，表示需要的队列的特性，如图形、计算、传输等
		 
		// 定义一个设备创建信息的结构体，用于指定创建逻辑设备的参数，如队列创建信息的数量和数组、启用的物理设备特性、启用的设备扩展等
		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		// 设备特征
		// 把this->enabledFeatures字段设为enabledFeatures，表示记录已经启用的物理设备特性
		this->enabledFeatures = enabledFeatures;
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};	// 定义一个物理设备特性2的结构体，用于指定启用的物理设备特性和pNext链
		// 如果pNextChain不是空指针，就把它赋值给physicalDeviceFeatures2的pNext字段，表示需要附加到设备创建信息的pNext链，用于启用一些高级的特性或功能。
		if (pNextChain) {
			physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			physicalDeviceFeatures2.features = enabledFeatures;
			physicalDeviceFeatures2.pNext = pNextChain;
			deviceCreateInfo.pEnabledFeatures = nullptr;		// 把deviceCreateInfo的pEnabledFeatures字段设为nullptr，表示不使用普通的物理设备特性结构体
			deviceCreateInfo.pNext = &physicalDeviceFeatures2;	// 把deviceCreateInfo的pNext字段设为physicalDeviceFeatures2的地址，表示使用物理设备特性2结构体
		}


		// 设备队列信息
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};// 定义一个队列创建信息的向量，用于存储需要创建的队列的参数
		// 定义一个默认的队列优先级，用于指定队列的调度优先级，范围是0.0到1.0，越大越高
		const float defaultQueuePriority(0.0f);
		// 如果需要的队列包含图形特性，就从物理设备的队列族中找到一个支持图形特性的队列族，并把它的索引存储到queueFamilyIndices.graphics中。
		// 然后创建一个队列创建信息，指定队列族索引、队列数量和队列优先级，并把它添加到队列创建信息的向量中
		if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
		{
			queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
		// 如果不需要图形特性，就把queueFamilyIndices.graphics设为0，表示没有图形队
		else
		{
			queueFamilyIndices.graphics = 0;
		}
		// 如果需要的队列包含计算特性，就从物理设备的队列族中找到一个支持计算特性的队列族，并把它的索引存储到queueFamilyIndices.compute中。
		// 如果这个队列族和图形队列族不同，就创建一个队列创建信息，指定队列族索引、队列数量和队列优先级，并把它添加到队列创建信息的向量中
		if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
		{
			queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
			if (queueFamilyIndices.compute != queueFamilyIndices.graphics)
			{
				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
		}
		// 如果不需要计算特性，就把queueFamilyIndices.compute设为queueFamilyIndices.graphics，表示使用图形队列作为计算队列
		else
		{
			queueFamilyIndices.compute = queueFamilyIndices.graphics;
		}
		// 如果需要的队列包含传输特性，就从物理设备的队列族中找到一个支持传输特性的队列族，并把它的索引存储到queueFamilyIndices.transfer中。
		// 如果这个队列族和图形或计算队列族都不同，就创建一个队列创建信息，指定队列族索引、队列数量和队列优先级，并把它添加到队列创建信息的向量中
		if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
		{
			queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
			if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) && (queueFamilyIndices.transfer != queueFamilyIndices.compute))
			{
				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
		}
		// 如果不需要传输特性，就把queueFamilyIndices.transfer设为queueFamilyIndices.graphics，表示使用图形队列作为传输队列
		else
		{
			queueFamilyIndices.transfer = queueFamilyIndices.graphics;
		}
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();


		// 设备扩展信息
		// 定义一个设备扩展的向量，用于存储需要启用的设备扩展，先把参数中的设备扩展复制到这个向量中
		// 如果需要使用交换链，就把交换链扩展的名称添加到设备扩展的向量中
		std::vector<const char*> deviceExtensions(enabledDeviceExtensions);
		if (useSwapChain)
		{
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}
		// 如果设备扩展的向量不为空，就遍历它，检查每个设备扩展是否被物理设备支持，如果不支持，就输出一条错误信息
		if (deviceExtensions.size() > 0)
		{
			for (const char* enabledExtension : deviceExtensions)
			{
				if (!extensionSupported(enabledExtension)) {
					std::cerr << "Enabled device extension \"" << enabledExtension << "\" is not present at device level\n";
				}
			}

			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}


		// 调用vkCreateDevice函数，传入物理设备、设备创建信息、分配器和逻辑设备的地址，创建一个逻辑设备，并把结果赋值给result变量，如果结果不是VK_SUCCESS，就返回结果
		VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
		if (result != VK_SUCCESS) 
		{
			return result;
		}

		// 调用createCommandPool函数，传入图形队列族的索引，创建一个命令池，并把结果赋值给commandPool字段，表示用于分配命令缓冲的命令池
		commandPool = createCommandPool(queueFamilyIndices.graphics);

		return result;
	}

	VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, 
		VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data)
	{	// 定义一个函数，接受六个参数：
		// usageFlags是一个枚举类型的标志，表示缓冲区的用途，如顶点缓冲、索引缓冲、一致缓冲等；
		// memoryPropertyFlags是一个枚举类型的标志，表示缓冲区的内存属性，如设备本地、主机可见、主机一致等；
		// size是一个64位的整数，表示缓冲区的大小，单位是字节；
		// buffer是一个指针，用于返回创建的缓冲区的句柄；
		// memory是一个指针，用于返回分配的缓冲区内存的句柄；
		// data是一个指针，表示需要复制到缓冲区的数据，如果为nullptr，表示不需要复制数据

		// 定义一个缓冲区创建信息的结构体，用于指定创建缓冲区的参数，如用途标志、大小等，调用Cetus::initializers::bufferCreateInfo函数，传入用途标志和大小，初始化缓冲区创建信息
		// 把缓冲区创建信息的共享模式字段设为VK_SHARING_MODE_EXCLUSIVE，表示缓冲区只能被一个队列族访问，不需要进行队列族所有权转移
		  // 调用vkCreateBuffer函数，传入逻辑设备、缓冲区创建信息、分配器和缓冲区的地址，创建一个缓冲区，并把结果赋值给VK_CHECK_RESULT宏，如果结果不是VK_SUCCESS，就返回结果
		VkBufferCreateInfo bufferCreateInfo = Cetus::initializers::bufferCreateInfo(usageFlags, size);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer));

		// 定义一个内存分配信息的结构体，用于指定分配缓冲区内存的参数，如大小、内存类型索引等，调用Cetus::initializers::memoryAllocateInfo函数，初始化内存分配信息
		VkMemoryAllocateInfo memAlloc = Cetus::initializers::memoryAllocateInfo();

		// 定义一个内存需求的结构体，用于获取缓冲区的内存需求，如大小、对齐、类型位等
		// 调用vkGetBufferMemoryRequirements函数，传入逻辑设备、缓冲区和内存需求的地址，获取缓冲区的内存需求，并把结果赋值给memReqs结构体
		// 把内存分配信息的大小字段设为内存需求的大小字段，表示分配的内存大小等于缓冲区的内存需求大小
		// 调用getMemoryType函数，传入内存需求的类型位字段、内存属性标志和内存类型索引的地址，根据内存属性标志从物理设备的内存类型中找到一个匹配的内存类型，并把它的索引赋值给内存分配信息的内存类型索引字段
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);

		// 定义一个内存分配标志信息的结构体，用于指定分配缓冲区内存的标志，如设备地址等
		// 如果缓冲区的用途标志包含VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT，表示缓冲区需要支持设备地址
		//	设备地址（device address）是一个64位的整数，用于唯一标识一个Vulkan对象，如缓冲区、图像、加速结构等。
		//	设备地址可以在着色器中使用，以便直接访问这些对象的数据，而不需要通过描述符或索引。
		//	设备地址的使用可以提高光线追踪的性能，因为可以避免额外的内存访问开销。
		// 就把内存分配标志信息的类型字段设为VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR，
		// 把内存分配标志信息的标志字段设为VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR，
		// 把内存分配信息的pNext字段设为内存分配标志信息的地址，表示附加到内存分配信息的pNext链
		VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
		if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
			allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
			allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
			memAlloc.pNext = &allocFlagsInfo;
		}
		VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));
			
		
		if (data != nullptr)			// 如果数据不是空指针，表示需要复制数据到缓冲区
		{
			void *mapped;				// 定义一个指针，用于映射缓冲区内存
			// 调用vkMapMemory函数，传入逻辑设备、缓冲区内存、偏移、大小、标志和映射指针的地址，映射缓冲区内存，并把结果赋值给VK_CHECK_RESULT宏，如果结果不是VK_SUCCESS，就返回结果
			VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
			memcpy(mapped, data, size);	// 调用memcpy函数，传入映射指针、数据指针和大小，把数据复制到映射的缓冲区内存中
			// 如果内存属性标志不包含VK_MEMORY_PROPERTY_HOST_COHERENT_BIT，它们不会自动保持主机端和设备端的数据一致，就需要手动刷新缓冲区内存，使得设备能够看到主机的写入
			if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				// 定义一个映射内存范围的结构体，用于指定需要刷新的缓冲区内存的范围，如内存、偏移、大小等，调用Cetus::initializers::mappedMemoryRange函数，初始化映射内存范围
				VkMappedMemoryRange mappedRange = Cetus::initializers::mappedMemoryRange();
				mappedRange.memory = *memory;		// 把映射内存范围的内存字段设为缓冲区内存，表示需要刷新的内存
				mappedRange.offset = 0;				// 把映射内存范围的偏移字段设为0，表示从缓冲区内存的起始位置开始刷新
				mappedRange.size = size;			// 把映射内存范围的大小字段设为缓冲区的大小，表示刷新整个缓冲区内存
				// 调用vkFlushMappedMemoryRanges函数，传入逻辑设备、映射内存范围的数量和数组，刷新缓冲区内存，并把结果赋值给VK_CHECK_RESULT宏，如果结果不是VK_SUCCESS，就返回结果
				// vkFlushMappedMemoryRanges的作用是保证主机端写入到非一致性内存的内容对于设备端可见，也就是说，它可以刷新主机端的缓存，使得设备端能够读取到最新的数据。
				// Flush是一个英语单词，表示刷新或冲洗的意思，在这里表示刷新主机端的缓存，使得设备端能够看到最新的数据。
				// Mapped是一个英语单词，表示映射的意思，在这里表示已经映射到主机端的内存。
				// Memory是一个英语单词，表示内存的意思，在这里表示需要刷新的内存。
				// Ranges是一个英语单词，表示范围的意思，在这里表示需要刷新的内存的范围。
				vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
			}
			vkUnmapMemory(logicalDevice, *memory);	// 调用vkUnmapMemory函数，传入逻辑设备和缓冲区内存，取消映射缓冲区内存
		}

		// 调用vkBindBufferMemory函数，传入逻辑设备、缓冲区、缓冲区内存和偏移，绑定缓冲区和缓冲区内存，并把结果赋值给VK_CHECK_RESULT宏，如果结果不是VK_SUCCESS，就返回结果
		VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

		return VK_SUCCESS;
	}

	VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, Cetus::Buffer *buffer, VkDeviceSize size, void *data)
	{
		// 设置缓冲区的设备为逻辑设备 buffer->device = logicalDevice;
		buffer->device = logicalDevice;

		// 创建缓冲区对象
		VkBufferCreateInfo bufferCreateInfo = Cetus::initializers::bufferCreateInfo(usageFlags, size);	//比上个函数缺少一个队列共享模式
		VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer->buffer));

		// 创建缓冲区内存
		VkMemoryAllocateInfo memAlloc = Cetus::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(logicalDevice, buffer->buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
		VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
		if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
			allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
			allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
			memAlloc.pNext = &allocFlagsInfo;
		}
		VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &buffer->memory));

		buffer->alignment = memReqs.alignment;				// 将缓冲区的对齐设为内存需求结构体的对齐
		buffer->size = size;								// 将缓冲区的大小设为传入的参数size
		buffer->usageFlags = usageFlags;					// 将缓冲区的用途标志设为传入的参数usageFlags
		buffer->memoryPropertyFlags = memoryPropertyFlags;	// 将缓冲区的内存属性标志设为传入的参数memoryPropertyFlags

		// 上传缓冲区数据
		if (data != nullptr)
		{
			VK_CHECK_RESULT(buffer->map());					// 调用buffer->map函数，将缓冲区的内存映射到主机内存，并将其赋值给buffer->mapped
			memcpy(buffer->mapped, data, size);				// 调用memcpy函数，将data指向的数据复制到buffer->mapped指向的内存中，复制的大小为size
			if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)// 如果缓冲区的内存属性标志不包含VK_MEMORY_PROPERTY_HOST_COHERENT_BIT，即内存不是主机一致的
				buffer->flush();							// 调用buffer->flush函数，将主机内存的数据刷新到设备内存中
			buffer->unmap();								// 调用buffer->unmap函数，取消缓冲区的内存映射
		}

		// 对象与内存绑定
		buffer->setupDescriptor();							// 调用buffer->setupDescriptor函数，设置缓冲区的描述符信息，包括缓冲区对象，偏移量和范围
		return buffer->bind();								// 调用buffer->bind函数，将缓冲区的内存绑定到缓冲区对象，并返回绑定结果
	}

	void VulkanDevice::copyBuffer(Cetus::Buffer *src, Cetus::Buffer *dst, VkQueue queue, VkBufferCopy *copyRegion)
	{	// 定义一个函数，接受四个参数：
		// src是一个指向源缓冲区的指针，
		// dst是一个指向目标缓冲区的指针，
		// queue是一个VkQueue对象，表示用于执行复制操作的队列，
		// copyRegion是一个指向VkBufferCopy结构体的指针，表示复制的源和目标区域，如果为nullptr，表示复制整个缓冲区
		assert(dst->size <= src->size);	// 断言目标缓冲区的大小不小于源缓冲区的大小
		assert(src->buffer);			// 断言源缓冲区的句柄不为空
		VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkBufferCopy bufferCopy{};		// 定义一个VkBufferCopy结构体，表示复制的源和目标区域
		if (copyRegion == nullptr)
		{
			bufferCopy.size = src->size;
		}
		else
		{
			bufferCopy = *copyRegion;
		}

		// 调用vkCmdCopyBuffer函数，传入命令缓冲区、源缓冲区、目标缓冲区、复制区域的数量（这里是1）和复制区域的数组（这里是bufferCopy的地址），在命令缓冲区中记录一个复制缓冲区的命令
		vkCmdCopyBuffer(copyCmd, src->buffer, dst->buffer, 1, &bufferCopy);

		// 调用flushCommandBuffer函数，传入命令缓冲区和队列，结束命令缓冲区的记录，提交命令缓冲区到队列，并等待队列完成操作
		flushCommandBuffer(copyCmd, queue);
	}

	VkCommandPool VulkanDevice::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
	{
		// 定义一个函数，接受两个参数：queueFamilyIndex是一个32位的整数，表示命令池所属的队列族索引，createFlags是一个枚举类型的标志，表示命令池的创建标志，返回一个VkCommandPool对象，表示创建的命令池
		// 定义一个命令池创建信息的结构体，用于指定创建命令池的参数，如队列族索引、创建标志等
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
		cmdPoolInfo.flags = createFlags;
		VkCommandPool cmdPool;
		VK_CHECK_RESULT(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool));
		return cmdPool;
	}

	VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
	{	// 定义一个函数，接受三个参数：
		// level是一个枚举类型的值，表示命令缓冲区的级别，可以是VK_COMMAND_BUFFER_LEVEL_PRIMARY或VK_COMMAND_BUFFER_LEVEL_SECONDARY，表示主命令缓冲区或辅助命令缓冲区，
		// pool是一个VkCommandPool对象，表示用于分配命令缓冲区的命令池，
		// begin是一个布尔值，表示是否开始记录命令，返回一个VkCommandBuffer对象，表示创建的命令缓冲区
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = Cetus::initializers::commandBufferAllocateInfo(pool, level, 1);
		VkCommandBuffer cmdBuffer;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));
		if (begin)
		{
			VkCommandBufferBeginInfo cmdBufInfo = Cetus::initializers::commandBufferBeginInfo();
			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
		}
		return cmdBuffer;
	}
			
	VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, bool begin)
	{
		return createCommandBuffer(level, commandPool, begin);
	}

	void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free)
	{
		// createCommandBuffer begin为true，开始记录缓冲，flushCommandBuffer结束记录并上传到队列上，并释放缓冲区；
		// 定义一个函数，接受四个参数：
		// commandBuffer是一个VkCommandBuffer对象，表示要结束、提交和释放的命令缓冲区，
		// queue是一个VkQueue对象，表示用于执行命令缓冲区的队列，
		// pool是一个VkCommandPool对象，表示用于分配命令缓冲区的命令池，
		// free是一个布尔值，表示是否释放命令缓冲区的内存
		// 如果命令缓冲区是空句柄，表示没有有效的命令缓冲区，就直接返回
		if (commandBuffer == VK_NULL_HANDLE)
		{
			return;
		}
		// 调用vkEndCommandBuffer函数，传入命令缓冲区，结束命令缓冲区的记录
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		// 定义一个队列提交信息的结构体，用于指定提交命令缓冲区的参数，如等待信号量、信号信号量、命令缓冲区数量和数组等，
		// 调用Cetus::initializers::submitInfo函数，初始化提交信息
		VkSubmitInfo submitInfo = Cetus::initializers::submitInfo();
		submitInfo.commandBufferCount = 1;				// 把提交信息的命令缓冲区数量字段设为1，表示只提交一个命令缓冲区
		submitInfo.pCommandBuffers = &commandBuffer;	// 把提交信息的命令缓冲区数组字段设为命令缓冲区的地址，表示要提交的命令缓冲区
		
		// 定义一个栅栏创建信息的结构体，用于指定创建栅栏的参数，如标志等，调用Cetus::initializers::fenceCreateInfo函数
		VkFenceCreateInfo fenceInfo = Cetus::initializers::fenceCreateInfo(VK_FLAGS_NONE);
		VkFence fence;									// 定义并下面创建一个VkFence对象
		VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
		
		// 调用vkQueueSubmit函数，传入队列、提交信息的数量（这里是1）、提交信息的数组（这里是submitInfo的地址）、栅栏，
		//	把命令缓冲区提交到队列
		// 调用vkWaitForFences函数，传入逻辑设备、栅栏的数量（这里是1）、栅栏的数组（这里是fence的地址）、是否等待所有栅栏、超时时间（这里是默认的超时时间），
		//	等待栅栏被触发，表示命令缓冲区命令全部传输到队列完成操作
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
		
		// 调用vkDestroyFence函数，传入逻辑设备、栅栏和分配器，销毁栅栏
		vkDestroyFence(logicalDevice, fence, nullptr);	
		if (free)
		{
			// 如果free参数为true，表示需要释放命令缓冲区的内存
			// 调用vkFreeCommandBuffers函数，传入逻辑设备、命令池、命令缓冲区的数量（这里是1）和命令缓冲区的数组（这里是commandBuffer的地址），
			//  释放命令缓冲区的内存
			vkFreeCommandBuffers(logicalDevice, pool, 1, &commandBuffer);
		}
	}

	void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
	{
		return flushCommandBuffer(commandBuffer, queue, commandPool, free);
	}

	bool VulkanDevice::extensionSupported(std::string extension)
	{
		return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end());
	}

	VkFormat VulkanDevice::getSupportedDepthFormat(bool checkSamplingSupport)
	{
		// 定义一个函数，接受一个参数：
		// checkSamplingSupport是一个布尔值，表示是否检查格式是否支持采样图像，返回一个VkFormat枚举值，表示支持的深度格式
		// 定义一个VkFormat枚举值的向量，表示候选的深度格式，按照优先级从高到低排列
		std::vector<VkFormat> depthFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
		for (auto& format : depthFormats)	// 遍历候选的深度格式
		{
			// 定义一个VkFormatProperties结构体，用于获取格式的属性，如线性平铺特性、最优平铺特性、缓冲特性等
			// 调用vkGetPhysicalDeviceFormatProperties函数，传入物理设备、格式和格式属性的地址，获取格式的属性，并赋值给formatProperties结构体
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
			// 如果格式的最优平铺特性包含VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT标志，表示格式支持深度模板附着
			if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				if (checkSamplingSupport) { // 如果检查采样支持，就进一步判断
					// 如果格式的最优平铺特性不包含VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT标志，表示格式不支持采样图像，就跳过这个格式，继续下一个循环
					if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
						continue;
					}
				}
				// 如果格式满足要求，就返回这个格式
				return format;
			}
		}
		// 如果遍历完所有候选的深度格式都没有找到合适的格式，就抛出一个运行时错误，表示没有找到匹配的深度格式
		throw std::runtime_error("Could not find a matching depth format");
	}

};
