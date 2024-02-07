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
		std::array<VkAttachmentDescription, 2> attachments{}; // ����2����������������

		// ����0������
		attachments[0].format = swapChain.colorFormat;                                    // ʹ�ý�����ѡ�����ɫ��ʽ
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;                                   // ��ʾ���в�ʹ�ö��ز���
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                              // ����Ⱦͨ����ʼʱ�����������
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;                            // ����Ⱦͨ���������������ݣ�������ʾ��
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                   // ��ʹ��ģ�壬���Բ����ļ���
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;                 // �洢Ҳ������
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                         // ��Ⱦͨ����ʼʱ�Ĳ��֣���ʼ״̬����Ҫ������ʹ��δ���岼��
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;                     // ��Ⱦͨ������ʱ���ɵ��Ĳ��֣����ڳ��ֵ�������

		// ����1������
		attachments[1].format = depthFormat;                                              // ��ʾ��������ѡ�����ʵ�����ȸ�ʽ
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                              // �ڵ�һ����ͨ����ʼʱ������
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;                        // ����Ⱦͨ�����������ǲ���Ҫ��ȣ�DONT_CARE���ܻ�������ܣ�
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                   // û��ģ��
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;                 // û��ģ��
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                         // ��Ⱦͨ����ʼʱ�Ĳ��֣���ʼ״̬����Ҫ������ʹ��δ���岼��
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	  // ���ɵ����/ģ�帽���Ĳ���

		VkAttachmentReference colorReference{};
		colorReference.attachment = 0;												// ����0����ɫ
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;			// ����ͨ���ڼ�������ɫ�ĸ�������

		VkAttachmentReference depthReference{};
		depthReference.attachment = 1;												// ����1�����
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	// ����ͨ���ڼ��������/ģ��ĸ�������

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;   // ��ͨ��ʹ��ͼ�ι��߰󶨵�
		subpassDescription.colorAttachmentCount = 1;                              // ��ͨ��ʹ��һ����ɫ����
		subpassDescription.pColorAttachments = &colorReference;                   // ����ɫ�����������ڲ��0��
		subpassDescription.pDepthStencilAttachment = &depthReference;             // ����ȸ����������ڲ��1��
		subpassDescription.inputAttachmentCount = 0;                              // ���븽�������ڴ���ǰ��ͨ���������в���
		subpassDescription.pInputAttachments = nullptr;                           // ����ʾ����δʹ�����븽����
		subpassDescription.preserveAttachmentCount = 0;                           // ������������������ͨ��֮��ѭ����������������
		subpassDescription.pPreserveAttachments = nullptr;                        // ����ʾ����δʹ�ñ���������
		subpassDescription.pResolveAttachments = nullptr;                         // ������������ͨ������ʱ�������������ڶ��ز�����

		std::array<VkSubpassDependency, 2> dependencies;

		// ��һ����ͨ��֮ǰ���ⲿ����
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dependencies[0].dependencyFlags = 0;

		// ��һ����ͨ��֮����ⲿ����
		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = 0;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[1].dependencyFlags = 0;

		VkRenderPassCreateInfo renderPassCI{};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());   // ����Ⱦͨ��ʹ�õĸ�������
		renderPassCI.pAttachments = attachments.data();                             // ��Ⱦͨ��ʹ�õĸ���������
		renderPassCI.subpassCount = 1;                                              // ��ʾ����ֻʹ��һ����ͨ��
		renderPassCI.pSubpasses = &subpassDescription;                              // ��ͨ��������
		renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());  // ��ͨ��������ϵ������
		renderPassCI.pDependencies = dependencies.data();                           // ��Ⱦͨ��ʹ�õ���ͨ��������ϵ
		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderPass));
	}

	void createDescriptorSetLayout()
	{	// ���������������ֺ͹��߲��ֵĺ���
		// ���������������ְ�
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// ����������ΪUniform������
		layoutBinding.descriptorCount = 1;									// ����������Ϊ1
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				// �ڶ�����ɫ���׶�ʹ��
		layoutBinding.pImmutableSamplers = nullptr;							// ��ʹ�ò��ɱ�Ĳ�����

		// ���������������ֵ���Ϣ�ṹ
		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCI.pNext = nullptr;
		descriptorLayoutCI.bindingCount = 1;								// �������󶨵�����
		descriptorLayoutCI.pBindings = &layoutBinding;						// ������������
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout)); // ����������������

		// �������߲��ֵ���Ϣ�ṹ
		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.pNext = nullptr;
		pipelineLayoutCI.setLayoutCount = 1;								// �����������ֵ�����
		pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;				// ����������������
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout)); // �������߲���
	}

	void createPipelines()
	{	// ����ͼ�ι��ߵĺ���
		// ͼ�ι��ߴ�����Ϣ�ṹ
		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = pipelineLayout; // ʹ�õĹ��߲���
		pipelineCI.renderPass = renderPass; // ʹ�õ���Ⱦͨ��

		// ������װ״̬��Ϣ�ṹ
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // �������б�����

		// ��դ��״̬��Ϣ�ṹ
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;			// ��������ģʽ
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;					// ���޳�����
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// ��ʱ��Ϊ����
		rasterizationStateCI.depthClampEnable = VK_FALSE;					// ��Ȳü�����
		rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;			// ��դ����������Ƭ��
		rasterizationStateCI.depthBiasEnable = VK_FALSE;					// ���ƫ�ƽ���
		rasterizationStateCI.lineWidth = 1.0f;								// �߿�Ϊ1.0

		// ��ɫ�ںϸ���״̬��Ϣ�ṹ
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = 0xf;							// RGBA������ɫд��
		blendAttachmentState.blendEnable = VK_FALSE;						// �ںϽ���
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentState;

		// �ӿ�״̬��Ϣ�ṹ
		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;

		// ��̬״̬��Ϣ�ṹ
		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);			// ��̬�ӿ�
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);			// ��̬�ü�
		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// ���ģ��״̬��Ϣ�ṹ
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_TRUE;						// ��Ȳ�������
		depthStencilStateCI.depthWriteEnable = VK_TRUE;						// ���д������
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;	// ��ȱȽϲ���
		depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;				// ��ȷ�Χ���Խ���
		depthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilStateCI.stencilTestEnable = VK_FALSE;					// ģ����Խ���
		depthStencilStateCI.front = depthStencilStateCI.back;

		// ���ز���״̬��Ϣ�ṹ
		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// ������Ϊ1
		multisampleStateCI.pSampleMask = nullptr;

		// �������������������
		VkVertexInputBindingDescription vertexInputBinding{};
		vertexInputBinding.binding = 0;										// �󶨺ţ������������������
		vertexInputBinding.stride = sizeof(Vertex);							// һ���������ݵ��ֽ���
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;			// ���ƶ������ݵĲ�����ʽ������ÿ�������������ǽ������е�

		// ��������������������������
		std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributes;
		vertexInputAttributes[0].binding = 0;								// ������İ󶨺Ŷ�Ӧ
		vertexInputAttributes[0].location = 0;								// ������ɫ�������Ե�λ��ָ��
		vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;		// ���ݸ�ʽ������������32λ������
		vertexInputAttributes[0].offset = offsetof(Vertex, position);		// �����ڽṹ���е�ƫ����

		vertexInputAttributes[1].binding = 0;								// ������İ󶨺Ŷ�Ӧ
		vertexInputAttributes[1].location = 1;								// ������ɫ�������Ե�λ��ָ��
		vertexInputAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;		// ���ݸ�ʽ������������32λ������
		vertexInputAttributes[1].offset = offsetof(Vertex, color);			// �����ڽṹ���е�ƫ����

		// ������������״̬��Ϣ�ṹ
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
		vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCI.vertexBindingDescriptionCount = 1;				// �����������������
		vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;// ���������������
		vertexInputStateCI.vertexAttributeDescriptionCount = 2;				// ��������������������
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data(); // ������������������

		// ������ɫ����Ƭ����ɫ���׶���Ϣ�ṹ
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		shaderStages[0] = loadShader("src/triangle/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("src/triangle/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size()); // ��ɫ���׶�����
		pipelineCI.pStages = shaderStages.data();							// ��ɫ���׶�����

		// ����ͼ�ι��ߵĸ���״̬��Ϣ
		pipelineCI.pVertexInputState = &vertexInputStateCI;					// ��������״̬��Ϣ
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;				// ������װ״̬��Ϣ
		pipelineCI.pRasterizationState = &rasterizationStateCI;				// ��դ��״̬��Ϣ
		pipelineCI.pColorBlendState = &colorBlendStateCI;					// ��ɫ���״̬��Ϣ
		pipelineCI.pMultisampleState = &multisampleStateCI;					// �����״̬��Ϣ
		pipelineCI.pViewportState = &viewportStateCI;						// �ӿ�״̬��Ϣ
		pipelineCI.pDepthStencilState = &depthStencilStateCI;				// ���ģ��״̬��Ϣ
		pipelineCI.pDynamicState = &dynamicStateCI;							// ��̬״̬��Ϣ

		// ����ͼ�ι���
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

		// ������ɫ��ģ��
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

		// �������ڶ������ݵ��ݴ滺����
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

		// �������ڶ������ݵ�Ŀ�껺����
		vertexBufferInfoCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &vertices.buffer));
		vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

		// ���������������ݵ��ݴ滺����
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

		// ���������������ݵ�Ŀ�껺����
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
		
		// �����������ݵ�Ŀ�껺�����������������ݵ�Ŀ�껺����
		VkBufferCopy copyRegion{};
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingVerticesBF.buffer, vertices.buffer, 1, &copyRegion);
		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingIndicesBF.buffer, indices.buffer, 1, &copyRegion);
		VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

		// �ύ����������в��ȴ����
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

		// ������Դ
		vkDestroyFence(device, fence, nullptr);
		vkFreeCommandBuffers(device, cmdPool, 1, &copyCmd);
		vkDestroyBuffer(device, stagingVerticesBF.buffer, nullptr);
		vkFreeMemory(device, stagingVerticesBF.memory, nullptr);
		vkDestroyBuffer(device, stagingIndicesBF.buffer, nullptr);
		vkFreeMemory(device, stagingIndicesBF.memory, nullptr);
	}

	void createUniformBuffers()
	{
		VkBufferCreateInfo bufferInfo{};		// ���建������Ϣ�ṹ��
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(ShaderData);
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VkMemoryAllocateInfo allocInfo{};		// �����ڴ������Ϣ�ṹ��
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = 0;
		allocInfo.memoryTypeIndex = 0;
		VkMemoryRequirements memReqs;

		// ѭ���������֡��Uniform������
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffers[i].buffer));	// ����Uniform������
			vkGetBufferMemoryRequirements(device, uniformBuffers[i].buffer, &memReqs);					// ��ȡ���������ڴ�����
			allocInfo.allocationSize = memReqs.size;													// �����ڴ������Ϣ
			allocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &(uniformBuffers[i].memory)));
			VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffers[i].buffer, uniformBuffers[i].memory, 0));// ���ڴ��뻺������
			VK_CHECK_RESULT(vkMapMemory(device, uniformBuffers[i].memory, 0, sizeof(ShaderData), 0, (void**)&uniformBuffers[i].mapped));// ӳ���ڴ浽CPU�ɷ��ʵ�ָ��
		}
	}

	void createDescriptorPool()
	{
		// �����������������������ͺ�������uniform���͵���������������
		VkDescriptorPoolSize descriptorTypeCounts[1];
		descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorTypeCounts[0].descriptorCount = MAX_CONCURRENT_FRAMES;

		// ��ʼ������������Ϣ�ṹ��
		VkDescriptorPoolCreateInfo descriptorPoolCI{};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.pNext = nullptr;
		descriptorPoolCI.poolSizeCount = 1;
		descriptorPoolCI.pPoolSizes = descriptorTypeCounts;
		descriptorPoolCI.maxSets = MAX_CONCURRENT_FRAMES;

		// ������������
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));
	}

	void createDescriptorSets()
	{	// ʹ��ѭ��Ϊÿһ֡��������������
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			// �������������Ϸ�����Ϣ�ṹ��
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;			// ���������Ϸ�����ʹ�õ���������
			allocInfo.descriptorSetCount = 1;					// Ҫ�������������������
			allocInfo.pSetLayouts = &descriptorSetLayout;		// ���������ϵĲ���
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffers[i].descriptorSet)); // ��������������

			// ����д�����������ϵĽṹ��
			VkWriteDescriptorSet writeDescriptorSet{};

			// ����������������Ϣ�ṹ��
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i].buffer;		// �����������Ļ������
			bufferInfo.range = sizeof(ShaderData);				// �����������Ļ������Ĵ�С

			// ���д�����������ϵĽṹ��
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = uniformBuffers[i].descriptorSet; // Ŀ������������
			writeDescriptorSet.dstBinding = 0;					// �����������еİ󶨵�
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // ����������ΪUniform����
			writeDescriptorSet.descriptorCount = 1;				// Ҫ���µ�����������
			writeDescriptorSet.pBufferInfo = &bufferInfo;		// ָ��������������Ϣ��ָ��
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr); // ��������������
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
		if (!renderingAllowed) return;					// �����������Ⱦ����ֱ�ӷ���
		
		vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX);		// �ȴ���ǰ֡��fence�źţ�ȷ��ǰһ֡����Ⱦ�����
		
		uint32_t imageIndex;							// ��ȡ��һ��ͼ�񣬼��ӽ������л�ȡ��д��ͼ������
		VkResult result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {		// ������ش������������������Ч�������Ǵ��ڴ�С�仯����������´����ڴ�С
			windowResize();								
			return;
		}
		else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)) {				// �����ȡͼ��ʧ�ܣ����׳��쳣
			throw "Could not acquire the next swap chain image!";
		}

		
		ShaderData shaderData{};						// ׼�����ݸ���ɫ����Uniform����
		shaderData.projectionMatrix = camera.matrices.perspective;
		shaderData.viewMatrix = camera.matrices.view;
		shaderData.modelMatrix = glm::mat4(1.0f);
		memcpy(uniformBuffers[currentBuffer].mapped, &shaderData, sizeof(ShaderData));	// ��Uniform���ݿ�������ǰ֡��Ӧ�Ļ��������

		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentFrame]));			// ���õ�ǰ֡��fence�ź�
		vkResetCommandBuffer(commandBuffers[currentBuffer], 0);							// ���õ�ǰ֡��Ӧ������壬׼����ʼ��¼����
		
		VkCommandBufferBeginInfo cmdBufInfo{};			// ���������Ŀ�ʼ��Ϣ
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		
		VkClearValue clearValues[2];					// �������ֵ���������ɫ�����/ģ�建��
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };
		VkRenderPassBeginInfo renderPassBeginInfo{};	// ������Ⱦͨ���Ŀ�ʼ��Ϣ
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
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffers[currentBuffer], &cmdBufInfo));// ��ʼ��¼�����
		vkCmdBeginRenderPass(commandBuffers[currentBuffer], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		VkViewport viewport{};							// �����������ӿ�
		viewport.height = (float)windowdata.height;
		viewport.width = (float)windowdata.width;
		viewport.minDepth = (float)0.0f;
		viewport.maxDepth = (float)1.0f;
		vkCmdSetViewport(commandBuffers[currentBuffer], 0, 1, &viewport);
		VkRect2D scissor{};								// ���������òü�����
		scissor.extent.width = windowdata.width;
		scissor.extent.height = windowdata.height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(commandBuffers[currentBuffer], 0, 1, &scissor);						// ������������ͼ�ι���
		vkCmdBindDescriptorSets(commandBuffers[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uniformBuffers[currentBuffer].descriptorSet, 0, nullptr);
		vkCmdBindPipeline(commandBuffers[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);// ��ͼ�ι���
		VkDeviceSize offsets[1]{ 0 };					// �󶨶��㻺�壬����������
		vkCmdBindVertexBuffers(commandBuffers[currentBuffer], 0, 1, &vertices.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffers[currentBuffer], indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffers[currentBuffer], indices.count, 1, 0, 0, 1);			// ִ��������������
		vkCmdEndRenderPass(commandBuffers[currentBuffer]);									// ������Ⱦͨ��
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffers[currentBuffer]));					// ���������ļ�¼

		// ָ���ȴ��׶ε�����Ϊ��ɫ��������׶�
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		
		VkSubmitInfo submitInfo{};						// �����ύ��Ϣ�ṹ��
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &waitStageMask;	// ָ���źŵȴ��׶ε����룬���ύ�ȴ��źŵĹ��߽׶�
		submitInfo.waitSemaphoreCount = 1;				// ���õȴ����ź�������������
		submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentFrame];
		submitInfo.signalSemaphoreCount = 1;			// �����źŵ��ź�������������
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentFrame];
		submitInfo.commandBufferCount = 1;				// ����Ҫִ�е���������鼰����
		submitInfo.pCommandBuffers = &commandBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentFrame]));	// �ύ����嵽���У�ͬʱ���ݵȴ��źš��źź�ͬ����fence

		// ����������Ϣ�ṹ��
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;				// ���õȴ����ź�������������
		presentInfo.pWaitSemaphores = &renderCompleteSemaphores[currentFrame];
		presentInfo.swapchainCount = 1;					// ���ý��������������顢�Լ�ͼ������
		presentInfo.pSwapchains = &swapChain.swapChain;
		presentInfo.pImageIndices = &imageIndex;
		result = vkQueuePresentKHR(queue, &presentInfo);// ʹ�ý���������ͼ��

		// �����ֽ�������������������Ч���߲����룬�������ڴ�С�仯
		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
			windowResize();
		}
		else if (result != VK_SUCCESS) {				// �������ͼ��ʧ�ܣ����׳��쳣
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
