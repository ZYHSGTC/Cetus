#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <exception>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "base/test/VulkanBase.h"

#define ENABLE_VALIDATION true
#define MAX_CONCURRENT_FRAMES 2

class VulkanExample : public VulkanBase
{
public:
	struct Vertex {
		float position[3];
		float color[3];
	};

	struct BaseBuffer{
		VkDeviceMemory memory;
		VkBuffer buffer;
	} vertices;

	struct {
		VkDeviceMemory memory;
		VkBuffer buffer;
		uint32_t count;
	} indices;

	struct UniformBuffer {
		VkDeviceMemory memory;
		VkBuffer buffer;
		VkDescriptorSet descriptorSet;
		uint8_t* mapped{ nullptr };
	};
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffers;

	struct ShaderData {
		glm::mat4 projectionMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
	};

	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;


	std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> presentCompleteSemaphores;
	std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> renderCompleteSemaphores;
	std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES> commandBuffers;
	std::array<VkFence, MAX_CONCURRENT_FRAMES> waitFences;

	uint32_t currentFrame = 0;

	VulkanExample() : VulkanBase(ENABLE_VALIDATION)
	{
		windowdata.name = "Vulkan Example - Basic indexed triangle";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));
		camera.setRotation(glm::vec3(0.0f));
		camera.setPerspective(60.0f, (float)windowdata.width / (float)windowdata.height, 1.0f, 256.0f);
	}

	~VulkanExample()
	{
		vkDestroyPipeline(device, pipeline, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, vertices.buffer, nullptr);
		vkFreeMemory(device, vertices.memory, nullptr);

		vkDestroyBuffer(device, indices.buffer, nullptr);
		vkFreeMemory(device, indices.memory, nullptr);

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++)
		{
			vkDestroyFence(device, waitFences[i], nullptr);
			vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
			vkDestroyBuffer(device, uniformBuffers[i].buffer, nullptr);
			vkFreeMemory(device, uniformBuffers[i].memory, nullptr);
		}
	}

	void prepare()
	{
		VulkanBase::prepare();
		createDescriptorSetLayout();
		createPipelines();
		createCommandBuffers();
		createVertexBuffer();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createSynchronizationPrimitives();
		renderingAllowed  = true;
	}

	void setupRenderPass()
	{
		std::array<VkAttachmentDescription, 2> attachments{}; // 定义2个附件描述的数组

		// 附件0的描述
		attachments[0].format = swapChain.colorFormat;                                    // 使用交换链选择的颜色格式
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;                                   // 本示例中不使用多重采样
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                              // 在渲染通道开始时清除附件内容
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;                            // 在渲染通道结束后保留其内容（用于显示）
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                   // 不使用模板，所以不关心加载
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;                 // 存储也不关心
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                         // 渲染通道开始时的布局，初始状态不重要，所以使用未定义布局
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;                     // 渲染通道结束时过渡到的布局，用于呈现到交换链

		// 附件1的描述
		attachments[1].format = depthFormat;                                              // 在示例基类中选择了适当的深度格式
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                              // 在第一个子通道开始时清除深度
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;                        // 在渲染通道结束后我们不需要深度（DONT_CARE可能会提高性能）
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                   // 没有模板
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;                 // 没有模板
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                         // 渲染通道开始时的布局，初始状态不重要，所以使用未定义布局
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	  // 过渡到深度/模板附件的布局

		VkAttachmentReference colorReference{};
		colorReference.attachment = 0;												// 附件0是颜色
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;			// 在子通道期间用作颜色的附件布局

		VkAttachmentReference depthReference{};
		depthReference.attachment = 1;												// 附件1是深度
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	// 在子通道期间用作深度/模板的附件布局

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;   // 子通道使用图形管线绑定点
		subpassDescription.colorAttachmentCount = 1;                              // 子通道使用一个颜色附件
		subpassDescription.pColorAttachments = &colorReference;                   // 对颜色附件的引用在插槽0中
		subpassDescription.pDepthStencilAttachment = &depthReference;             // 对深度附件的引用在插槽1中
		subpassDescription.inputAttachmentCount = 0;                              // 输入附件可用于从先前子通道的内容中采样
		subpassDescription.pInputAttachments = nullptr;                           // （本示例中未使用输入附件）
		subpassDescription.preserveAttachmentCount = 0;                           // 保留附件可用于在子通道之间循环（并保留）附件
		subpassDescription.pPreserveAttachments = nullptr;                        // （本示例中未使用保留附件）
		subpassDescription.pResolveAttachments = nullptr;                         // 解析附件在子通道结束时解析，并可用于多重采样等

		std::array<VkSubpassDependency, 2> dependencies;

		// 第一个子通道之前的外部依赖
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dependencies[0].dependencyFlags = 0;

		// 第一个子通道之后的外部依赖
		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = 0;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[1].dependencyFlags = 0;

		VkRenderPassCreateInfo renderPassCI{};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());   // 此渲染通道使用的附件数量
		renderPassCI.pAttachments = attachments.data();                             // 渲染通道使用的附件的描述
		renderPassCI.subpassCount = 1;                                              // 本示例中只使用一个子通道
		renderPassCI.pSubpasses = &subpassDescription;                              // 子通道的描述
		renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());  // 子通道依赖关系的数量
		renderPassCI.pDependencies = dependencies.data();                           // 渲染通道使用的子通道依赖关系
		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderPass));
	}

	void createDescriptorSetLayout()
	{	// 创建描述符集布局和管线布局的函数
		// 创建描述符集布局绑定
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// 描述符类型为Uniform缓冲区
		layoutBinding.descriptorCount = 1;									// 描述符数量为1
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				// 在顶点着色器阶段使用
		layoutBinding.pImmutableSamplers = nullptr;							// 不使用不可变的采样器

		// 创建描述符集布局的信息结构
		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCI.pNext = nullptr;
		descriptorLayoutCI.bindingCount = 1;								// 描述符绑定的数量
		descriptorLayoutCI.pBindings = &layoutBinding;						// 描述符绑定数组
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout)); // 创建描述符集布局

		// 创建管线布局的信息结构
		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.pNext = nullptr;
		pipelineLayoutCI.setLayoutCount = 1;								// 描述符集布局的数量
		pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;				// 描述符集布局数组
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout)); // 创建管线布局
	}

	void createPipelines()
	{	// 创建图形管线的函数
		// 图形管线创建信息结构
		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = pipelineLayout; // 使用的管线布局
		pipelineCI.renderPass = renderPass; // 使用的渲染通道

		// 输入组装状态信息结构
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // 三角形列表拓扑

		// 光栅化状态信息结构
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;			// 多边形填充模式
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;					// 不剔除背面
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// 逆时针为正面
		rasterizationStateCI.depthClampEnable = VK_FALSE;					// 深度裁剪禁用
		rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;			// 光栅化器不丢弃片段
		rasterizationStateCI.depthBiasEnable = VK_FALSE;					// 深度偏移禁用
		rasterizationStateCI.lineWidth = 1.0f;								// 线宽为1.0

		// 颜色融合附件状态信息结构
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = 0xf;							// RGBA所有颜色写入
		blendAttachmentState.blendEnable = VK_FALSE;						// 融合禁用
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentState;

		// 视口状态信息结构
		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;

		// 动态状态信息结构
		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);			// 动态视口
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);			// 动态裁剪
		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// 深度模板状态信息结构
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_TRUE;						// 深度测试启用
		depthStencilStateCI.depthWriteEnable = VK_TRUE;						// 深度写入启用
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;	// 深度比较操作
		depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;				// 深度范围测试禁用
		depthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilStateCI.stencilTestEnable = VK_FALSE;					// 模板测试禁用
		depthStencilStateCI.front = depthStencilStateCI.back;

		// 多重采样状态信息结构
		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// 采样数为1
		multisampleStateCI.pSampleMask = nullptr;

		// 创建顶点输入绑定描述符
		VkVertexInputBindingDescription vertexInputBinding{};
		vertexInputBinding.binding = 0;										// 绑定号，顶点数据在这个绑定上
		vertexInputBinding.stride = sizeof(Vertex);							// 一个顶点数据的字节数
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;			// 控制顶点数据的步进方式，这里每个顶点间的数据是紧密排列的

		// 创建顶点输入属性描述符数组
		std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributes;
		vertexInputAttributes[0].binding = 0;								// 与上面的绑定号对应
		vertexInputAttributes[0].location = 0;								// 顶点着色器中属性的位置指定
		vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;		// 数据格式，这里是三个32位浮点数
		vertexInputAttributes[0].offset = offsetof(Vertex, position);		// 数据在结构体中的偏移量

		vertexInputAttributes[1].binding = 0;								// 与上面的绑定号对应
		vertexInputAttributes[1].location = 1;								// 顶点着色器中属性的位置指定
		vertexInputAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;		// 数据格式，这里是三个32位浮点数
		vertexInputAttributes[1].offset = offsetof(Vertex, color);			// 数据在结构体中的偏移量

		// 创建顶点输入状态信息结构
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
		vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCI.vertexBindingDescriptionCount = 1;				// 顶点绑定描述符的数量
		vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;// 顶点绑定描述符数组
		vertexInputStateCI.vertexAttributeDescriptionCount = 2;				// 顶点属性描述符的数量
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data(); // 顶点属性描述符数组

		// 顶点着色器和片段着色器阶段信息结构
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		shaderStages[0] = loadShader("src/triangle/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("src/triangle/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size()); // 着色器阶段数量
		pipelineCI.pStages = shaderStages.data();							// 着色器阶段数组

		// 设置图形管线的各个状态信息
		pipelineCI.pVertexInputState = &vertexInputStateCI;					// 顶点输入状态信息
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;				// 输入组装状态信息
		pipelineCI.pRasterizationState = &rasterizationStateCI;				// 光栅化状态信息
		pipelineCI.pColorBlendState = &colorBlendStateCI;					// 颜色混合状态信息
		pipelineCI.pMultisampleState = &multisampleStateCI;					// 多采样状态信息
		pipelineCI.pViewportState = &viewportStateCI;						// 视口状态信息
		pipelineCI.pDepthStencilState = &depthStencilStateCI;				// 深度模板状态信息
		pipelineCI.pDynamicState = &dynamicStateCI;							// 动态状态信息

		// 创建图形管线
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

		// 销毁着色器模块
		vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
		vkDestroyShaderModule(device, shaderStages[1].module, nullptr);

	}

	void createCommandBuffers()
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = Cetus::initializers::commandBufferAllocateInfo(cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_CONCURRENT_FRAMES);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, commandBuffers.data()));
	}

	void createVertexBuffer()
	{
		std::vector<Vertex> vertexBuffer{
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
		};
		uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

		std::vector<uint32_t> indexBuffer{ 0, 1, 2 };
		indices.count = static_cast<uint32_t>(indexBuffer.size());
		uint32_t indexBufferSize = indices.count * sizeof(uint32_t);

		VkMemoryAllocateInfo memAlloc{};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs;

		BaseBuffer stagingVerticesBF;
		BaseBuffer stagingIndicesBF;
		void* data;

		// 创建用于顶点数据的暂存缓冲区
		VkBufferCreateInfo vertexBufferInfoCI{};
		vertexBufferInfoCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfoCI.size = vertexBufferSize;
		vertexBufferInfoCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &stagingVerticesBF.buffer));

		vkGetBufferMemoryRequirements(device, stagingVerticesBF.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingVerticesBF.memory));

		VK_CHECK_RESULT(vkMapMemory(device, stagingVerticesBF.memory, 0, memAlloc.allocationSize, 0, &data));
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(device, stagingVerticesBF.memory);
		VK_CHECK_RESULT(vkBindBufferMemory(device, stagingVerticesBF.buffer, stagingVerticesBF.memory, 0));

		// 创建用于顶点数据的目标缓冲区
		vertexBufferInfoCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &vertices.buffer));
		vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

		// 创建用于索引数据的暂存缓冲区
		VkBufferCreateInfo indexbufferCI{};
		indexbufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferCI.size = indexBufferSize;
		indexbufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &stagingIndicesBF.buffer));

		vkGetBufferMemoryRequirements(device, stagingIndicesBF.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingIndicesBF.memory));

		VK_CHECK_RESULT(vkMapMemory(device, stagingIndicesBF.memory, 0, indexBufferSize, 0, &data));
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(device, stagingIndicesBF.memory);
		VK_CHECK_RESULT(vkBindBufferMemory(device, stagingIndicesBF.buffer, stagingIndicesBF.memory, 0));

		// 创建用于索引数据的目标缓冲区
		indexbufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &indices.buffer));
		vkGetBufferMemoryRequirements(device, indices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &indices.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, indices.buffer, indices.memory, 0));

		VkCommandBuffer copyCmd;

		VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = cmdPool;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));
		VkCommandBufferBeginInfo cmdBufInfo = Cetus::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
		
		// 拷贝顶点数据到目标缓冲区，拷贝索引数据到目标缓冲区
		VkBufferCopy copyRegion{};
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingVerticesBF.buffer, vertices.buffer, 1, &copyRegion);
		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingIndicesBF.buffer, indices.buffer, 1, &copyRegion);
		VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

		// 提交拷贝命令到队列并等待完成
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyCmd;
		VkFenceCreateInfo fenceCI{};
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.flags = 0;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

		// 清理资源
		vkDestroyFence(device, fence, nullptr);
		vkFreeCommandBuffers(device, cmdPool, 1, &copyCmd);
		vkDestroyBuffer(device, stagingVerticesBF.buffer, nullptr);
		vkFreeMemory(device, stagingVerticesBF.memory, nullptr);
		vkDestroyBuffer(device, stagingIndicesBF.buffer, nullptr);
		vkFreeMemory(device, stagingIndicesBF.memory, nullptr);
	}

	void createUniformBuffers()
	{
		VkBufferCreateInfo bufferInfo{};		// 定义缓冲区信息结构体
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(ShaderData);
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VkMemoryAllocateInfo allocInfo{};		// 定义内存分配信息结构体
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = 0;
		allocInfo.memoryTypeIndex = 0;
		VkMemoryRequirements memReqs;

		// 循环创建多个帧的Uniform缓冲区
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffers[i].buffer));	// 创建Uniform缓冲区
			vkGetBufferMemoryRequirements(device, uniformBuffers[i].buffer, &memReqs);					// 获取缓冲区的内存需求
			allocInfo.allocationSize = memReqs.size;													// 配置内存分配信息
			allocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &(uniformBuffers[i].memory)));
			VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffers[i].buffer, uniformBuffers[i].memory, 0));// 将内存与缓冲区绑定
			VK_CHECK_RESULT(vkMapMemory(device, uniformBuffers[i].memory, 0, sizeof(ShaderData), 0, (void**)&uniformBuffers[i].mapped));// 映射内存到CPU可访问的指针
		}
	}

	void createDescriptorPool()
	{
		// 定义描述符池中描述符类型和数量，uniform类型的描述符有两个。
		VkDescriptorPoolSize descriptorTypeCounts[1];
		descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorTypeCounts[0].descriptorCount = MAX_CONCURRENT_FRAMES;

		// 初始化描述符池信息结构体
		VkDescriptorPoolCreateInfo descriptorPoolCI{};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.pNext = nullptr;
		descriptorPoolCI.poolSizeCount = 1;
		descriptorPoolCI.pPoolSizes = descriptorTypeCounts;
		descriptorPoolCI.maxSets = MAX_CONCURRENT_FRAMES;

		// 创建描述符池
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));
	}

	void createDescriptorSets()
	{	// 使用循环为每一帧创建描述符集合
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			// 创建描述符集合分配信息结构体
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;			// 描述符集合分配所使用的描述符池
			allocInfo.descriptorSetCount = 1;					// 要分配的描述符集合数量
			allocInfo.pSetLayouts = &descriptorSetLayout;		// 描述符集合的布局
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffers[i].descriptorSet)); // 分配描述符集合

			// 创建写入描述符集合的结构体
			VkWriteDescriptorSet writeDescriptorSet{};

			// 创建描述符缓冲信息结构体
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i].buffer;		// 描述符关联的缓冲对象
			bufferInfo.range = sizeof(ShaderData);				// 描述符关联的缓冲对象的大小

			// 填充写入描述符集合的结构体
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = uniformBuffers[i].descriptorSet; // 目标描述符集合
			writeDescriptorSet.dstBinding = 0;					// 描述符集合中的绑定点
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // 描述符类型为Uniform缓冲
			writeDescriptorSet.descriptorCount = 1;				// 要更新的描述符数量
			writeDescriptorSet.pBufferInfo = &bufferInfo;		// 指向描述符缓冲信息的指针
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr); // 更新描述符集合
		}
	}

	void createSynchronizationPrimitives()
	{
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {

			VkSemaphoreCreateInfo semaphoreCI{};
			semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &presentCompleteSemaphores[i]));
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &renderCompleteSemaphores[i]));

			VkFenceCreateInfo fenceCI{};
			fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &waitFences[i]));

		}
	}

	virtual void render()
	{
		if (!renderingAllowed) return;					// 如果不允许渲染，则直接返回
		
		vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX);		// 等待当前帧的fence信号，确保前一帧的渲染已完成
		
		uint32_t imageIndex;							// 获取下一个图像，即从交换链中获取可写的图像索引
		VkResult result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {		// 如果返回错误表明交换链不再有效，可能是窗口大小变化等情况，重新处理窗口大小
			windowResize();								
			return;
		}
		else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)) {				// 如果获取图像失败，则抛出异常
			throw "Could not acquire the next swap chain image!";
		}

		
		ShaderData shaderData{};						// 准备传递给着色器的Uniform数据
		shaderData.projectionMatrix = camera.matrices.perspective;
		shaderData.viewMatrix = camera.matrices.view;
		shaderData.modelMatrix = glm::mat4(1.0f);
		memcpy(uniformBuffers[currentBuffer].mapped, &shaderData, sizeof(ShaderData));	// 将Uniform数据拷贝到当前帧对应的缓冲对象中

		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentFrame]));			// 重置当前帧的fence信号
		vkResetCommandBuffer(commandBuffers[currentBuffer], 0);							// 重置当前帧对应的命令缓冲，准备开始记录命令
		
		VkCommandBufferBeginInfo cmdBufInfo{};			// 设置命令缓冲的开始信息
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		
		VkClearValue clearValues[2];					// 设置清除值，即清除颜色和深度/模板缓冲
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };
		VkRenderPassBeginInfo renderPassBeginInfo{};	// 设置渲染通道的开始信息
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = windowdata.width;
		renderPassBeginInfo.renderArea.extent.height = windowdata.height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = frameBuffers[imageIndex];
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffers[currentBuffer], &cmdBufInfo));// 开始记录命令缓冲
		vkCmdBeginRenderPass(commandBuffers[currentBuffer], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		VkViewport viewport{};							// 创建并设置视口
		viewport.height = (float)windowdata.height;
		viewport.width = (float)windowdata.width;
		viewport.minDepth = (float)0.0f;
		viewport.maxDepth = (float)1.0f;
		vkCmdSetViewport(commandBuffers[currentBuffer], 0, 1, &viewport);
		VkRect2D scissor{};								// 创建并设置裁剪矩形
		scissor.extent.width = windowdata.width;
		scissor.extent.height = windowdata.height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(commandBuffers[currentBuffer], 0, 1, &scissor);						// 绑定描述符集到图形管线
		vkCmdBindDescriptorSets(commandBuffers[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uniformBuffers[currentBuffer].descriptorSet, 0, nullptr);
		vkCmdBindPipeline(commandBuffers[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);// 绑定图形管线
		VkDeviceSize offsets[1]{ 0 };					// 绑定顶点缓冲，绑定索引缓冲
		vkCmdBindVertexBuffers(commandBuffers[currentBuffer], 0, 1, &vertices.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffers[currentBuffer], indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffers[currentBuffer], indices.count, 1, 0, 0, 1);			// 执行索引绘制命令
		vkCmdEndRenderPass(commandBuffers[currentBuffer]);									// 结束渲染通道
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffers[currentBuffer]));					// 结束命令缓冲的记录

		// 指定等待阶段的掩码为颜色附着输出阶段
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		
		VkSubmitInfo submitInfo{};						// 创建提交信息结构体
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &waitStageMask;	// 指定信号等待阶段的掩码，即提交等待信号的管线阶段
		submitInfo.waitSemaphoreCount = 1;				// 设置等待的信号量数量和数组
		submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentFrame];
		submitInfo.signalSemaphoreCount = 1;			// 设置信号的信号量数量和数组
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentFrame];
		submitInfo.commandBufferCount = 1;				// 设置要执行的命令缓冲数组及数量
		submitInfo.pCommandBuffers = &commandBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentFrame]));	// 提交命令缓冲到队列，同时传递等待信号、信号和同步的fence

		// 创建呈现信息结构体
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;				// 设置等待的信号量数量和数组
		presentInfo.pWaitSemaphores = &renderCompleteSemaphores[currentFrame];
		presentInfo.swapchainCount = 1;					// 设置交换链数量和数组、以及图像索引
		presentInfo.pSwapchains = &swapChain.swapChain;
		presentInfo.pImageIndices = &imageIndex;
		result = vkQueuePresentKHR(queue, &presentInfo);// 使用交换链呈现图像

		// 检查呈现结果，如果交换链不再有效或者不理想，触发窗口大小变化
		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
			windowResize();
		}
		else if (result != VK_SUCCESS) {				// 如果呈现图像失败，则抛出异常
			throw "Could not present the image to the swap chain!";
		}
	}
};

VulkanExample* vulkanExample;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow(hInstance, WndProc);
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
