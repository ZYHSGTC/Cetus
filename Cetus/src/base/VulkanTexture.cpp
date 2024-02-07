#include "VulkanTexture.h"

namespace Cetus
{
	
	void Texture::updateDescriptor()
	{	// 定义一个Texture类的成员函数，用于更新纹理的描述符（descriptor）
		descriptor.sampler = sampler;
		descriptor.imageView = view;
		descriptor.imageLayout = imageLayout;
	}

	void Texture::destroy()
	{	// 定义一个Texture类的成员函数，用于销毁纹理的相关资源
		vkDestroyImageView(device->logicalDevice, view, nullptr);
		vkDestroyImage(device->logicalDevice, image, nullptr);
		if (sampler)	// 如果纹理的采样器存在，就销毁它
		{
			vkDestroySampler(device->logicalDevice, sampler, nullptr);
		}				// 释放纹理的设备内存
		vkFreeMemory(device->logicalDevice, deviceMemory, nullptr);
	}

	ktxResult Texture::loadKTXFile(std::string filename, ktxTexture **target)
	{
		ktxResult result = KTX_SUCCESS;
		if (!Cetus::tools::fileExists(filename)) {
			Cetus::tools::exitFatal("Could not load texture from " + filename + "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
		}
		result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, target);				
		return result;
	}

