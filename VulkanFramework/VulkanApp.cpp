#include "VulkanApp.h"
#include "VulkanSwapchain.h"
#include "VulkanHelpers.h"
#include "Buffer.h"
#include "Texture.h"
#include "VulkanDevice.h"
#include "Shader.h"
#include "Helper.h"
#include <array>
#include "Window.h"
#include "RenderPass.h"
#include "CommandPool.h"
#include "FrameBuffer.h"

VulkanApp::VulkanApp(vkw::VulkanDevice* pDevice):VulkanBaseApp(pDevice, "Raytracing God :)")
{
}


VulkanApp::~VulkanApp()
{
}

void VulkanApp::Render()
{
	GetSwapchain()->AcquireNextImage(GetPresentCompleteSemaphore());
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &GetPresentCompleteSemaphore();
	VkPipelineStageFlags waitDstMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.pWaitDstStageMask = &waitDstMask;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &GetRenderCompleteSemaphore();
	submitInfo.pCommandBuffers = &GetDrawCommandBuffers()[GetSwapchain()->GetActiveImageId()];
	submitInfo.commandBufferCount = 1;

	ErrorCheck(vkQueueSubmit(GetDevice()->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE));

	GetSwapchain()->PresentImage(GetRenderCompleteSemaphore());
	ErrorCheck(vkQueueWaitIdle(GetDevice()->GetQueue()));

	vkWaitForFences(GetDevice()->GetDevice(), 1, &m_ComputeFence, VK_TRUE, UINT64_MAX);
	vkResetFences(GetDevice()->GetDevice(), 1, &m_ComputeFence);

	VkSubmitInfo computeSubmitInfo{};
	computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	computeSubmitInfo.commandBufferCount = 1;
	computeSubmitInfo.pCommandBuffers = &m_ComputeCommandBuffer;

	ErrorCheck(vkQueueSubmit(m_ComputeQueue, 1, &computeSubmitInfo, m_ComputeFence));
}

bool VulkanApp::Update(float dTime)
{
	m_AccuTime += dTime;
	UpdateUniformBuffers();
	return VulkanBaseApp::Update(dTime);
}


void VulkanApp::Init(float width, float height)
{
	VulkanBaseApp::Init(width, height);
	CreateStorageBuffers();
	CreateUniformBuffers();
	UpdateUniformBuffers();
	CreateTextureTarget();
	CreateGraphicsPipeline();
	CreateDescriptorPool();
	CreateDescriptorSet();
	CreateComputePipeline();
	BuildDrawCommandBuffers();
	BuildComputeCommandBuffers();
}

void VulkanApp::Cleanup()
{
	ErrorCheck(vkQueueWaitIdle(GetDevice()->GetQueue()));
	ErrorCheck(vkQueueWaitIdle(m_ComputeQueue));
	VulkanBaseApp::Cleanup();
	DestroyComputePipeline();
	DestroyDescriptorPool();
	DestroyGraphicsPipeline();
	DestroyTextureTarget();
	DestroyUniformBuffers();
	DestroyStorageBuffers();
}

