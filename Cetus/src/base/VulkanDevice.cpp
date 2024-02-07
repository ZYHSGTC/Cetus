#include "base/VulkanDevice.h"
#include <unordered_set>

namespace Cetus
{	

	VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice)
	{
		assert(physicalDevice);
		/* ��չ����Ϊ��
		if (!(physicalDevice)) {
			fprintf(stderr, "Assertion failed: (%s), function %s, file %s, line %d.\n", "physicalDevice", __func__, __FILE__, __LINE__);
			abort();
		}��δ���������Ǽ��physicalDevice�Ƿ�Ϊ�ǿ�ָ�룬����ǿ�ָ�룬�����׼�������һ������ʧ�ܵ���Ϣ���������ʽ�����������ļ������кţ�Ȼ�����abort()������ֹ�������С�*/

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
		// ����һ����������������������
		// typeBits��һ��32λ����������ʾ��Ҫ���ڴ����͵�λ���룻
		// properties��һ��ö�����͵ı�־����ʾ��Ҫ���ڴ����ԣ���ɶ�����д����ӳ��ȣ�
		// memTypeFound��һ���������͵�ָ�룬���ڷ����Ƿ��ҵ�ƥ����ڴ�����
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)			// ��λ�����1������������жϵ�ǰ���ڴ������Ƿ�����Ҫ���ڴ����ͣ�����ǣ��ͼ����ж��ڴ������Ƿ�ƥ��
			{
				if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{								// ��λ�����properties������������жϵ�ǰ���ڴ����͵������Ƿ��������Ҫ�����ԣ�����ǣ��ͷ����ҵ�ƥ����ڴ����͵ı�־������
					if (memTypeFound)
					{
						*memTypeFound = true;	// ���memTypeFound���ǿ�ָ�룬�Ͱ���ָ���ֵ��Ϊtrue����ʾ�ҵ���ƥ����ڴ�����
					}
					return i;
				}
			}
			typeBits >>= 1;
		}
		// ���ѭ��������û�з��أ�˵��û���ҵ�ƥ����ڴ�����
		// ���memTypeFound���ǿ�ָ�룬�Ͱ���ָ���ֵ��Ϊfalse����ʾû���ҵ�ƥ����ڴ����ͣ�������0��ΪĬ��ֵ
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
		// ����һ������������һ��������queueFlags��һ��ö�����͵ı�־����ʾ��Ҫ�Ķ��е����ԣ���ͼ�Ρ����㡢�����
		// �����Ҫ�Ķ���ֻ�м������ԣ�������Ѱ��һ��ֻ�м������ԵĶ����壬�������Ա����ͼ�ζ��й�����Դ
		if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
			{
				// ��λ�����VK_QUEUE_COMPUTE_BIT������������жϵ�ǰ�Ķ������Ƿ��м�������;
				// Ȼ����λ�����VK_QUEUE_GRAPHICS_BIT������������жϵ�ǰ�Ķ������Ƿ�û��ͼ�����ԣ���������㣬�ͷ��ص�ǰ�Ķ����������
				if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					return i;
				}
			}
		}

		// �����Ҫ�Ķ���ֻ�д������ԣ�������Ѱ��һ��ֻ�д������ԵĶ����壬�������Ա����ͼ�λ������й�����Դ
		if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
			{
				// ��λ�����VK_QUEUE_TRANSFER_BIT������������жϵ�ǰ�Ķ������Ƿ��д�������;
				// Ȼ����λ�����VK_QUEUE_GRAPHICS_BIT��VK_QUEUE_COMPUTE_BIT������������жϵ�ǰ�Ķ������Ƿ�û��ͼ�λ�������ԣ���������㣬�ͷ��ص�ǰ�Ķ����������
				if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					return i;
				}
			}
		}

		// ���û���ҵ�ֻ�м���������ԵĶ����壬��Ѱ��һ��������������Ҫ�����ԵĶ����壬��������������͵�����
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
		{
			if ((queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags)
			{
				return i;
			}
		}

		// ���ѭ��������û�з��أ�˵��û���ҵ�ƥ��Ķ����壬���׳�һ������ʱ���󣬱�ʾ�޷��ҵ�ƥ��Ķ���������
		throw std::runtime_error("Could not find a matching queue family index");
	}

	VkResult VulkanDevice::createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char*> enabledDeviceExtensions
		, void* pNextChain, bool useSwapChain, VkQueueFlags requestedQueueTypes)
	{			
		// ����һ���������������������
		// enabledFeatures��һ���ṹ�壬��ʾ��Ҫ���õ������豸���ԣ��缸����ɫ�������ز�����(��դ��)��
		// enabledExtensions��һ���ַ�������������ʾ��Ҫ���õ��豸��չ������桢�������ȣ�
		// pNextChain��һ��ָ�룬��ʾ��Ҫ���ӵ��豸������Ϣ��pNext������������һЩ�߼������Ի���(����׷�٣�����ϸ��)��
		// useSwapChain��һ������ֵ����ʾ�Ƿ���Ҫʹ�ý�����������ǣ��ͻ��Զ���ӽ�������չ���豸��չ�У�
		// requestedQueueTypes��һ��ö�����͵ı�־����ʾ��Ҫ�Ķ��е����ԣ���ͼ�Ρ����㡢�����
		 
		// ����һ���豸������Ϣ�Ľṹ�壬����ָ�������߼��豸�Ĳ���������д�����Ϣ�����������顢���õ������豸���ԡ����õ��豸��չ��
		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		// �豸����
		// ��this->enabledFeatures�ֶ���ΪenabledFeatures����ʾ��¼�Ѿ����õ������豸����
		this->enabledFeatures = enabledFeatures;
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};	// ����һ�������豸����2�Ľṹ�壬����ָ�����õ������豸���Ժ�pNext��
		// ���pNextChain���ǿ�ָ�룬�Ͱ�����ֵ��physicalDeviceFeatures2��pNext�ֶΣ���ʾ��Ҫ���ӵ��豸������Ϣ��pNext������������һЩ�߼������Ի��ܡ�
		if (pNextChain) {
			physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			physicalDeviceFeatures2.features = enabledFeatures;
			physicalDeviceFeatures2.pNext = pNextChain;
			deviceCreateInfo.pEnabledFeatures = nullptr;		// ��deviceCreateInfo��pEnabledFeatures�ֶ���Ϊnullptr����ʾ��ʹ����ͨ�������豸���Խṹ��
			deviceCreateInfo.pNext = &physicalDeviceFeatures2;	// ��deviceCreateInfo��pNext�ֶ���ΪphysicalDeviceFeatures2�ĵ�ַ����ʾʹ�������豸����2�ṹ��
		}


		// �豸������Ϣ
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};// ����һ�����д�����Ϣ�����������ڴ洢��Ҫ�����Ķ��еĲ���
		// ����һ��Ĭ�ϵĶ������ȼ�������ָ�����еĵ������ȼ�����Χ��0.0��1.0��Խ��Խ��
		const float defaultQueuePriority(0.0f);
		// �����Ҫ�Ķ��а���ͼ�����ԣ��ʹ������豸�Ķ��������ҵ�һ��֧��ͼ�����ԵĶ����壬�������������洢��queueFamilyIndices.graphics�С�
		// Ȼ�󴴽�һ�����д�����Ϣ��ָ�����������������������Ͷ������ȼ�����������ӵ����д�����Ϣ��������
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
		// �������Ҫͼ�����ԣ��Ͱ�queueFamilyIndices.graphics��Ϊ0����ʾû��ͼ�ζ�
		else
		{
			queueFamilyIndices.graphics = 0;
		}
		// �����Ҫ�Ķ��а����������ԣ��ʹ������豸�Ķ��������ҵ�һ��֧�ּ������ԵĶ����壬�������������洢��queueFamilyIndices.compute�С�
		// �������������ͼ�ζ����岻ͬ���ʹ���һ�����д�����Ϣ��ָ�����������������������Ͷ������ȼ�����������ӵ����д�����Ϣ��������
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
		// �������Ҫ�������ԣ��Ͱ�queueFamilyIndices.compute��ΪqueueFamilyIndices.graphics����ʾʹ��ͼ�ζ�����Ϊ�������
		else
		{
			queueFamilyIndices.compute = queueFamilyIndices.graphics;
		}
		// �����Ҫ�Ķ��а����������ԣ��ʹ������豸�Ķ��������ҵ�һ��֧�ִ������ԵĶ����壬�������������洢��queueFamilyIndices.transfer�С�
		// �������������ͼ�λ��������嶼��ͬ���ʹ���һ�����д�����Ϣ��ָ�����������������������Ͷ������ȼ�����������ӵ����д�����Ϣ��������
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
		// �������Ҫ�������ԣ��Ͱ�queueFamilyIndices.transfer��ΪqueueFamilyIndices.graphics����ʾʹ��ͼ�ζ�����Ϊ�������
		else
		{
			queueFamilyIndices.transfer = queueFamilyIndices.graphics;
		}
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();


		// �豸��չ��Ϣ
		// ����һ���豸��չ�����������ڴ洢��Ҫ���õ��豸��չ���ȰѲ����е��豸��չ���Ƶ����������
		// �����Ҫʹ�ý��������Ͱѽ�������չ��������ӵ��豸��չ��������
		std::vector<const char*> deviceExtensions(enabledDeviceExtensions);
		if (useSwapChain)
		{
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}
		// ����豸��չ��������Ϊ�գ��ͱ����������ÿ���豸��չ�Ƿ������豸֧�֣������֧�֣������һ��������Ϣ
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


		// ����vkCreateDevice���������������豸���豸������Ϣ�����������߼��豸�ĵ�ַ������һ���߼��豸�����ѽ����ֵ��result����������������VK_SUCCESS���ͷ��ؽ��
		VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
		if (result != VK_SUCCESS) 
		{
			return result;
		}

		// ����createCommandPool����������ͼ�ζ����������������һ������أ����ѽ����ֵ��commandPool�ֶΣ���ʾ���ڷ��������������
		commandPool = createCommandPool(queueFamilyIndices.graphics);

		return result;
	}

	VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, 
		VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data)
	{	// ����һ����������������������
		// usageFlags��һ��ö�����͵ı�־����ʾ����������;���綥�㻺�塢�������塢һ�»���ȣ�
		// memoryPropertyFlags��һ��ö�����͵ı�־����ʾ���������ڴ����ԣ����豸���ء������ɼ�������һ�µȣ�
		// size��һ��64λ����������ʾ�������Ĵ�С����λ���ֽڣ�
		// buffer��һ��ָ�룬���ڷ��ش����Ļ������ľ����
		// memory��һ��ָ�룬���ڷ��ط���Ļ������ڴ�ľ����
		// data��һ��ָ�룬��ʾ��Ҫ���Ƶ������������ݣ����Ϊnullptr����ʾ����Ҫ��������

		// ����һ��������������Ϣ�Ľṹ�壬����ָ�������������Ĳ���������;��־����С�ȣ�����Cetus::initializers::bufferCreateInfo������������;��־�ʹ�С����ʼ��������������Ϣ
		// �ѻ�����������Ϣ�Ĺ���ģʽ�ֶ���ΪVK_SHARING_MODE_EXCLUSIVE����ʾ������ֻ�ܱ�һ����������ʣ�����Ҫ���ж���������Ȩת��
		  // ����vkCreateBuffer�����������߼��豸��������������Ϣ���������ͻ������ĵ�ַ������һ�������������ѽ����ֵ��VK_CHECK_RESULT�꣬����������VK_SUCCESS���ͷ��ؽ��
		VkBufferCreateInfo bufferCreateInfo = Cetus::initializers::bufferCreateInfo(usageFlags, size);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer));

		// ����һ���ڴ������Ϣ�Ľṹ�壬����ָ�����仺�����ڴ�Ĳ��������С���ڴ����������ȣ�����Cetus::initializers::memoryAllocateInfo��������ʼ���ڴ������Ϣ
		VkMemoryAllocateInfo memAlloc = Cetus::initializers::memoryAllocateInfo();

		// ����һ���ڴ�����Ľṹ�壬���ڻ�ȡ���������ڴ��������С�����롢����λ��
		// ����vkGetBufferMemoryRequirements�����������߼��豸�����������ڴ�����ĵ�ַ����ȡ���������ڴ����󣬲��ѽ����ֵ��memReqs�ṹ��
		// ���ڴ������Ϣ�Ĵ�С�ֶ���Ϊ�ڴ�����Ĵ�С�ֶΣ���ʾ������ڴ��С���ڻ��������ڴ������С
		// ����getMemoryType�����������ڴ����������λ�ֶΡ��ڴ����Ա�־���ڴ����������ĵ�ַ�������ڴ����Ա�־�������豸���ڴ��������ҵ�һ��ƥ����ڴ����ͣ���������������ֵ���ڴ������Ϣ���ڴ����������ֶ�
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);

		// ����һ���ڴ�����־��Ϣ�Ľṹ�壬����ָ�����仺�����ڴ�ı�־�����豸��ַ��
		// �������������;��־����VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT����ʾ��������Ҫ֧���豸��ַ
		//	�豸��ַ��device address����һ��64λ������������Ψһ��ʶһ��Vulkan�����绺������ͼ�񡢼��ٽṹ�ȡ�
		//	�豸��ַ��������ɫ����ʹ�ã��Ա�ֱ�ӷ�����Щ��������ݣ�������Ҫͨ����������������
		//	�豸��ַ��ʹ�ÿ�����߹���׷�ٵ����ܣ���Ϊ���Ա��������ڴ���ʿ�����
		// �Ͱ��ڴ�����־��Ϣ�������ֶ���ΪVK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR��
		// ���ڴ�����־��Ϣ�ı�־�ֶ���ΪVK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR��
		// ���ڴ������Ϣ��pNext�ֶ���Ϊ�ڴ�����־��Ϣ�ĵ�ַ����ʾ���ӵ��ڴ������Ϣ��pNext��
		VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
		if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
			allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
			allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
			memAlloc.pNext = &allocFlagsInfo;
		}
		VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));
			
		
		if (data != nullptr)			// ������ݲ��ǿ�ָ�룬��ʾ��Ҫ�������ݵ�������
		{
			void *mapped;				// ����һ��ָ�룬����ӳ�仺�����ڴ�
			// ����vkMapMemory�����������߼��豸���������ڴ桢ƫ�ơ���С����־��ӳ��ָ��ĵ�ַ��ӳ�仺�����ڴ棬���ѽ����ֵ��VK_CHECK_RESULT�꣬����������VK_SUCCESS���ͷ��ؽ��
			VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
			memcpy(mapped, data, size);	// ����memcpy����������ӳ��ָ�롢����ָ��ʹ�С�������ݸ��Ƶ�ӳ��Ļ������ڴ���
			// ����ڴ����Ա�־������VK_MEMORY_PROPERTY_HOST_COHERENT_BIT�����ǲ����Զ����������˺��豸�˵�����һ�£�����Ҫ�ֶ�ˢ�»������ڴ棬ʹ���豸�ܹ�����������д��
			if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				// ����һ��ӳ���ڴ淶Χ�Ľṹ�壬����ָ����Ҫˢ�µĻ������ڴ�ķ�Χ�����ڴ桢ƫ�ơ���С�ȣ�����Cetus::initializers::mappedMemoryRange��������ʼ��ӳ���ڴ淶Χ
				VkMappedMemoryRange mappedRange = Cetus::initializers::mappedMemoryRange();
				mappedRange.memory = *memory;		// ��ӳ���ڴ淶Χ���ڴ��ֶ���Ϊ�������ڴ棬��ʾ��Ҫˢ�µ��ڴ�
				mappedRange.offset = 0;				// ��ӳ���ڴ淶Χ��ƫ���ֶ���Ϊ0����ʾ�ӻ������ڴ����ʼλ�ÿ�ʼˢ��
				mappedRange.size = size;			// ��ӳ���ڴ淶Χ�Ĵ�С�ֶ���Ϊ�������Ĵ�С����ʾˢ�������������ڴ�
				// ����vkFlushMappedMemoryRanges�����������߼��豸��ӳ���ڴ淶Χ�����������飬ˢ�»������ڴ棬���ѽ����ֵ��VK_CHECK_RESULT�꣬����������VK_SUCCESS���ͷ��ؽ��
				// vkFlushMappedMemoryRanges�������Ǳ�֤������д�뵽��һ�����ڴ�����ݶ����豸�˿ɼ���Ҳ����˵��������ˢ�������˵Ļ��棬ʹ���豸���ܹ���ȡ�����µ����ݡ�
				// Flush��һ��Ӣ�ﵥ�ʣ���ʾˢ�»��ϴ����˼���������ʾˢ�������˵Ļ��棬ʹ���豸���ܹ��������µ����ݡ�
				// Mapped��һ��Ӣ�ﵥ�ʣ���ʾӳ�����˼���������ʾ�Ѿ�ӳ�䵽�����˵��ڴ档
				// Memory��һ��Ӣ�ﵥ�ʣ���ʾ�ڴ����˼���������ʾ��Ҫˢ�µ��ڴ档
				// Ranges��һ��Ӣ�ﵥ�ʣ���ʾ��Χ����˼���������ʾ��Ҫˢ�µ��ڴ�ķ�Χ��
				vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
			}
			vkUnmapMemory(logicalDevice, *memory);	// ����vkUnmapMemory�����������߼��豸�ͻ������ڴ棬ȡ��ӳ�仺�����ڴ�
		}

		// ����vkBindBufferMemory�����������߼��豸�����������������ڴ��ƫ�ƣ��󶨻������ͻ������ڴ棬���ѽ����ֵ��VK_CHECK_RESULT�꣬����������VK_SUCCESS���ͷ��ؽ��
		VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

		return VK_SUCCESS;
	}

	VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, Cetus::Buffer *buffer, VkDeviceSize size, void *data)
	{
		// ���û��������豸Ϊ�߼��豸 buffer->device = logicalDevice;
		buffer->device = logicalDevice;

		// ��������������
		VkBufferCreateInfo bufferCreateInfo = Cetus::initializers::bufferCreateInfo(usageFlags, size);	//���ϸ�����ȱ��һ�����й���ģʽ
		VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer->buffer));

		// �����������ڴ�
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

		buffer->alignment = memReqs.alignment;				// ���������Ķ�����Ϊ�ڴ�����ṹ��Ķ���
		buffer->size = size;								// ���������Ĵ�С��Ϊ����Ĳ���size
		buffer->usageFlags = usageFlags;					// ������������;��־��Ϊ����Ĳ���usageFlags
		buffer->memoryPropertyFlags = memoryPropertyFlags;	// �����������ڴ����Ա�־��Ϊ����Ĳ���memoryPropertyFlags

		// �ϴ�����������
		if (data != nullptr)
		{
			VK_CHECK_RESULT(buffer->map());					// ����buffer->map�����������������ڴ�ӳ�䵽�����ڴ棬�����丳ֵ��buffer->mapped
			memcpy(buffer->mapped, data, size);				// ����memcpy��������dataָ������ݸ��Ƶ�buffer->mappedָ����ڴ��У����ƵĴ�СΪsize
			if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)// ������������ڴ����Ա�־������VK_MEMORY_PROPERTY_HOST_COHERENT_BIT�����ڴ治������һ�µ�
				buffer->flush();							// ����buffer->flush�������������ڴ������ˢ�µ��豸�ڴ���
			buffer->unmap();								// ����buffer->unmap������ȡ�����������ڴ�ӳ��
		}

		// �������ڴ��
		buffer->setupDescriptor();							// ����buffer->setupDescriptor���������û���������������Ϣ����������������ƫ�����ͷ�Χ
		return buffer->bind();								// ����buffer->bind�����������������ڴ�󶨵����������󣬲����ذ󶨽��
	}

	void VulkanDevice::copyBuffer(Cetus::Buffer *src, Cetus::Buffer *dst, VkQueue queue, VkBufferCopy *copyRegion)
	{	// ����һ�������������ĸ�������
		// src��һ��ָ��Դ��������ָ�룬
		// dst��һ��ָ��Ŀ�껺������ָ�룬
		// queue��һ��VkQueue���󣬱�ʾ����ִ�и��Ʋ����Ķ��У�
		// copyRegion��һ��ָ��VkBufferCopy�ṹ���ָ�룬��ʾ���Ƶ�Դ��Ŀ���������Ϊnullptr����ʾ��������������
		assert(dst->size <= src->size);	// ����Ŀ�껺�����Ĵ�С��С��Դ�������Ĵ�С
		assert(src->buffer);			// ����Դ�������ľ����Ϊ��
		VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkBufferCopy bufferCopy{};		// ����һ��VkBufferCopy�ṹ�壬��ʾ���Ƶ�Դ��Ŀ������
		if (copyRegion == nullptr)
		{
			bufferCopy.size = src->size;
		}
		else
		{
			bufferCopy = *copyRegion;
		}

		// ����vkCmdCopyBuffer�������������������Դ��������Ŀ�껺���������������������������1���͸�����������飨������bufferCopy�ĵ�ַ��������������м�¼һ�����ƻ�����������
		vkCmdCopyBuffer(copyCmd, src->buffer, dst->buffer, 1, &bufferCopy);

		// ����flushCommandBuffer������������������Ͷ��У�������������ļ�¼���ύ������������У����ȴ�������ɲ���
		flushCommandBuffer(copyCmd, queue);
	}

	VkCommandPool VulkanDevice::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
	{
		// ����һ����������������������queueFamilyIndex��һ��32λ����������ʾ����������Ķ�����������createFlags��һ��ö�����͵ı�־����ʾ����صĴ�����־������һ��VkCommandPool���󣬱�ʾ�����������
		// ����һ������ش�����Ϣ�Ľṹ�壬����ָ����������صĲ������������������������־��
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
		cmdPoolInfo.flags = createFlags;
		VkCommandPool cmdPool;
		VK_CHECK_RESULT(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool));
		return cmdPool;
	}

	VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
	{	// ����һ����������������������
		// level��һ��ö�����͵�ֵ����ʾ��������ļ��𣬿�����VK_COMMAND_BUFFER_LEVEL_PRIMARY��VK_COMMAND_BUFFER_LEVEL_SECONDARY����ʾ����������������������
		// pool��һ��VkCommandPool���󣬱�ʾ���ڷ����������������أ�
		// begin��һ������ֵ����ʾ�Ƿ�ʼ��¼�������һ��VkCommandBuffer���󣬱�ʾ�������������
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
		// createCommandBuffer beginΪtrue����ʼ��¼���壬flushCommandBuffer������¼���ϴ��������ϣ����ͷŻ�������
		// ����һ�������������ĸ�������
		// commandBuffer��һ��VkCommandBuffer���󣬱�ʾҪ�������ύ���ͷŵ����������
		// queue��һ��VkQueue���󣬱�ʾ����ִ����������Ķ��У�
		// pool��һ��VkCommandPool���󣬱�ʾ���ڷ����������������أ�
		// free��һ������ֵ����ʾ�Ƿ��ͷ�����������ڴ�
		// �����������ǿվ������ʾû����Ч�������������ֱ�ӷ���
		if (commandBuffer == VK_NULL_HANDLE)
		{
			return;
		}
		// ����vkEndCommandBuffer�������������������������������ļ�¼
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		// ����һ�������ύ��Ϣ�Ľṹ�壬����ָ���ύ��������Ĳ�������ȴ��ź������ź��ź����������������������ȣ�
		// ����Cetus::initializers::submitInfo��������ʼ���ύ��Ϣ
		VkSubmitInfo submitInfo = Cetus::initializers::submitInfo();
		submitInfo.commandBufferCount = 1;				// ���ύ��Ϣ��������������ֶ���Ϊ1����ʾֻ�ύһ���������
		submitInfo.pCommandBuffers = &commandBuffer;	// ���ύ��Ϣ��������������ֶ���Ϊ��������ĵ�ַ����ʾҪ�ύ���������
		
		// ����һ��դ��������Ϣ�Ľṹ�壬����ָ������դ���Ĳ��������־�ȣ�����Cetus::initializers::fenceCreateInfo����
		VkFenceCreateInfo fenceInfo = Cetus::initializers::fenceCreateInfo(VK_FLAGS_NONE);
		VkFence fence;									// ���岢���洴��һ��VkFence����
		VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
		
		// ����vkQueueSubmit������������С��ύ��Ϣ��������������1�����ύ��Ϣ�����飨������submitInfo�ĵ�ַ����դ����
		//	����������ύ������
		// ����vkWaitForFences�����������߼��豸��դ����������������1����դ�������飨������fence�ĵ�ַ�����Ƿ�ȴ�����դ������ʱʱ�䣨������Ĭ�ϵĳ�ʱʱ�䣩��
		//	�ȴ�դ������������ʾ�����������ȫ�����䵽������ɲ���
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
		
		// ����vkDestroyFence�����������߼��豸��դ���ͷ�����������դ��
		vkDestroyFence(logicalDevice, fence, nullptr);	
		if (free)
		{
			// ���free����Ϊtrue����ʾ��Ҫ�ͷ�����������ڴ�
			// ����vkFreeCommandBuffers�����������߼��豸������ء����������������������1����������������飨������commandBuffer�ĵ�ַ����
			//  �ͷ�����������ڴ�
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
		// ����һ������������һ��������
		// checkSamplingSupport��һ������ֵ����ʾ�Ƿ����ʽ�Ƿ�֧�ֲ���ͼ�񣬷���һ��VkFormatö��ֵ����ʾ֧�ֵ���ȸ�ʽ
		// ����һ��VkFormatö��ֵ����������ʾ��ѡ����ȸ�ʽ���������ȼ��Ӹߵ�������
		std::vector<VkFormat> depthFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
		for (auto& format : depthFormats)	// ������ѡ����ȸ�ʽ
		{
			// ����һ��VkFormatProperties�ṹ�壬���ڻ�ȡ��ʽ�����ԣ�������ƽ�����ԡ�����ƽ�����ԡ��������Ե�
			// ����vkGetPhysicalDeviceFormatProperties���������������豸����ʽ�͸�ʽ���Եĵ�ַ����ȡ��ʽ�����ԣ�����ֵ��formatProperties�ṹ��
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
			// �����ʽ������ƽ�����԰���VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT��־����ʾ��ʽ֧�����ģ�帽��
			if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				if (checkSamplingSupport) { // ���������֧�֣��ͽ�һ���ж�
					// �����ʽ������ƽ�����Բ�����VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT��־����ʾ��ʽ��֧�ֲ���ͼ�񣬾����������ʽ��������һ��ѭ��
					if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
						continue;
					}
				}
				// �����ʽ����Ҫ�󣬾ͷ��������ʽ
				return format;
			}
		}
		// ������������к�ѡ����ȸ�ʽ��û���ҵ����ʵĸ�ʽ�����׳�һ������ʱ���󣬱�ʾû���ҵ�ƥ�����ȸ�ʽ
		throw std::runtime_error("Could not find a matching depth format");
	}

};