	// 用纹理文件路径创建纹理
	void Texture2D::loadFromFile(std::string filename, VkFormat format, Cetus::VulkanDevice *device, 
		VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout, bool forceLinear)
	{	// 定义一个Texture2D类的成员函数，用于从一个文件加载一个二维纹理
		// 参数分别为：文件名，图像格式，Vulkan设备，复制队列，图像使用标志，图像布局，是否强制线性
		
		// 读取ktxTexture地数据
		ktxTexture* ktxTexture;										// 定义一个ktxTexture指针，用于存储从KTX文件（一种纹理文件格式）加载的纹理
		ktxResult result = loadKTXFile(filename, &ktxTexture);		// 调用Texture类的静态函数loadKTXFile，根据文件名加载一个ktxTexture对象，并返回一个ktxResult类型的结果
		assert(result == KTX_SUCCESS);								// 断言结果为KTX_SUCCESS，否则表示加载失败
		ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);// 获取ktxTexture对象的数据指针，并赋值给一个ktx_uint8_t类型的指针
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);	// 获取ktxTexture对象的数据大小，并赋值给一个ktx_size_t类型的变量

		// 初始化纹理的数据
		this->device = device;				// 将参数device赋值给Texture类的成员变量device，表示使用的Vulkan设备
		this->imageLayout = imageLayout;	// 将参数imageLayout赋值给Texture类的成员变量imageLayout，表示纹理的图像布局
		width = ktxTexture->baseWidth;		// 将ktxTexture对象的基本宽度赋值给Texture类的成员变量width，表示纹理的宽度
		height = ktxTexture->baseHeight;	// 将ktxTexture对象的基本高度赋值给Texture类的成员变量height，表示纹理的高度
		mipLevels = ktxTexture->numLevels;	// 将ktxTexture对象的层级数赋值给Texture类的成员变量mipLevels，表示纹理的多级渐远纹理（mipmap）层数

		VkBool32 useStaging = !forceLinear;	// 定义一个VkBool32类型的变量，用于表示是否使用暂存缓冲区（staging buffer）

		VkMemoryAllocateInfo memAllocInfo = Cetus::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;
		// 创建并开始命令缓冲区记录
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// 创建texture图像与图像内存
		if (useStaging)						// 如果使用暂存缓冲区
		{
			VkBuffer stagingBuffer;			// 定义一个VkBuffer类型的变量，用于存储暂存缓冲区
			VkDeviceMemory stagingMemory;	// 定义一个VkDeviceMemory类型的变量，用于存储暂存缓冲区的内存

			// 创建暂缓缓冲区
			VkBufferCreateInfo bufferCreateInfo = Cetus::initializers::bufferCreateInfo();
			bufferCreateInfo.size = ktxTextureSize;						// 表示缓冲区的大小
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;	// 表示缓冲区用于传输源
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// 表示缓冲区的共享模式为独占
			VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));

			vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			// 暂存缓冲区是一种用于在主机和设备之间传输数据的缓冲区，它需要设置VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT和VK_MEMORY_PROPERTY_HOST_COHERENT_BIT两个标志位，原因如下：
			//  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT表示这种内存可以被主机（CPU）访问，这是为了将数据从主机复制到暂存缓冲区，然后再从暂存缓冲区复制到设备（GPU）可访问的内存。
			// 	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT表示这种内存不需要显式地刷新或失效，以保证主机和设备之间的数据一致性。这是为了避免额外的同步开销，以及在不同平台上的行为差异。
			memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
			VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));


			// 向暂缓缓冲区加载入数据
			uint8_t *data;
			VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
			memcpy(data, ktxTextureData, ktxTextureSize);
			vkUnmapMemory(device->logicalDevice, stagingMemory);


			// 创建一个texture储存的图像
			VkImageCreateInfo imageCreateInfo = Cetus::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;				// 将VK_IMAGE_TYPE_2D赋值给imageCreateInfo变量的imageType字段，表示图像的类型为二维
			imageCreateInfo.format = format;							// 将参数format赋值给imageCreateInfo变量的format字段，表示图像的格式
			imageCreateInfo.mipLevels = mipLevels;						// 将纹理的多级渐远纹理层数赋值给imageCreateInfo变量的mipLevels字段，表示图像的多级渐远纹理层数
			imageCreateInfo.arrayLayers = 1;							// 将1赋值给imageCreateInfo变量的arrayLayers字段，表示图像的数组层数
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;			// 将VK_SAMPLE_COUNT_1_BIT赋值给imageCreateInfo变量的samples字段，表示图像的采样数为1
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;			// 将VK_IMAGE_TILING_OPTIMAL赋值给imageCreateInfo变量的tiling字段，表示图像的平铺方式为最优
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// 将VK_SHARING_MODE_EXCLUSIVE赋值给imageCreateInfo变量的sharingMode字段，表示图像的共享模式为独占
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	// 将VK_IMAGE_LAYOUT_UNDEFINED赋值给imageCreateInfo变量的initialLayout字段，表示图像的初始布局为未定义
			imageCreateInfo.extent = { width, height, 1 };				// 将纹理的宽度，高度和1赋值给imageCreateInfo变量的extent字段，表示图像的尺寸
			imageCreateInfo.usage = imageUsageFlags;					// 将参数imageUsageFlags赋值给imageCreateInfo变量的usage字段，表示图像的使用标志
			if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
			{	// 如果图像的使用标志不包含VK_IMAGE_USAGE_TRANSFER_DST_BIT，就将其添加到图像的使用标志中，表示图像用于传输目的
				imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}
			VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image));


			// 创建图像设备内存
			vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
			VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));


			// 暂缓缓冲区到图像的复制区域
			std::vector<VkBufferImageCopy> bufferCopyRegions;
			for (uint32_t i = 0; i < mipLevels; i++)
			{
				ktx_size_t offset;
				KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
				assert(result == KTX_SUCCESS);

				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = i;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				// 将ktxTexture对象的基本宽度右移i位（相当于除以2的i次方），并取最大值为1，赋值给bufferCopyRegion变量的imageExtent字段的width字段，表示图像的宽度
				// 将ktxTexture对象的基本高度右移i位（相当于除以2的i次方），并取最大值为1，赋值给bufferCopyRegion变量的imageExtent字段的height字段，表示图像的高度
				bufferCopyRegion.imageExtent.width = max(1u, ktxTexture->baseWidth >> i);	//1u是一个无符号整数（unsigned int）字面量，表示数值1
				bufferCopyRegion.imageExtent.height = max(1u, ktxTexture->baseHeight >> i);
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = offset;

				bufferCopyRegions.push_back(bufferCopyRegion);
			}

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = mipLevels;
			subresourceRange.layerCount = 1;

			Cetus::tools::setImageLayout(
				copyCmd,
				image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data()
			);

			Cetus::tools::setImageLayout(
				copyCmd,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				imageLayout,
				subresourceRange);

			// 结束并释放命令缓冲区，删释放暂存缓冲区
			device->flushCommandBuffer(copyCmd, copyQueue);
			vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
			vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);
		}
		else	// 如果不使用暂存缓冲区
		{
			// 断言formatProperties变量的linearTilingFeatures字段包含VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT，
			// 否则表示图像格式不支持线性平铺
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(device->physicalDevice, format, &formatProperties);
			assert(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

			VkImage mappableImage;			// 定义一个VkImage类型的变量，用于存储可映射的图像
			VkDeviceMemory mappableMemory;	// 定义一个VkDeviceMemory类型的变量，用于存储可映射的内存

			// 创建纹理图像
			VkImageCreateInfo imageCreateInfo = Cetus::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;				// 将VK_IMAGE_TYPE_2D赋值给imageCreateInfo变量的imageType字段，表示图像的类型为二维
			imageCreateInfo.format = format;							// 将参数format赋值给imageCreateInfo变量的format字段，表示图像的格式
			imageCreateInfo.extent = { width, height, 1 };				// 将纹理的宽度，高度和1赋值给imageCreateInfo变量的extent字段，表示图像的尺寸
			imageCreateInfo.mipLevels = 1;								// 将1赋值给imageCreateInfo变量的mipLevels字段，表示图像的多级渐远纹理层数
			imageCreateInfo.arrayLayers = 1;							// 将1赋值给imageCreateInfo变量的arrayLayers字段，表示图像的数组层数
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;			// 将VK_SAMPLE_COUNT_1_BIT赋值给imageCreateInfo变量的samples字段，表示图像的采样数为1
			imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;			// 将VK_IMAGE_TILING_LINEAR赋值给imageCreateInfo变量的tiling字段，表示图像的平铺方式为线性
			imageCreateInfo.usage = imageUsageFlags;					// 将参数imageUsageFlags赋值给imageCreateInfo变量的usage字段，表示图像的使用标志
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// 将VK_SHARING_MODE_EXCLUSIVE赋值给imageCreateInfo变量的sharingMode字段，表示图像的共享模式为独占
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	// 将VK_IMAGE_LAYOUT_UNDEFINED赋值给imageCreateInfo变量的initialLayout字段，表示图像的初始布局为未定义
			VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &mappableImage));

			// 创建可映射图形内存
			vkGetImageMemoryRequirements(device->logicalDevice, mappableImage, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &mappableMemory));
			VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, mappableImage, mappableMemory, 0));

			// 获得图像子资源布局
			VkImageSubresource subRes = {};
			subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subRes.mipLevel = 0;
			VkSubresourceLayout subResLayout;
			vkGetImageSubresourceLayout(device->logicalDevice, mappableImage, &subRes, &subResLayout);

			void *data;
			VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, mappableMemory, 0, memReqs.size, 0, &data));
			memcpy(data, ktxTextureData, memReqs.size);
			vkUnmapMemory(device->logicalDevice, mappableMemory);

			image = mappableImage;
			deviceMemory = mappableMemory;
			Cetus::tools::setImageLayout(copyCmd, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);
			
			device->flushCommandBuffer(copyCmd, copyQueue);
		}

		ktxTexture_Destroy(ktxTexture);


		// 创建采样器
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;						// 将VK_FILTER_LINEAR赋值给samplerCreateInfo变量的magFilter字段，表示当纹理被放大时，采样器使用线性过滤
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;						// 将VK_FILTER_LINEAR赋值给samplerCreateInfo变量的minFilter字段，表示当纹理被缩小时，采样器使用线性过滤
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// 将VK_SAMPLER_MIPMAP_MODE_LINEAR赋值给samplerCreateInfo变量的mipmapMode字段，表示当纹理使用多级渐远纹理（mipmap）时，采样器使用线性过滤
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// 将VK_SAMPLER_ADDRESS_MODE_REPEAT赋值给samplerCreateInfo变量的addressModeU字段，表示当纹理的水平坐标超出范围时，采样器使用重复模式
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// 将VK_SAMPLER_ADDRESS_MODE_REPEAT赋值给samplerCreateInfo变量的addressModeV字段，表示当纹理的垂直坐标超出范围时，采样器使用重复模式
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// 将VK_SAMPLER_ADDRESS_MODE_REPEAT赋值给samplerCreateInfo变量的addressModeW字段，表示当纹理的深度坐标超出范围时，采样器使用重复模式
		samplerCreateInfo.mipLodBias = 0.0f;								// 将0.0f赋值给samplerCreateInfo变量的mipLodBias字段，表示采样器对多级渐远纹理的层级偏移量为0
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;					// 将VK_COMPARE_OP_NEVER赋值给samplerCreateInfo变量的compareOp字段，表示采样器不使用比较操作
		samplerCreateInfo.minLod = 0.0f;									// 将0.0f赋值给samplerCreateInfo变量的minLod字段，表示采样器对多级渐远纹理的最小层级为0
		samplerCreateInfo.maxLod = (useStaging) ? (float)mipLevels : 0.0f;	// 根据useStaging变量的值，将纹理的多级渐远纹理层数或0.0f赋值给samplerCreateInfo变量的maxLod字段，表示采样器对多级渐远纹理的最大层级
		// 根据device->enabledFeatures.samplerAnisotropy变量的值，将device->properties.limits.maxSamplerAnisotropy或1.0f赋值给samplerCreateInfo变量的maxAnisotropy字段，表示采样器的最大各向异性
		samplerCreateInfo.anisotropyEnable = device->enabledFeatures.samplerAnisotropy;
		samplerCreateInfo.maxAnisotropy = device->enabledFeatures.samplerAnisotropy ? device->properties.limits.maxSamplerAnisotropy : 1.0f;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;	// 将VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE赋值给samplerCreateInfo变量的borderColor字段，表示当纹理的坐标超出范围时，采样器使用白色边界颜色
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerCreateInfo, nullptr, &sampler));


		// 创建纹理的图像视图
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		viewCreateInfo.subresourceRange.levelCount = (useStaging) ? mipLevels : 1;
		viewCreateInfo.image = image;
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view));

		updateDescriptor();
	}

	// 用已有的纹理数据创建纹理
	void Texture2D::fromBuffer(void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, 
		Cetus::VulkanDevice *device, VkQueue copyQueue, VkFilter filter, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout)
	{
		assert(buffer);

		this->device = device;
		this->imageLayout = imageLayout;
		width = texWidth;
		height = texHeight;
		mipLevels = 1;

		// 开始复制命令缓冲
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		// 创建缓冲区
		VkBufferCreateInfo bufferCreateInfo = Cetus::initializers::bufferCreateInfo();
		bufferCreateInfo.size = bufferSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));

		// 创建缓冲区内存
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);
		VkMemoryAllocateInfo memAllocInfo = Cetus::initializers::memoryAllocateInfo();
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
		VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));

		// 将数据复制到暂存缓冲区中
		uint8_t *data;
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
		memcpy(data, buffer, bufferSize);
		vkUnmapMemory(device->logicalDevice, stagingMemory);


		// 创建纹理图像
		VkImageCreateInfo imageCreateInfo = Cetus::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = mipLevels;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.usage = imageUsageFlags;
		if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		{
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image));
		vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);

		// 创建纹理图像内存
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));


		// 进行图像复制，创建图像复制区域信息
		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = width;
		bufferCopyRegion.imageExtent.height = height;
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = 0;

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = 1;

		Cetus::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion
		);

		Cetus::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			imageLayout,
			subresourceRange);

		// 结束复制命令缓冲区
		device->flushCommandBuffer(copyCmd, copyQueue);
		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);

		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = filter;
		samplerCreateInfo.minFilter = filter;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;	// 采样第零层
		samplerCreateInfo.maxAnisotropy = 1.0f;
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerCreateInfo, nullptr, &sampler));

		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.pNext = NULL;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.image = image;
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view));

		updateDescriptor();
	}

	void Texture2DArray::loadFromFile(std::string filename, VkFormat format, Cetus::VulkanDevice *device, 
		VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout)
	{
		// 纹理信息读取
		ktxTexture* ktxTexture;
		ktxResult result = loadKTXFile(filename, &ktxTexture);
		assert(result == KTX_SUCCESS);
		ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);


		// 纹理成员初始化
		this->device = device;
		this->imageLayout = imageLayout;
		width = ktxTexture->baseWidth;
		height = ktxTexture->baseHeight;
		layerCount = ktxTexture->numLayers;
		mipLevels = ktxTexture->numLevels;
	

		// 建立暂存缓冲区
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VkBufferCreateInfo bufferCreateInfo = Cetus::initializers::bufferCreateInfo();
		bufferCreateInfo.size = ktxTextureSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));

		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);
		VkMemoryAllocateInfo memAllocInfo = Cetus::initializers::memoryAllocateInfo();
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
		VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));


		// 复制数据到暂存缓冲区
		uint8_t *data;
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
		memcpy(data, ktxTextureData, ktxTextureSize);
		vkUnmapMemory(device->logicalDevice, stagingMemory);


		// 创建纹理图像
		VkImageCreateInfo imageCreateInfo = Cetus::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.usage = imageUsageFlags;
		if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		{
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		imageCreateInfo.arrayLayers = layerCount;//
		imageCreateInfo.mipLevels = mipLevels;
		VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image));


		// 创建纹理图像内存
		vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));


		// 暂存缓冲区向图像复制数据
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		std::vector<VkBufferImageCopy> bufferCopyRegions;
		for (uint32_t layer = 0; layer < layerCount; layer++)//
		{
			for (uint32_t level = 0; level < mipLevels; level++)
			{
				ktx_size_t offset;
				KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, level, layer, 0, &offset);//
				assert(result == KTX_SUCCESS);

				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = level;
				bufferCopyRegion.imageSubresource.baseArrayLayer = layer;//
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
				bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = offset;

				bufferCopyRegions.push_back(bufferCopyRegion);
			}
		}

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = layerCount;//

		Cetus::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(bufferCopyRegions.size()),
			bufferCopyRegions.data());

		Cetus::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			imageLayout,
			subresourceRange);

		device->flushCommandBuffer(copyCmd, copyQueue);


		// 创建采样器
		VkSamplerCreateInfo samplerCreateInfo = Cetus::initializers::samplerCreateInfo();
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.maxAnisotropy = device->enabledFeatures.samplerAnisotropy ? device->properties.limits.maxSamplerAnisotropy : 1.0f;
		samplerCreateInfo.anisotropyEnable = device->enabledFeatures.samplerAnisotropy;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = (float)mipLevels;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerCreateInfo, nullptr, &sampler));


		// 创建纹理图像
		VkImageViewCreateInfo viewCreateInfo = Cetus::initializers::imageViewCreateInfo();
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		viewCreateInfo.format = format;
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		viewCreateInfo.subresourceRange.layerCount = layerCount;//
		viewCreateInfo.subresourceRange.levelCount = mipLevels;
		viewCreateInfo.image = image;
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view));

		ktxTexture_Destroy(ktxTexture);
		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);

		updateDescriptor();
	}

	void TextureCubeMap::loadFromFile(std::string filename, VkFormat format, Cetus::VulkanDevice *device, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout)
	{
		// 获得纹理信息
		ktxTexture* ktxTexture;
		ktxResult result = loadKTXFile(filename, &ktxTexture);
		assert(result == KTX_SUCCESS);
		ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);


		// 初始化成员变量
		this->device = device;
		this->imageLayout = imageLayout;
		width = ktxTexture->baseWidth;
		height = ktxTexture->baseHeight;
		mipLevels = ktxTexture->numLevels;


		// 创建暂存缓冲区
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VkBufferCreateInfo bufferCreateInfo = Cetus::initializers::bufferCreateInfo();
		bufferCreateInfo.size = ktxTextureSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));

		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);
		VkMemoryAllocateInfo memAllocInfo = Cetus::initializers::memoryAllocateInfo();
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
		VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));


		// 向暂存缓冲区存入数据
		uint8_t *data;
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
		memcpy(data, ktxTextureData, ktxTextureSize);
		vkUnmapMemory(device->logicalDevice, stagingMemory);

		
		// 创建纹理图像
		VkImageCreateInfo imageCreateInfo = Cetus::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = mipLevels;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.usage = imageUsageFlags;
		if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		{
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		imageCreateInfo.arrayLayers = 6;//
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;	//
		VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image));


		// 创建纹理图像内存
		vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));


		// 从暂存缓冲区复制数据到纹理图像
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		std::vector<VkBufferImageCopy> bufferCopyRegions;
		for (uint32_t face = 0; face < 6; face++)//
		{
			for (uint32_t level = 0; level < mipLevels; level++)
			{
				ktx_size_t offset;
				KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);//
				assert(result == KTX_SUCCESS);

				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = level;
				bufferCopyRegion.imageSubresource.baseArrayLayer = face;//
				bufferCopyRegion.imageSubresource.layerCount = 1;//
				bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
				bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = offset;

				bufferCopyRegions.push_back(bufferCopyRegion);
			}
		}

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = 6;//

		Cetus::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(bufferCopyRegions.size()),
			bufferCopyRegions.data());

		Cetus::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			imageLayout,
			subresourceRange);

		device->flushCommandBuffer(copyCmd, copyQueue);


		// 创建采样器
		VkSamplerCreateInfo samplerCreateInfo = Cetus::initializers::samplerCreateInfo();
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.maxAnisotropy = device->enabledFeatures.samplerAnisotropy ? device->properties.limits.maxSamplerAnisotropy : 1.0f;
		samplerCreateInfo.anisotropyEnable = device->enabledFeatures.samplerAnisotropy;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = (float)mipLevels;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerCreateInfo, nullptr, &sampler));


		// 创建纹理图像视图
		VkImageViewCreateInfo viewCreateInfo = Cetus::initializers::imageViewCreateInfo();
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;	//
		viewCreateInfo.format = format;
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		viewCreateInfo.subresourceRange.layerCount = 6;//
		viewCreateInfo.subresourceRange.levelCount = mipLevels;
		viewCreateInfo.image = image;
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view));

		ktxTexture_Destroy(ktxTexture);
		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);

		updateDescriptor();
	}

}
