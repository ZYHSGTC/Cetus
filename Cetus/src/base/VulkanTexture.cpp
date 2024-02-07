#include "VulkanTexture.h"

namespace Cetus
{
	
	void Texture::updateDescriptor()
	{	// ����һ��Texture��ĳ�Ա���������ڸ����������������descriptor��
		descriptor.sampler = sampler;
		descriptor.imageView = view;
		descriptor.imageLayout = imageLayout;
	}

	void Texture::destroy()
	{	// ����һ��Texture��ĳ�Ա������������������������Դ
		vkDestroyImageView(device->logicalDevice, view, nullptr);
		vkDestroyImage(device->logicalDevice, image, nullptr);
		if (sampler)	// �������Ĳ��������ڣ���������
		{
			vkDestroySampler(device->logicalDevice, sampler, nullptr);
		}				// �ͷ�������豸�ڴ�
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

	// �������ļ�·����������
	void Texture2D::loadFromFile(std::string filename, VkFormat format, Cetus::VulkanDevice *device, 
		VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout, bool forceLinear)
	{	// ����һ��Texture2D��ĳ�Ա���������ڴ�һ���ļ�����һ����ά����
		// �����ֱ�Ϊ���ļ�����ͼ���ʽ��Vulkan�豸�����ƶ��У�ͼ��ʹ�ñ�־��ͼ�񲼾֣��Ƿ�ǿ������
		
		// ��ȡktxTexture������
		ktxTexture* ktxTexture;										// ����һ��ktxTextureָ�룬���ڴ洢��KTX�ļ���һ�������ļ���ʽ�����ص�����
		ktxResult result = loadKTXFile(filename, &ktxTexture);		// ����Texture��ľ�̬����loadKTXFile�������ļ�������һ��ktxTexture���󣬲�����һ��ktxResult���͵Ľ��
		assert(result == KTX_SUCCESS);								// ���Խ��ΪKTX_SUCCESS�������ʾ����ʧ��
		ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);// ��ȡktxTexture���������ָ�룬����ֵ��һ��ktx_uint8_t���͵�ָ��
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);	// ��ȡktxTexture��������ݴ�С������ֵ��һ��ktx_size_t���͵ı���

		// ��ʼ�����������
		this->device = device;				// ������device��ֵ��Texture��ĳ�Ա����device����ʾʹ�õ�Vulkan�豸
		this->imageLayout = imageLayout;	// ������imageLayout��ֵ��Texture��ĳ�Ա����imageLayout����ʾ�����ͼ�񲼾�
		width = ktxTexture->baseWidth;		// ��ktxTexture����Ļ�����ȸ�ֵ��Texture��ĳ�Ա����width����ʾ����Ŀ��
		height = ktxTexture->baseHeight;	// ��ktxTexture����Ļ����߶ȸ�ֵ��Texture��ĳ�Ա����height����ʾ����ĸ߶�
		mipLevels = ktxTexture->numLevels;	// ��ktxTexture����Ĳ㼶����ֵ��Texture��ĳ�Ա����mipLevels����ʾ����Ķ༶��Զ����mipmap������

		VkBool32 useStaging = !forceLinear;	// ����һ��VkBool32���͵ı��������ڱ�ʾ�Ƿ�ʹ���ݴ滺������staging buffer��

