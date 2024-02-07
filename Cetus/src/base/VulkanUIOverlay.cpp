#include "VulkanUIOverlay.h"

namespace Cetus 
{
	UIOverlay::UIOverlay()
	{
		// ����ImGui::CreateContext����������һ��ImGui�������ģ����ڴ洢ImGui��״̬������
		ImGui::CreateContext();
		// ��ȡImGui����ʽ��������ã���������ImGui�����
		ImGuiStyle& style = ImGui::GetStyle();
		// ����ImGui�ĸ�����ɫ���������ֲ�ͬ�Ŀؼ���״̬
		// ���磬������������ɫ����Ϊ����ɫ�����˵�������ɫ����Ϊ��͸���ĺ�ɫ������ť����ɫ����Ϊ��ͬ͸���ȵĺ�ɫ���ȵ�
		style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
		style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		ImGuiIO& io = ImGui::GetIO();
		io.FontGlobalScale = scale;
	}

	UIOverlay::~UIOverlay()	{
		if (ImGui::GetCurrentContext()) {
			ImGui::DestroyContext();
		}
	}

	// ׼��Ƭ����ɫ���е���������texturesampler��һ��sampler��������Ӣ���ַ���ͼƬ��
	void UIOverlay::prepareResources()
	{
		ImGuiIO& io = ImGui::GetIO();	// ��һ���ļ�����һ�����壬����ȡ�����ԭʼ���ݣ���Ⱥ͸߶�
		unsigned char* fontData;		// ����һ��unsigned char���͵�ָ�룬���ڴ洢�����ԭʼ����
		int texWidth, texHeight;		// ��������int���͵ı��������ڴ洢����Ŀ�Ⱥ͸߶�
		const std::string filename = getAssetPath() + "Roboto-Medium.ttf";
		io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f * scale);
		io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

		ImGuiStyle& style = ImGui::GetStyle();	// ��ȡImGui����ʽ��������ã���������ImGui�����
		style.ScaleAllSizes(scale);				// ����style�����ScaleAllSizes����������scale������ImGui�����гߴ�


		// ��������ͼ��ͼ����Դ��ͼ����ͼ
		VkImageCreateInfo imageInfo = Cetus::initializers::imageCreateInfo();
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.extent.width = texWidth;
		imageInfo.extent.height = texHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageInfo, nullptr, &fontImage));
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device->logicalDevice, fontImage, &memReqs);
		VkMemoryAllocateInfo memAllocInfo = Cetus::initializers::memoryAllocateInfo();
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &fontMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, fontImage, fontMemory, 0));

		VkImageViewCreateInfo viewInfo = Cetus::initializers::imageViewCreateInfo();
		viewInfo.image = fontImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &fontView));


		// Ϊͼ���ڴ渴����Դ
		Cetus::Buffer stagingBuffer;
		VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, uploadSize));

		stagingBuffer.map();
		memcpy(stagingBuffer.mapped, fontData, uploadSize);
		stagingBuffer.unmap();

		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		Cetus::tools::setImageLayout(
			copyCmd,
			fontImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = texWidth;
		bufferCopyRegion.imageExtent.height = texHeight;
		bufferCopyRegion.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer.buffer,
			fontImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion
		);

		Cetus::tools::setImageLayout(
			copyCmd,
			fontImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		device->flushCommandBuffer(copyCmd, queue, true);
		stagingBuffer.destroy();


		// ����һ�������������ڶ�����ͼ������˲���Ѱַ
		VkSamplerCreateInfo samplerInfo = Cetus::initializers::samplerCreateInfo();
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler));


		// ����һ���������أ����ڷ�����������
		std::vector<VkDescriptorPoolSize> poolSizes = {
			Cetus::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)					// ����������������������
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = Cetus::initializers::descriptorPoolCreateInfo(poolSizes, 2);// �������سߴ������������������	
		VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));


		// ����һ�������������֣����ڶ������������Ľṹ
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			Cetus::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0), //  ���ͣ��׶Σ��󶨵�
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = Cetus::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout));


		// ����һ���������������ڰ�����ͼ�����ͼ
		VkDescriptorSetAllocateInfo allocInfo = Cetus::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1); // ����������������Ϊ1
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &descriptorSet));


		// ������������ͼ����ͼ�������(������ͼ��)
		VkDescriptorImageInfo fontDescriptor = Cetus::initializers::descriptorImageInfo(sampler, fontView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			Cetus::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor)		// ���µİ󶨵�Ϊ��
		};
		vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	// ׼��UIͼ�ι��ߣ�
	void UIOverlay::preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass, const VkFormat colorFormat, const VkFormat depthFormat)
	{
		// ����һ�����ͳ�����Χ�������ڶ�����ɫ���д���UI�ı任����
		VkPushConstantRange pushConstantRange = Cetus::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
		
		// ����һ���ܵ����֣�����һ�������������ֺ�һ�����ͳ�����Χ
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = Cetus::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// ����һ������װ��״̬��ָ��ʹ���������б����ͼԪ
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			Cetus::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

		// ����һ����դ��״̬��ָ��ʹ�����ģʽ����ʹ�ñ����޳���ʹ����ʱ�뷽��Ϊ����
		VkPipelineRasterizationStateCreateInfo rasterizationState =
			Cetus::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		// ����һ����ɫ��ϸ���״̬��ָ��ʹ��Դ��ɫ��alphaֵ��Ŀ����ɫ��1-alphaֵ���л�ϣ���ʵ��͸��Ч��
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		// ʹ��Դ��ɫ��1-alphaֵ��ΪԴ���ӣ�ʹ��0��ΪĿ�����ӣ�ʹ�üӷ���Ϊ��ϲ�����
		// ���յ�alphaֵ����Դ��ɫ��alphaֵ����1-alphaֵ���ټ���Ŀ����ɫ��alphaֵ����0����Da�� = Sa * (1 - Sa) + Da * 0��
		// ���ֻ�Ϸ�ʽ��ʹԴ��ɫ��alphaֵԽС��Ŀ����ɫ��alphaֵԽ�ӽ�0��Ҳ����Խ͸����
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo colorBlendState =
			Cetus::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

		// ����һ�����ģ��״̬��ָ����ʹ����Ȳ��Ժ�ģ�����
		VkPipelineDepthStencilStateCreateInfo depthStencilState =
			Cetus::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

		// ����һ���ӿ�״̬��ָ��ʹ��һ���ӿں�һ���ü�����
		VkPipelineViewportStateCreateInfo viewportState =
			Cetus::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

		// ����һ�����ز���״̬��ָ��ʹ�õĲ�����
		VkPipelineMultisampleStateCreateInfo multisampleState =
			Cetus::initializers::pipelineMultisampleStateCreateInfo(rasterizationSamples);

		// ����һ����̬״̬��ָ��ʹ�ö�̬�ӿںͶ�̬�ü�����
		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState =
			Cetus::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

		// ���������������ԣ�
		std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
			Cetus::initializers::vertexInputBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
		};
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			Cetus::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),	
			Cetus::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),	
			Cetus::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),
		};
		VkPipelineVertexInputStateCreateInfo vertexInputState = Cetus::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();


		// ����һ��ͼ�ιܵ�������Ϣ�������ܵ����ֺ���Ⱦͨ��
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = Cetus::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaders.size());
		pipelineCreateInfo.pStages = shaders.data();
		pipelineCreateInfo.subpass = subpass;
		pipelineCreateInfo.pVertexInputState = &vertexInputState;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}

	// ���UI��ȫ������
	bool UIOverlay::update()
	{
		// ��ȡImGui�Ļ������ݣ��������㡢�����ͻ�������
		ImDrawData* imDrawData = ImGui::GetDrawData();
		bool updateCmdBuffers = false;

		// ���û�л������ݣ�ֱ�ӷ���
		if (!imDrawData) { return false; };

		// ���㶥�㻺�����������������Ĵ�С
		VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
		VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

		if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
			return false;
		}

		// ������㻺���������ڻ��߶���������ƥ�䣬���´������㻺��������ӳ�䵽�ڴ�
		if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
			vertexBuffer.unmap();
			vertexBuffer.destroy();
			VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &vertexBuffer, vertexBufferSize));
			vertexCount = imDrawData->TotalVtxCount;
			vertexBuffer.unmap();
			vertexBuffer.map();
			updateCmdBuffers = true;
		}

		// ������������������ڻ��������������㣬���´�����������������ӳ�䵽�ڴ�
		if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) {
			indexBuffer.unmap();
			indexBuffer.destroy();
			VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &indexBuffer, indexBufferSize));
			indexCount = imDrawData->TotalIdxCount;
			indexBuffer.map();
			updateCmdBuffers = true;
		}


		// ��ȡ���㻺������������������ָ��
		ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffer.mapped;
		ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffer.mapped;
		// ����ÿ�������б���������������ݸ��Ƶ���������
		for (int n = 0; n < imDrawData->CmdListsCount; n++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		// ˢ�»�������ʹ���ݶ�GPU�ɼ�
		vertexBuffer.flush();
		indexBuffer.flush();

		return updateCmdBuffers;
	}

	// UI���ڻ�������
	void UIOverlay::draw(const VkCommandBuffer commandBuffer)
	{
		// ��ȡImGui�Ļ������ݣ��������㡢�����ͻ�������
		ImDrawData* imDrawData = ImGui::GetDrawData();
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;
		// ���û�л������ݻ��߻����б�Ϊ�գ�ֱ�ӷ���
		if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
			return;
		}

		// ��ȡImGui���������״̬
		// ��ͼ�ιܵ�������ΪUI���ǲ����Ⱦ״̬
		// ���������������ڷ���������������ͳ���
		ImGuiIO& io = ImGui::GetIO();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

		// �������ͳ��������ڽ����������ImGui�ķ�Χת������Ļ��Χ
		pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		pushConstBlock.translate = glm::vec2(-1.0f);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

		// ����ÿ�������б�ִ��ÿ����������
		for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
		{
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
				VkRect2D scissorRect;
				scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
				scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
				scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
				vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += pcmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
	}

	void UIOverlay::resize(uint32_t width, uint32_t height)
	{
		// ����ImGui����ʾ��С��������Ӧ���ڵı仯
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)(width), (float)(height));
	}

	void UIOverlay::freeResources()
	{
		vertexBuffer.destroy();
		indexBuffer.destroy();
		vkDestroyImageView(device->logicalDevice, fontView, nullptr);
		vkDestroyImage(device->logicalDevice, fontImage, nullptr);
		vkFreeMemory(device->logicalDevice, fontMemory, nullptr);
		vkDestroySampler(device->logicalDevice, sampler, nullptr);
		vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
		vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
		vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
	}

	// header�������ڴ���һ�����۵��ı��⣬����caption�Ǳ�����ı�������ֵ�Ǳ����Ƿ�չ����
	// ��������ImGui::CollapsingHeader������ʹ����ImGuiTreeNodeFlags_DefaultOpen��־����ʾ����Ĭ����չ���ġ�
	bool UIOverlay::header(const char *caption)
	{
		return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
	}

	// checkBox�������������أ��ֱ����ڴ���һ����ѡ�򣬲���caption�Ǹ�ѡ����ı�������value�Ǹ�ѡ���״̬������ֵ�Ǹ�ѡ���Ƿ񱻸ı䡣
	// ���ǵ�����ImGui::Checkbox������ʹ����value�ĵ�ַ��Ϊ������
	// �����ѡ�򱻸ı䣬���ǻὫupdated������Ϊtrue����ʾUIOverlay��״̬��Ҫ���¡�
	bool UIOverlay::checkBox(const char *caption, bool *value)
	{
		bool res = ImGui::Checkbox(caption, value);
		if (res) { updated = true; };
		return res;
	}

	bool UIOverlay::checkBox(const char *caption, int32_t *value)
	{
		bool val = (*value == 1);
		bool res = ImGui::Checkbox(caption, &val);
		*value = val;
		if (res) { updated = true; };
		return res;
	}

	// radioButton�������ڴ���һ����ѡ��ť������caption�ǵ�ѡ��ť���ı�������value�ǵ�ѡ��ť��״̬������ֵ�ǵ�ѡ��ť�Ƿ�ѡ�С�
	// ��������ImGui::RadioButton������ʹ����value��Ϊ������
	// �����ѡ��ť��ѡ�У����Ὣupdated������Ϊtrue����ʾUIOverlay��״̬��Ҫ���¡�
	bool UIOverlay::radioButton(const char* caption, bool value)
	{
		bool res = ImGui::RadioButton(caption, value);
		if (res) { updated = true; };
		return res;
	}

	// inputFloat�������ڴ���һ������������򣬲���caption���������ı�������value��������ֵ������step�������Ĳ���������precision�������ľ��ȣ�����ֵ��������Ƿ񱻸ı䡣
	// ��������ImGui::InputFloat������ʹ����value�ĵ�ַ��step��step * 10.0f��str.c_str()��Ϊ������
	// str��һ���ַ��������ڸ�ʽ����������ʾ��������precision��ֵ������һ������"%.2f"���ַ�����
	// �������򱻸ı䣬���Ὣupdated������Ϊtrue����ʾUIOverlay��״̬��Ҫ���¡�
	bool UIOverlay::inputFloat(const char *caption, float *value, float step, uint32_t precision)
	{
		std::string str = "%." + std::to_string(precision) + "f";
		bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, str.c_str());
		if (res) { updated = true; };
		return res;
	}

	// sliderFloat�������ڴ���һ��������������������caption�ǻ��������ı�������value�ǻ�������ֵ������min�ǻ���������Сֵ������max�ǻ����������ֵ������ֵ�ǻ������Ƿ񱻸ı䡣
	// ��������ImGui::SliderFloat������ʹ����value�ĵ�ַ��min��max��Ϊ������
	// ������������ı䣬���Ὣupdated������Ϊtrue����ʾUIOverlay��״̬��Ҫ���¡�
	bool UIOverlay::sliderFloat(const char* caption, float* value, float min, float max)
	{
		bool res = ImGui::SliderFloat(caption, value, min, max);
		if (res) { updated = true; };
		return res;
	}

	// sliderInt�������ڴ���һ������������������caption�ǻ��������ı�������value�ǻ�������ֵ������min�ǻ���������Сֵ������max�ǻ����������ֵ������ֵ�ǻ������Ƿ񱻸ı䡣
	// ��������ImGui::SliderInt������ʹ����value�ĵ�ַ��min��max��Ϊ������
	// ������������ı䣬���Ὣupdated������Ϊtrue����ʾUIOverlay��״̬��Ҫ���¡�
	bool UIOverlay::sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
	{
		bool res = ImGui::SliderInt(caption, value, min, max);
		if (res) { updated = true; };
		return res;
	}

	// comboBox�������ڴ���һ�������б�����caption�������б���ı�������itemindex�������б�ĵ�ǰѡ������������items�������б������ѡ�����ֵ�������б��Ƿ񱻸ı䡣
	// �����ж�items�Ƿ�Ϊ�գ����Ϊ���򷵻�false��
	// Ȼ����������һ��charitems�����������ڴ洢items�е��ַ�����ָ�롣
	// Ȼ����������ImGui::Combo������ʹ����caption��itemindex�ĵ�ַ��charitems������ָ�룬charitems�Ĵ�С��charitems�Ĵ�С��Ϊ������
	// ��������б��ı䣬���Ὣupdated������Ϊtrue����ʾUIOverlay��״̬��Ҫ���¡�
	bool UIOverlay::comboBox(const char *caption, int32_t *itemindex, std::vector<std::string> items)
	{
		if (items.empty()) {
			return false;
		}
		std::vector<const char*> charitems;
		charitems.reserve(items.size());
		for (size_t i = 0; i < items.size(); i++) {
			charitems.push_back(items[i].c_str());
		}
		uint32_t itemCount = static_cast<uint32_t>(charitems.size());
		bool res = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
		if (res) { updated = true; };
		return res;
	}

	// button�������ڴ���һ����ť������caption�ǰ�ť���ı�������ֵ�ǰ�ť�Ƿ񱻰��¡�
	// ��������ImGui::Button������ʹ����caption��Ϊ������
	// �����ť�����£����Ὣupdated������Ϊtrue����ʾUIOverlay��״̬��Ҫ���¡�
	bool UIOverlay::button(const char *caption)
	{
		bool res = ImGui::Button(caption);
		if (res) { updated = true; };
		return res;
	}

	// colorPicker�������ڴ���һ����ɫѡ����������caption����ɫѡ�������ı�������color����ɫѡ��������ɫ������ֵ����ɫѡ�����Ƿ񱻸ı䡣
	// ��������ImGui::ColorEdit4������ʹ����caption��color�ĵ�ַ��ImGuiColorEditFlags_NoInputs��Ϊ������
	// ImGuiColorEditFlags_NoInputs��ʾ��ɫѡ��������ʾ�����
	// �����ɫѡ�������ı䣬���Ὣupdated������Ϊtrue����ʾUIOverlay��״̬��Ҫ���¡�
	bool UIOverlay::colorPicker(const char* caption, float* color) {
		bool res = ImGui::ColorEdit4(caption, color, ImGuiColorEditFlags_NoInputs);
		if (res) { updated = true; };
		return res;
	}

	// text�������ڴ���һ���ı�������formatstr���ı��ĸ�ʽ�ַ���������Ŀɱ�������ı��Ĳ�����
	// ��������ImGui::TextV������ʹ����formatstr��args��Ϊ������
	// args��һ��va_list���͵ı��������ڴ洢�ɱ������
	// ���ȵ���va_start������ʹ����formatstr��args��Ϊ��������ʾ��ʼ����ɱ������
	// Ȼ�����va_end������ʹ����args��Ϊ��������ʾ��������ɱ������
	/*
	#include <stdio.h>
	#include <stdarg.h>

	// ����һ���ɱ�����������������������ֵ
	int max(int n, ...) {
	  va_list ap; // ����һ��va_list���͵ı���
	  va_start(ap, n); // ��n�ĵ�ַ��ʼ��ap��ʹ��ָ���һ���ɱ����
	  int max = 0; // ����һ�����������ڴ洢���ֵ
	  for (int i = 0; i < n; i++) { // ѭ��n�Σ���ȡÿ���ɱ������ֵ
		int x = va_arg(ap, int); // ��va_arg���ȡ��һ����������������ֵ��x
		if (x > max) { // ���x����max������max��ֵ
		  max = x;
		}
	  }
	  va_end(ap); // ��va_end������ɱ�����Ļ�ȡ
	  return max; // �������ֵ
	}

	int main() {
	  printf("The max of 1, 2, 3 is %d\n", max(3, 1, 2, 3)); // ���ÿɱ��������������3����������
	  printf("The max of 4, 5, 6, 7 is %d\n", max(4, 4, 5, 6, 7)); // ���ÿɱ��������������4����������
	  return 0;
	}*/
	void UIOverlay::text(const char *formatstr, ...)
	{
		va_list args;
		va_start(args, formatstr);
		ImGui::TextV(formatstr, args);
		va_end(args);
	}
}