void VulkanApp::CreateStorageBuffers()
{
	// Spheres
	std::vector<Sphere> spheres;
	spheres.push_back(Sphere{ glm::vec3(1.75f, -0.5f, 0.0f), 1.0f, glm::vec3(0.0f, 1.0f, 0.0f), 32.0f, 0 });
	spheres.push_back(Sphere{ glm::vec3(0.0f, 1.0f, -0.5f), 1.0f, glm::vec3(0.65f, 0.77f, 0.97f), 32.0f, 1 });
	spheres.push_back(Sphere{ glm::vec3(-1.75f, -0.75f, -0.5f), 1.25f, glm::vec3(0.9f, 0.76f, 0.46f), 32.0f, 2 });
	VkDeviceSize storageBufferSize = spheres.size() * sizeof(Sphere);

	m_pSphereGeomBuffer = new vkw::Buffer(
		GetDevice(), GetCommandPool(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		size_t(spheres.size()*sizeof(Sphere)), (void*)spheres.data()
	);

	// Planes
	std::vector<Plane> planes;
	const float roomDim = 4.0f;
	planes.push_back(Plane{ glm::vec3(0.0f, 1.0f, 0.0f), roomDim, glm::vec3(1.0f), 32.0f, 3 });
	//planes.push_back(Plane{ glm::vec3(0.0f, -1.0f, 0.0f), roomDim, glm::vec3(1.0f), 32.0f, 4 });
	//planes.push_back(Plane{ glm::vec3(0.0f, 0.0f, 1.0f), roomDim, glm::vec3(1.0f), 32.0f, 5 });
	//planes.push_back(Plane{ glm::vec3(0.0f, 0.0f, -1.0f), roomDim, glm::vec3(0.0f), 32.0f, 6 });
	//planes.push_back(Plane{ glm::vec3(-1.0f, 0.0f, 0.0f), roomDim, glm::vec3(1.0f, 0.0f, 0.0f), 32.0f, 7 });
	//planes.push_back(Plane{ glm::vec3(1.0f, 0.0f, 0.0f), roomDim, glm::vec3(0.0f, 1.0f, 0.0f), 32.0f, 8 });
	storageBufferSize = planes.size() * sizeof(Plane);

	m_pPlaneGeomBuffer = new vkw::Buffer(
		GetDevice(), GetCommandPool(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		size_t(planes.size() * sizeof(Plane)), (void*)planes.data()
	);

	std::vector<Triangle> triangles;
	triangles.push_back(Triangle{ {-1.f, -1.f, 0.f}, 4, {0.f, 1.f, 0.f}, 32.f, {1.f, -1.f, 0.f}, 0, {0.f, 0.f, 1.f}, 0, {0.65f, 0.77f, 0.97f} });

	m_pTriangleGeomBuffer = new vkw::Buffer(
		GetDevice(), GetCommandPool(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		size_t(triangles.size()*sizeof(Triangle)), triangles.data()
	);
}

void VulkanApp::CreateUniformBuffers()
{
	m_pUniformBuffer = new vkw::Buffer(
		GetDevice(), GetCommandPool(),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		size_t(sizeof(UBOCompute)), nullptr
	);
}

void VulkanApp::UpdateUniformBuffers()
{
	m_UniformBufferData.lightPos.x = 0.0f + sin(glm::radians(m_AccuTime * 20.0f)) * cos(glm::radians(1 * 360.0f)) * 2.0f;
	m_UniformBufferData.lightPos.y = 0.0f + sin(glm::radians(m_AccuTime * 20.0f)) * 2.0f;
	m_UniformBufferData.lightPos.z = 0.0f + cos(glm::radians(m_AccuTime * 20.0f)) * 2.0f;
	m_UniformBufferData.lookat = { 0.0f, 0.5f, 0.0f };
	m_UniformBufferData.pos = { 0.0f, 0.0f, 4.0f };
	m_pUniformBuffer->Update(&m_UniformBufferData ,sizeof(UBOCompute), GetCommandPool());
}

void VulkanApp::CreateTextureTarget()
{
	m_pTexture = new vkw::Texture(GetDevice(), GetCommandPool(), VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_LAYOUT_GENERAL, nullptr, GetWindow()->GetSurfaceSize().width, GetWindow()->GetSurfaceSize().height);
}

void VulkanApp::CreateGraphicsPipeline()
{
	//Binding 0 = FS image sampler
	VkDescriptorSetLayoutBinding setLayoutBinding {};
	setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	setLayoutBinding.binding = 0;
	setLayoutBinding.descriptorCount = 1;

	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
	{
		setLayoutBinding
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
	descriptorSetLayoutCreateInfo.bindingCount = setLayoutBindings.size();

	
	ErrorCheck(vkCreateDescriptorSetLayout(GetDevice()->GetDevice(), &descriptorSetLayoutCreateInfo, nullptr, &m_GraphicsDescriptorSetLayout));

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_GraphicsDescriptorSetLayout;

	
	ErrorCheck(vkCreatePipelineLayout(GetDevice()->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_GraphicsPipelineLayout));

	std::vector<char> vertShaderCode = readFile("Shaders/vert.spv");
	std::vector<char> fragShaderCode = readFile("Shaders/frag.spv");

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode, GetDevice()->GetDevice());
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode, GetDevice()->GetDevice());

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

	//Empty since we provide vertex data from the shader
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = GetWindow()->GetSurfaceSize().width;
	viewport.height = GetWindow()->GetSurfaceSize().height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = GetWindow()->GetSurfaceSize();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizerState{};
	rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerState.depthClampEnable = VK_FALSE;
	rasterizerState.rasterizerDiscardEnable = VK_FALSE;
	rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerState.lineWidth = 1.0f;
	rasterizerState.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizerState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerState.depthBiasEnable = VK_FALSE;
	rasterizerState.depthBiasConstantFactor = 0.0f; // Optional
	rasterizerState.depthBiasClamp = 0.0f; // Optional
	rasterizerState.depthBiasSlopeFactor = 0.0f; // Optional


	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.sampleShadingEnable = VK_FALSE;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.minSampleShading = 1.0f; // Optional
	multisampleState.pSampleMask = nullptr; // Optional
	multisampleState.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampleState.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	std::array<VkPipelineColorBlendAttachmentState, 1> colorBlendAttachments{ colorBlendAttachment };

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlendState.attachmentCount = colorBlendAttachments.size();
	colorBlendState.pAttachments = colorBlendAttachments.data();
	colorBlendState.blendConstants[0] = 0.0f; // Optional
	colorBlendState.blendConstants[1] = 0.0f; // Optional
	colorBlendState.blendConstants[2] = 0.0f; // Optional
	colorBlendState.blendConstants[3] = 0.0f; // Optional


	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizerState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.layout = m_GraphicsPipelineLayout;
	pipelineCreateInfo.renderPass = GetRenderPass()->GetHandle();
	pipelineCreateInfo.subpass = 0;

	ErrorCheck(vkCreateGraphicsPipelines(GetDevice()->GetDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_GraphicsPipeline));

	vkDestroyShaderModule(GetDevice()->GetDevice(), fragShaderModule, nullptr);
	vkDestroyShaderModule(GetDevice()->GetDevice(), vertShaderModule, nullptr);
}

void VulkanApp::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 4> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(GetSwapchain()->GetImageCount());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(GetSwapchain()->GetImageCount());
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[2].descriptorCount = 1;
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[3].descriptorCount = static_cast<uint32_t>(GetSwapchain()->GetImageCount());

	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = poolSizes.size();
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = 2;

	ErrorCheck(vkCreateDescriptorPool(GetDevice()->GetDevice(), &descriptorPoolInfo, nullptr, &m_DescriptorPool));
}

void VulkanApp::CreateDescriptorSet()
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.pSetLayouts = &m_GraphicsDescriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	ErrorCheck(vkAllocateDescriptorSets(GetDevice()->GetDevice(), &allocInfo, &m_GraphicsDescriptorSet));

	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = m_GraphicsDescriptorSet;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet.dstBinding = 0;
	writeDescriptorSet.pImageInfo = &m_pTexture->GetDescriptor();
	writeDescriptorSet.descriptorCount = 1;
	

	vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &writeDescriptorSet, 0, NULL);
}