		VkMemoryAllocateInfo memAllocInfo = Cetus::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;
		// ��������ʼ���������¼
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// ����textureͼ����ͼ���ڴ�
		if (useStaging)						// ���ʹ���ݴ滺����
		{
			VkBuffer stagingBuffer;			// ����һ��VkBuffer���͵ı��������ڴ洢�ݴ滺����
			VkDeviceMemory stagingMemory;	// ����һ��VkDeviceMemory���͵ı��������ڴ洢�ݴ滺�������ڴ�

			// �����ݻ�������
			VkBufferCreateInfo bufferCreateInfo = Cetus::initializers::bufferCreateInfo();
			bufferCreateInfo.size = ktxTextureSize;						// ��ʾ�������Ĵ�С
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;	// ��ʾ���������ڴ���Դ
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// ��ʾ�������Ĺ���ģʽΪ��ռ
			VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));

			vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			// �ݴ滺������һ���������������豸֮�䴫�����ݵĻ�����������Ҫ����VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT��VK_MEMORY_PROPERTY_HOST_COHERENT_BIT������־λ��ԭ�����£�
			//  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT��ʾ�����ڴ���Ա�������CPU�����ʣ�����Ϊ�˽����ݴ��������Ƶ��ݴ滺������Ȼ���ٴ��ݴ滺�������Ƶ��豸��GPU���ɷ��ʵ��ڴ档
			// 	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT��ʾ�����ڴ治��Ҫ��ʽ��ˢ�»�ʧЧ���Ա�֤�������豸֮�������һ���ԡ�����Ϊ�˱�������ͬ���������Լ��ڲ�ͬƽ̨�ϵ���Ϊ���졣
			memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
			VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));


			// ���ݻ�����������������
			uint8_t *data;
			VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
			memcpy(data, ktxTextureData, ktxTextureSize);
			vkUnmapMemory(device->logicalDevice, stagingMemory);


			// ����һ��texture�����ͼ��
			VkImageCreateInfo imageCreateInfo = Cetus::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;				// ��VK_IMAGE_TYPE_2D��ֵ��imageCreateInfo������imageType�ֶΣ���ʾͼ�������Ϊ��ά
			imageCreateInfo.format = format;							// ������format��ֵ��imageCreateInfo������format�ֶΣ���ʾͼ��ĸ�ʽ
			imageCreateInfo.mipLevels = mipLevels;						// ������Ķ༶��Զ���������ֵ��imageCreateInfo������mipLevels�ֶΣ���ʾͼ��Ķ༶��Զ�������
			imageCreateInfo.arrayLayers = 1;							// ��1��ֵ��imageCreateInfo������arrayLayers�ֶΣ���ʾͼ����������
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;			// ��VK_SAMPLE_COUNT_1_BIT��ֵ��imageCreateInfo������samples�ֶΣ���ʾͼ��Ĳ�����Ϊ1
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;			// ��VK_IMAGE_TILING_OPTIMAL��ֵ��imageCreateInfo������tiling�ֶΣ���ʾͼ���ƽ�̷�ʽΪ����
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// ��VK_SHARING_MODE_EXCLUSIVE��ֵ��imageCreateInfo������sharingMode�ֶΣ���ʾͼ��Ĺ���ģʽΪ��ռ
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	// ��VK_IMAGE_LAYOUT_UNDEFINED��ֵ��imageCreateInfo������initialLayout�ֶΣ���ʾͼ��ĳ�ʼ����Ϊδ����
			imageCreateInfo.extent = { width, height, 1 };				// ������Ŀ�ȣ��߶Ⱥ�1��ֵ��imageCreateInfo������extent�ֶΣ���ʾͼ��ĳߴ�
			imageCreateInfo.usage = imageUsageFlags;					// ������imageUsageFlags��ֵ��imageCreateInfo������usage�ֶΣ���ʾͼ���ʹ�ñ�־
			if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
			{	// ���ͼ���ʹ�ñ�־������VK_IMAGE_USAGE_TRANSFER_DST_BIT���ͽ�����ӵ�ͼ���ʹ�ñ�־�У���ʾͼ�����ڴ���Ŀ��
				imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}
			VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image));


			// ����ͼ���豸�ڴ�
			vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
			VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));


			// �ݻ���������ͼ��ĸ�������
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
				// ��ktxTexture����Ļ����������iλ���൱�ڳ���2��i�η�������ȡ���ֵΪ1����ֵ��bufferCopyRegion������imageExtent�ֶε�width�ֶΣ���ʾͼ��Ŀ��
				// ��ktxTexture����Ļ����߶�����iλ���൱�ڳ���2��i�η�������ȡ���ֵΪ1����ֵ��bufferCopyRegion������imageExtent�ֶε�height�ֶΣ���ʾͼ��ĸ߶�
				bufferCopyRegion.imageExtent.width = max(1u, ktxTexture->baseWidth >> i);	//1u��һ���޷���������unsigned int������������ʾ��ֵ1
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

			// �������ͷ����������ɾ�ͷ��ݴ滺����
			device->flushCommandBuffer(copyCmd, copyQueue);
			vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
			vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);
		}
		else	// �����ʹ���ݴ滺����
		{
			// ����formatProperties������linearTilingFeatures�ֶΰ���VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT��
			// �����ʾͼ���ʽ��֧������ƽ��
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(device->physicalDevice, format, &formatProperties);
			assert(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

			VkImage mappableImage;			// ����һ��VkImage���͵ı��������ڴ洢��ӳ���ͼ��
			VkDeviceMemory mappableMemory;	// ����һ��VkDeviceMemory���͵ı��������ڴ洢��ӳ����ڴ�

			// ��������ͼ��
			VkImageCreateInfo imageCreateInfo = Cetus::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;				// ��VK_IMAGE_TYPE_2D��ֵ��imageCreateInfo������imageType�ֶΣ���ʾͼ�������Ϊ��ά
			imageCreateInfo.format = format;							// ������format��ֵ��imageCreateInfo������format�ֶΣ���ʾͼ��ĸ�ʽ
			imageCreateInfo.extent = { width, height, 1 };				// ������Ŀ�ȣ��߶Ⱥ�1��ֵ��imageCreateInfo������extent�ֶΣ���ʾͼ��ĳߴ�
			imageCreateInfo.mipLevels = 1;								// ��1��ֵ��imageCreateInfo������mipLevels�ֶΣ���ʾͼ��Ķ༶��Զ�������
			imageCreateInfo.arrayLayers = 1;							// ��1��ֵ��imageCreateInfo������arrayLayers�ֶΣ���ʾͼ����������
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;			// ��VK_SAMPLE_COUNT_1_BIT��ֵ��imageCreateInfo������samples�ֶΣ���ʾͼ��Ĳ�����Ϊ1
			imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;			// ��VK_IMAGE_TILING_LINEAR��ֵ��imageCreateInfo������tiling�ֶΣ���ʾͼ���ƽ�̷�ʽΪ����
			imageCreateInfo.usage = imageUsageFlags;					// ������imageUsageFlags��ֵ��imageCreateInfo������usage�ֶΣ���ʾͼ���ʹ�ñ�־
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// ��VK_SHARING_MODE_EXCLUSIVE��ֵ��imageCreateInfo������sharingMode�ֶΣ���ʾͼ��Ĺ���ģʽΪ��ռ
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	// ��VK_IMAGE_LAYOUT_UNDEFINED��ֵ��imageCreateInfo������initialLayout�ֶΣ���ʾͼ��ĳ�ʼ����Ϊδ����
			VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &mappableImage));

			// ������ӳ��ͼ���ڴ�
			vkGetImageMemoryRequirements(device->logicalDevice, mappableImage, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &mappableMemory));
			VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, mappableImage, mappableMemory, 0));

			// ���ͼ������Դ����
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


		// ����������
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;						// ��VK_FILTER_LINEAR��ֵ��samplerCreateInfo������magFilter�ֶΣ���ʾ�������Ŵ�ʱ��������ʹ�����Թ���
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;						// ��VK_FILTER_LINEAR��ֵ��samplerCreateInfo������minFilter�ֶΣ���ʾ��������Сʱ��������ʹ�����Թ���
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// ��VK_SAMPLER_MIPMAP_MODE_LINEAR��ֵ��samplerCreateInfo������mipmapMode�ֶΣ���ʾ������ʹ�ö༶��Զ����mipmap��ʱ��������ʹ�����Թ���
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// ��VK_SAMPLER_ADDRESS_MODE_REPEAT��ֵ��samplerCreateInfo������addressModeU�ֶΣ���ʾ�������ˮƽ���곬����Χʱ��������ʹ���ظ�ģʽ
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// ��VK_SAMPLER_ADDRESS_MODE_REPEAT��ֵ��samplerCreateInfo������addressModeV�ֶΣ���ʾ������Ĵ�ֱ���곬����Χʱ��������ʹ���ظ�ģʽ
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// ��VK_SAMPLER_ADDRESS_MODE_REPEAT��ֵ��samplerCreateInfo������addressModeW�ֶΣ���ʾ�������������곬����Χʱ��������ʹ���ظ�ģʽ
		samplerCreateInfo.mipLodBias = 0.0f;								// ��0.0f��ֵ��samplerCreateInfo������mipLodBias�ֶΣ���ʾ�������Զ༶��Զ����Ĳ㼶ƫ����Ϊ0
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;					// ��VK_COMPARE_OP_NEVER��ֵ��samplerCreateInfo������compareOp�ֶΣ���ʾ��������ʹ�ñȽϲ���
		samplerCreateInfo.minLod = 0.0f;									// ��0.0f��ֵ��samplerCreateInfo������minLod�ֶΣ���ʾ�������Զ༶��Զ�������С�㼶Ϊ0
		samplerCreateInfo.maxLod = (useStaging) ? (float)mipLevels : 0.0f;	// ����useStaging������ֵ��������Ķ༶��Զ���������0.0f��ֵ��samplerCreateInfo������maxLod�ֶΣ���ʾ�������Զ༶��Զ��������㼶
		// ����device->enabledFeatures.samplerAnisotropy������ֵ����device->properties.limits.maxSamplerAnisotropy��1.0f��ֵ��samplerCreateInfo������maxAnisotropy�ֶΣ���ʾ������������������
		samplerCreateInfo.anisotropyEnable = device->enabledFeatures.samplerAnisotropy;
		samplerCreateInfo.maxAnisotropy = device->enabledFeatures.samplerAnisotropy ? device->properties.limits.maxSamplerAnisotropy : 1.0f;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;	// ��VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE��ֵ��samplerCreateInfo������borderColor�ֶΣ���ʾ����������곬����Χʱ��������ʹ�ð�ɫ�߽���ɫ
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerCreateInfo, nullptr, &sampler));


		// ���������ͼ����ͼ
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

	// �����е��������ݴ�������
	void Texture2D::fromBuffer(void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, 
		Cetus::VulkanDevice *device, VkQueue copyQueue, VkFilter filter, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout)
	{
		assert(buffer);

		this->device = device;
		this->imageLayout = imageLayout;
		width = texWidth;
		height = texHeight;
		mipLevels = 1;

		// ��ʼ���������
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		// ����������
		VkBufferCreateInfo bufferCreateInfo = Cetus::initializers::bufferCreateInfo();
		bufferCreateInfo.size = bufferSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));

		// �����������ڴ�
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);
		VkMemoryAllocateInfo memAllocInfo = Cetus::initializers::memoryAllocateInfo();
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
		VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));

		// �����ݸ��Ƶ��ݴ滺������
		uint8_t *data;
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
		memcpy(data, buffer, bufferSize);
		vkUnmapMemory(device->logicalDevice, stagingMemory);


		// ��������ͼ��
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

		// ��������ͼ���ڴ�
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));


		// ����ͼ���ƣ�����ͼ����������Ϣ
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

		// ���������������
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
		samplerCreateInfo.maxLod = 0.0f;	// ���������
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
		// ������Ϣ��ȡ
		ktxTexture* ktxTexture;
		ktxResult result = loadKTXFile(filename, &ktxTexture);
		assert(result == KTX_SUCCESS);
		ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);


		// �����Ա��ʼ��
		this->device = device;
		this->imageLayout = imageLayout;
		width = ktxTexture->baseWidth;
		height = ktxTexture->baseHeight;
		layerCount = ktxTexture->numLayers;
		mipLevels = ktxTexture->numLevels;
	

		// �����ݴ滺����
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


		// �������ݵ��ݴ滺����
		uint8_t *data;
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
		memcpy(data, ktxTextureData, ktxTextureSize);
		vkUnmapMemory(device->logicalDevice, stagingMemory);


		// ��������ͼ��
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


		// ��������ͼ���ڴ�
		vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));


		// �ݴ滺������ͼ��������
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


		// ����������
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


		// ��������ͼ��
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
		// ���������Ϣ
		ktxTexture* ktxTexture;
		ktxResult result = loadKTXFile(filename, &ktxTexture);
		assert(result == KTX_SUCCESS);
		ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);


		// ��ʼ����Ա����
		this->device = device;
		this->imageLayout = imageLayout;
		width = ktxTexture->baseWidth;
		height = ktxTexture->baseHeight;
		mipLevels = ktxTexture->numLevels;


		// �����ݴ滺����
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


		// ���ݴ滺������������
		uint8_t *data;
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
		memcpy(data, ktxTextureData, ktxTextureSize);
		vkUnmapMemory(device->logicalDevice, stagingMemory);

		
		// ��������ͼ��
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


		// ��������ͼ���ڴ�
		vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));


		// ���ݴ滺�����������ݵ�����ͼ��
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


		// ����������
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


		// ��������ͼ����ͼ
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
