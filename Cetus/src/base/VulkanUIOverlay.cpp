#include "VulkanUIOverlay.h"

namespace Cetus 
{
	UIOverlay::UIOverlay()
	{
		// 调用ImGui::CreateContext函数，创建一个ImGui的上下文，用于存储ImGui的状态和配置
		ImGui::CreateContext();
		// 获取ImGui的样式对象的引用，用于设置ImGui的外观
		ImGuiStyle& style = ImGui::GetStyle();
		// 设置ImGui的各种颜色，用于区分不同的控件和状态
		// 例如，将标题栏的颜色设置为纯红色，将菜单栏的颜色设置为半透明的红色，将按钮的颜色设置为不同透明度的红色，等等
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

	// 准备片段着色器中的字体物理texturesampler，一个sampler包含基本英语字符的图片呢
	void UIOverlay::prepareResources()
	{
		ImGuiIO& io = ImGui::GetIO();	// 从一个文件加载一个字体，并获取字体的原始数据，宽度和高度
		unsigned char* fontData;		// 定义一个unsigned char类型的指针，用于存储字体的原始数据
		int texWidth, texHeight;		// 定义两个int类型的变量，用于存储字体的宽度和高度
		const std::string filename = getAssetPath() + "Roboto-Medium.ttf";
		io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f * scale);
		io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

		ImGuiStyle& style = ImGui::GetStyle();	// 获取ImGui的样式对象的引用，用于设置ImGui的外观
		style.ScaleAllSizes(scale);				// 调用style对象的ScaleAllSizes方法，根据scale，缩放ImGui的所有尺寸


		// 创建字体图像，图像资源与图像视图
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


		// 为图像内存复制资源
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


		// 创建一个采样器，用于对字体图像进行滤波和寻址
		VkSamplerCreateInfo samplerInfo = Cetus::initializers::samplerCreateInfo();
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler));


		// 创建一个描述符池，用于分配描述符集
		std::vector<VkDescriptorPoolSize> poolSizes = {
			Cetus::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)					// 描述符类型与描述符数量
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = Cetus::initializers::descriptorPoolCreateInfo(poolSizes, 2);// 描述符池尺寸与描述符集最大数量	
		VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));


		// 创建一个描述符集布局，用于定义描述符集的结构
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			Cetus::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0), //  类型，阶段，绑定点
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = Cetus::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout));


		// 创建一个描述符集，用于绑定字体图像和视图
		VkDescriptorSetAllocateInfo allocInfo = Cetus::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1); // 该类型描述符集数为1
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &descriptorSet));


		// 关联描述符，图像视图与采样器(采样器图像)
		VkDescriptorImageInfo fontDescriptor = Cetus::initializers::descriptorImageInfo(sampler, fontView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			Cetus::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor)		// 更新的绑定点为零
		};
		vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	// 准备UI图形管线，
	void UIOverlay::preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass, const VkFormat colorFormat, const VkFormat depthFormat)
	{
		// 定义一个推送常量范围，用于在顶点着色器中传递UI的变换矩阵
		VkPushConstantRange pushConstantRange = Cetus::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
		
		// 创建一个管道布局，包含一个描述符集布局和一个推送常量范围
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = Cetus::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// 创建一个输入装配状态，指定使用三角形列表绘制图元
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			Cetus::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

		// 创建一个光栅化状态，指定使用填充模式，不使用背面剔除，使用逆时针方向为正面
		VkPipelineRasterizationStateCreateInfo rasterizationState =
			Cetus::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		// 创建一个颜色混合附件状态，指定使用源颜色的alpha值和目标颜色的1-alpha值进行混合，以实现透明效果
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		// 使用源颜色的1-alpha值作为源因子，使用0作为目标因子，使用加法作为混合操作。
		// 最终的alpha值等于源颜色的alpha值乘以1-alpha值，再加上目标颜色的alpha值乘以0，即Da’ = Sa * (1 - Sa) + Da * 0。
		// 这种混合方式会使源颜色的alpha值越小，目标颜色的alpha值越接近0，也就是越透明。
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo colorBlendState =
			Cetus::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

		// 创建一个深度模板状态，指定不使用深度测试和模板测试
		VkPipelineDepthStencilStateCreateInfo depthStencilState =
			Cetus::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

		// 创建一个视口状态，指定使用一个视口和一个裁剪矩形
		VkPipelineViewportStateCreateInfo viewportState =
			Cetus::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

		// 创建一个多重采样状态，指定使用的采样数
		VkPipelineMultisampleStateCreateInfo multisampleState =
			Cetus::initializers::pipelineMultisampleStateCreateInfo(rasterizationSamples);

		// 创建一个动态状态，指定使用动态视口和动态裁剪矩形
		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState =
			Cetus::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

		// 创建顶点输入特性，
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


		// 创建一个图形管道创建信息，包含管道布局和渲染通道
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

	// 获得UI的全部顶点
	bool UIOverlay::update()
	{
		// 获取ImGui的绘制数据，包含顶点、索引和绘制命令
		ImDrawData* imDrawData = ImGui::GetDrawData();
		bool updateCmdBuffers = false;

		// 如果没有绘制数据，直接返回
		if (!imDrawData) { return false; };

		// 计算顶点缓冲区和索引缓冲区的大小
		VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
		VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

		if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
			return false;
		}

		// 如果顶点缓冲区不存在或者顶点数量不匹配，重新创建顶点缓冲区，并映射到内存
		if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
			vertexBuffer.unmap();
			vertexBuffer.destroy();
			VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &vertexBuffer, vertexBufferSize));
			vertexCount = imDrawData->TotalVtxCount;
			vertexBuffer.unmap();
			vertexBuffer.map();
			updateCmdBuffers = true;
		}

		// 如果索引缓冲区不存在或者索引数量不足，重新创建索引缓冲区，并映射到内存
		if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) {
			indexBuffer.unmap();
			indexBuffer.destroy();
			VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &indexBuffer, indexBufferSize));
			indexCount = imDrawData->TotalIdxCount;
			indexBuffer.map();
			updateCmdBuffers = true;
		}


		// 获取顶点缓冲区和索引缓冲区的指针
		ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffer.mapped;
		ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffer.mapped;
		// 遍历每个绘制列表，将顶点和索引数据复制到缓冲区中
		for (int n = 0; n < imDrawData->CmdListsCount; n++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		// 刷新缓冲区，使数据对GPU可见
		vertexBuffer.flush();
		indexBuffer.flush();

		return updateCmdBuffers;
	}

	// UI窗口绘制命令
	void UIOverlay::draw(const VkCommandBuffer commandBuffer)
	{
		// 获取ImGui的绘制数据，包含顶点、索引和绘制命令
		ImDrawData* imDrawData = ImGui::GetDrawData();
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;
		// 如果没有绘制数据或者绘制列表为空，直接返回
		if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
			return;
		}

		// 获取ImGui的输入输出状态
		// 绑定图形管道，设置为UI覆盖层的渲染状态
		// 绑定描述符集，用于访问字体纹理和推送常量
		ImGuiIO& io = ImGui::GetIO();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

		// 设置推送常量，用于将顶点坐标从ImGui的范围转换到屏幕范围
		pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		pushConstBlock.translate = glm::vec2(-1.0f);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

		// 遍历每个绘制列表，执行每个绘制命令
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
		// 更新ImGui的显示大小，用于适应窗口的变化
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

	// header函数用于创建一个可折叠的标题，参数caption是标题的文本，返回值是标题是否被展开。
	// 它调用了ImGui::CollapsingHeader函数，使用了ImGuiTreeNodeFlags_DefaultOpen标志，表示标题默认是展开的。
	bool UIOverlay::header(const char *caption)
	{
		return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
	}

	// checkBox函数有两个重载，分别用于创建一个复选框，参数caption是复选框的文本，参数value是复选框的状态，返回值是复选框是否被改变。
	// 它们调用了ImGui::Checkbox函数，使用了value的地址作为参数。
	// 如果复选框被改变，它们会将updated变量设为true，表示UIOverlay的状态需要更新。
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

	// radioButton函数用于创建一个单选按钮，参数caption是单选按钮的文本，参数value是单选按钮的状态，返回值是单选按钮是否被选中。
	// 它调用了ImGui::RadioButton函数，使用了value作为参数。
	// 如果单选按钮被选中，它会将updated变量设为true，表示UIOverlay的状态需要更新。
	bool UIOverlay::radioButton(const char* caption, bool value)
	{
		bool res = ImGui::RadioButton(caption, value);
		if (res) { updated = true; };
		return res;
	}

	// inputFloat函数用于创建一个浮点数输入框，参数caption是输入框的文本，参数value是输入框的值，参数step是输入框的步长，参数precision是输入框的精度，返回值是输入框是否被改变。
	// 它调用了ImGui::InputFloat函数，使用了value的地址，step，step * 10.0f和str.c_str()作为参数。
	// str是一个字符串，用于格式化输入框的显示，它根据precision的值生成了一个类似"%.2f"的字符串。
	// 如果输入框被改变，它会将updated变量设为true，表示UIOverlay的状态需要更新。
	bool UIOverlay::inputFloat(const char *caption, float *value, float step, uint32_t precision)
	{
		std::string str = "%." + std::to_string(precision) + "f";
		bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, str.c_str());
		if (res) { updated = true; };
		return res;
	}

	// sliderFloat函数用于创建一个浮点数滑动条，参数caption是滑动条的文本，参数value是滑动条的值，参数min是滑动条的最小值，参数max是滑动条的最大值，返回值是滑动条是否被改变。
	// 它调用了ImGui::SliderFloat函数，使用了value的地址，min和max作为参数。
	// 如果滑动条被改变，它会将updated变量设为true，表示UIOverlay的状态需要更新。
	bool UIOverlay::sliderFloat(const char* caption, float* value, float min, float max)
	{
		bool res = ImGui::SliderFloat(caption, value, min, max);
		if (res) { updated = true; };
		return res;
	}

	// sliderInt函数用于创建一个整数滑动条，参数caption是滑动条的文本，参数value是滑动条的值，参数min是滑动条的最小值，参数max是滑动条的最大值，返回值是滑动条是否被改变。
	// 它调用了ImGui::SliderInt函数，使用了value的地址，min和max作为参数。
	// 如果滑动条被改变，它会将updated变量设为true，表示UIOverlay的状态需要更新。
	bool UIOverlay::sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
	{
		bool res = ImGui::SliderInt(caption, value, min, max);
		if (res) { updated = true; };
		return res;
	}

	// comboBox函数用于创建一个下拉列表，参数caption是下拉列表的文本，参数itemindex是下拉列表的当前选项索引，参数items是下拉列表的所有选项，返回值是下拉列表是否被改变。
	// 它先判断items是否为空，如果为空则返回false。
	// 然后它创建了一个charitems的向量，用于存储items中的字符串的指针。
	// 然后它调用了ImGui::Combo函数，使用了caption，itemindex的地址，charitems的数据指针，charitems的大小和charitems的大小作为参数。
	// 如果下拉列表被改变，它会将updated变量设为true，表示UIOverlay的状态需要更新。
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

	// button函数用于创建一个按钮，参数caption是按钮的文本，返回值是按钮是否被按下。
	// 它调用了ImGui::Button函数，使用了caption作为参数。
	// 如果按钮被按下，它会将updated变量设为true，表示UIOverlay的状态需要更新。
	bool UIOverlay::button(const char *caption)
	{
		bool res = ImGui::Button(caption);
		if (res) { updated = true; };
		return res;
	}

	// colorPicker函数用于创建一个颜色选择器，参数caption是颜色选择器的文本，参数color是颜色选择器的颜色，返回值是颜色选择器是否被改变。
	// 它调用了ImGui::ColorEdit4函数，使用了caption，color的地址和ImGuiColorEditFlags_NoInputs作为参数。
	// ImGuiColorEditFlags_NoInputs表示颜色选择器不显示输入框。
	// 如果颜色选择器被改变，它会将updated变量设为true，表示UIOverlay的状态需要更新。
	bool UIOverlay::colorPicker(const char* caption, float* color) {
		bool res = ImGui::ColorEdit4(caption, color, ImGuiColorEditFlags_NoInputs);
		if (res) { updated = true; };
		return res;
	}

	// text函数用于创建一个文本，参数formatstr是文本的格式字符串，后面的可变参数是文本的参数。
	// 它调用了ImGui::TextV函数，使用了formatstr和args作为参数。
	// args是一个va_list类型的变量，用于存储可变参数。
	// 它先调用va_start函数，使用了formatstr和args作为参数，表示开始处理可变参数。
	// 然后调用va_end函数，使用了args作为参数，表示结束处理可变参数。
	/*
	#include <stdio.h>
	#include <stdarg.h>

	// 定义一个可变参数函数，返回整数的最大值
	int max(int n, ...) {
	  va_list ap; // 定义一个va_list类型的变量
	  va_start(ap, n); // 用n的地址初始化ap，使其指向第一个可变参数
	  int max = 0; // 定义一个变量，用于存储最大值
	  for (int i = 0; i < n; i++) { // 循环n次，获取每个可变参数的值
		int x = va_arg(ap, int); // 用va_arg宏获取下一个整数参数，并赋值给x
		if (x > max) { // 如果x大于max，更新max的值
		  max = x;
		}
	  }
	  va_end(ap); // 用va_end宏结束可变参数的获取
	  return max; // 返回最大值
	}

	int main() {
	  printf("The max of 1, 2, 3 is %d\n", max(3, 1, 2, 3)); // 调用可变参数函数，传递3个整数参数
	  printf("The max of 4, 5, 6, 7 is %d\n", max(4, 4, 5, 6, 7)); // 调用可变参数函数，传递4个整数参数
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