void VulkanApp::CreateComputePipeline()
{
	// Create a compute capable device queue
		// The VulkanDevice::createLogicalDevice functions finds a compute capable queue and prefers queue families that only support compute
		// Depending on the implementation this may result in different queue family indices for graphics and computes,
		// requiring proper synchronization (see the memory barriers in buildComputeCommandBuffer)
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.pNext = NULL;
	queueCreateInfo.queueFamilyIndex = GetDevice()->GetComputeFamilyQueueId();
	queueCreateInfo.queueCount = 1;
	vkGetDeviceQueue(GetDevice()->GetDevice(), GetDevice()->GetComputeFamilyQueueId(), 0, &m_ComputeQueue);

	std::array<VkDescriptorSetLayoutBinding, 5> setLayoutBindings{};
	setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	setLayoutBindings[0].binding = 0;
	setLayoutBindings[0].descriptorCount = 1;

	setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	setLayoutBindings[1].binding = 1;
	setLayoutBindings[1].descriptorCount = 1;

	setLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	setLayoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	setLayoutBindings[2].binding = 2;
	setLayoutBindings[2].descriptorCount = 1;

	setLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	setLayoutBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	setLayoutBindings[3].binding = 3;
	setLayoutBindings[3].descriptorCount = 1;

	setLayoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	setLayoutBindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	setLayoutBindings[4].binding = 4;
	setLayoutBindings[4].descriptorCount = 1;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
	descriptorSetLayoutCreateInfo.bindingCount = setLayoutBindings.size();


	ErrorCheck(vkCreateDescriptorSetLayout(GetDevice()->GetDevice(), &descriptorSetLayoutCreateInfo, nullptr, &m_ComputeDescriptorSetLayout));

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pSetLayouts = &m_ComputeDescriptorSetLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 1;

	
	ErrorCheck(vkCreatePipelineLayout(GetDevice()->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_ComputePipelineLayout));

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.pSetLayouts = &m_ComputeDescriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	
	ErrorCheck(vkAllocateDescriptorSets(GetDevice()->GetDevice(), &allocInfo, &m_ComputeDescriptorSet));

	std::array<VkWriteDescriptorSet, 5> computeWriteDescriptorSets{};
	computeWriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	computeWriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	computeWriteDescriptorSets[0].descriptorCount = 1;
	computeWriteDescriptorSets[0].dstSet = m_ComputeDescriptorSet;
	computeWriteDescriptorSets[0].dstBinding = 0;
	computeWriteDescriptorSets[0].pImageInfo = &m_pTexture->GetDescriptor();

	computeWriteDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	computeWriteDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	computeWriteDescriptorSets[1].descriptorCount = 1;
	computeWriteDescriptorSets[1].dstSet = m_ComputeDescriptorSet;
	computeWriteDescriptorSets[1].dstBinding = 1;
	computeWriteDescriptorSets[1].pBufferInfo = &m_pUniformBuffer->GetDescriptor();

	computeWriteDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	computeWriteDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	computeWriteDescriptorSets[2].descriptorCount = 1;
	computeWriteDescriptorSets[2].dstSet = m_ComputeDescriptorSet;
	computeWriteDescriptorSets[2].dstBinding = 2;
	computeWriteDescriptorSets[2].pBufferInfo = &m_pSphereGeomBuffer->GetDescriptor();

	computeWriteDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	computeWriteDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	computeWriteDescriptorSets[3].descriptorCount = 1;
	computeWriteDescriptorSets[3].dstBinding = 3;
	computeWriteDescriptorSets[3].dstSet = m_ComputeDescriptorSet;
	computeWriteDescriptorSets[3].pBufferInfo = &m_pPlaneGeomBuffer->GetDescriptor();

	computeWriteDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	computeWriteDescriptorSets[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	computeWriteDescriptorSets[4].descriptorCount = 1;
	computeWriteDescriptorSets[4].dstBinding = 4;
	computeWriteDescriptorSets[4].dstSet = m_ComputeDescriptorSet;
	computeWriteDescriptorSets[4].pBufferInfo = &m_pTriangleGeomBuffer->GetDescriptor();


	vkUpdateDescriptorSets(GetDevice()->GetDevice(), computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL);

	VkShaderModule computeShaderModule = CreateShaderModule(readFile("Shaders/comp.spv"), GetDevice()->GetDevice());

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = computeShaderModule;
	computeShaderStageInfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = m_ComputePipelineLayout;
	computePipelineCreateInfo.flags = 0;
	computePipelineCreateInfo.stage = computeShaderStageInfo;

	ErrorCheck(vkCreateComputePipelines(GetDevice()->GetDevice(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_ComputePipeline));

	vkDestroyShaderModule(GetDevice()->GetDevice(), computeShaderModule, nullptr);

	// Separate command pool as queue family for compute may be different than graphics
	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = GetDevice()->GetComputeFamilyQueueId();
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	
	ErrorCheck(vkCreateCommandPool(GetDevice()->GetDevice(), &cmdPoolInfo, nullptr, &m_ComputeCommandPool));

	// Create a command buffer for compute operations
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_ComputeCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;

	ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &commandBufferAllocateInfo, &m_ComputeCommandBuffer));

	// Fence for compute CB sync
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	
	ErrorCheck(vkCreateFence(GetDevice()->GetDevice(), &fenceCreateInfo, nullptr, &m_ComputeFence));

}

void VulkanApp::BuildComputeCommandBuffers()
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	ErrorCheck(vkBeginCommandBuffer(m_ComputeCommandBuffer, &commandBufferBeginInfo));

	vkCmdBindPipeline(m_ComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
	vkCmdBindDescriptorSets(m_ComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 0, 1, &m_ComputeDescriptorSet, 0, 0);

	vkCmdDispatch(m_ComputeCommandBuffer, m_pTexture->GetWidth() / 16, m_pTexture->GetHeight() / 16, 1);

	vkEndCommandBuffer(m_ComputeCommandBuffer);
}

void VulkanApp::DestroyStorageBuffers()
{
	delete m_pSphereGeomBuffer;
	delete m_pPlaneGeomBuffer;
	delete m_pTriangleGeomBuffer;
}

void VulkanApp::DestroyUniformBuffers()
{
	delete m_pUniformBuffer;
}

void VulkanApp::DestroyTextureTarget()
{
	delete m_pTexture;
}

void VulkanApp::DestroyGraphicsPipeline()
{
	vkDestroyPipeline(GetDevice()->GetDevice(), m_GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(GetDevice()->GetDevice(), m_GraphicsPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(GetDevice()->GetDevice(), m_GraphicsDescriptorSetLayout, nullptr);
}

void VulkanApp::DestroyDescriptorPool()
{
	vkDestroyDescriptorPool(GetDevice()->GetDevice(), m_DescriptorPool, nullptr);
}

void VulkanApp::DestroyComputePipeline()
{
	vkDestroyPipeline(GetDevice()->GetDevice(), m_ComputePipeline, nullptr);
	vkDestroyPipelineLayout(GetDevice()->GetDevice(), m_ComputePipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(GetDevice()->GetDevice(), m_ComputeDescriptorSetLayout, nullptr);
	vkDestroyFence(GetDevice()->GetDevice(), m_ComputeFence, nullptr);
	vkDestroyCommandPool(GetDevice()->GetDevice(), m_ComputeCommandPool, nullptr);
}





void VulkanApp::BuildDrawCommandBuffers()
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkClearValue clearValues[2];
	clearValues[0].color = { 0.5f, 0.5f, 0.5f, 1.f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = GetRenderPass()->GetHandle();
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent = GetWindow()->GetSurfaceSize();
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	for (int32_t i = 0; i < GetDrawCommandBuffers().size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();

		ErrorCheck(vkBeginCommandBuffer(GetDrawCommandBuffers()[i], &commandBufferBeginInfo));

		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.image = m_pTexture->GetImage();
		imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(
			GetDrawCommandBuffers()[i],
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier
		);

		vkCmdBeginRenderPass(GetDrawCommandBuffers()[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = GetWindow()->GetSurfaceSize().width;
		viewport.height = GetWindow()->GetSurfaceSize().height;
		vkCmdSetViewport(GetDrawCommandBuffers()[i], 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = GetWindow()->GetSurfaceSize();
		scissor.offset = { 0,0 };

		vkCmdSetScissor(GetDrawCommandBuffers()[i], 0, 1, &scissor);

		// Display ray traced image generated by compute shader as a full screen quad
		// Quad vertices are generated in the vertex shader
		vkCmdBindDescriptorSets(GetDrawCommandBuffers()[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelineLayout, 0, 1, &m_GraphicsDescriptorSet, 0, NULL);
		vkCmdBindPipeline(GetDrawCommandBuffers()[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
		vkCmdDraw(GetDrawCommandBuffers()[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(GetDrawCommandBuffers()[i]);

		ErrorCheck(vkEndCommandBuffer(GetDrawCommandBuffers()[i]));
	}
}
