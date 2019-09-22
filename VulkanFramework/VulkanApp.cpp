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
#include <sstream>
#include <algorithm>
#include <gli/gli.hpp>

VulkanApp::VulkanApp(vkw::VulkanDevice* pDevice):VulkanBaseApp(pDevice, "Raytracing")
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
	//UpdateSpheres();
	if(GetWindow()->IsKeyButtonDown('W'))
	{
		m_UniformBufferData.pos += dTime * 5 * m_UniformBufferData.forward;
	}
	if (GetWindow()->IsKeyButtonDown('S'))
	{
		m_UniformBufferData.pos -= dTime * 5 * m_UniformBufferData.forward;
	}
	if(GetWindow()->IsKeyButtonDown('A'))
	{
		m_UniformBufferData.pos -= dTime * 5 * m_UniformBufferData.right;
	}
	if (GetWindow()->IsKeyButtonDown('D'))
	{
		m_UniformBufferData.pos += dTime * 5 * m_UniformBufferData.right;
	}
	if(GetWindow()->IsKeyButtonDown(VK_SPACE))
	{
		m_UniformBufferData.pos.y += dTime * 5;
	}
	if(GetWindow()->IsKeyButtonDown(VK_SHIFT))
	{
		m_UniformBufferData.pos.y -= dTime * 5;
	}
	


	glm::vec2 mouseMovement{ GetWindow()->GetMousePos() - m_PrevMousePosition };
	m_PrevMousePosition = GetWindow()->GetMousePos();

	if(GetWindow()->IsKeyButtonDown(VK_LBUTTON))
	{
		m_CameraRotation.x -= mouseMovement.x*dTime;
		m_CameraRotation.y -= mouseMovement.y*dTime;

		if (m_CameraRotation.y > 0.5f)
			m_CameraRotation.y = 0.5f;
		if (m_CameraRotation.y < -0.5f)
			m_CameraRotation.y = -0.5f;

		glm::mat4  rotationMatrix(1);
		m_UniformBufferData.right = glm::normalize(m_UniformBufferData.right);
		rotationMatrix = glm::rotate(rotationMatrix, m_CameraRotation.y, glm::vec3(m_UniformBufferData.right));
		rotationMatrix *= glm::rotate(rotationMatrix, m_CameraRotation.x, glm::vec3(0.0f, 1.f, 0.f));
		m_UniformBufferData.forward = rotationMatrix * glm::vec4{0, 0, -1, 0};
		m_UniformBufferData.forward = glm::normalize(m_UniformBufferData.forward);
	}

	UpdateUniformBuffers();
	return VulkanBaseApp::Update(dTime);
}


void VulkanApp::Init(float width, float height)
{
	VulkanBaseApp::Init(width, height);
	CreateStorageBuffers();
	CreateUniformBuffers();
	UpdateUniformBuffers();
	CreateSampleTextures();
	CreateCubeMap();
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
	DestroyCubeMap();
	DestroyTextureSamples();
	DestroyUniformBuffers();
	DestroyStorageBuffers();
}

void VulkanApp::CreateStorageBuffers()
{
	uint32_t currentId{0};

	// Spheres
	float rows{ 10 };
	float cols{ 10 };
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			m_Spheres.push_back(Sphere{ glm::vec3(-1.75f + (2.5f/cols)*c, 0.0f, 0.0f - ((2.5f/rows)*r)), (1.f/((rows > cols)? rows : cols)), glm::vec3(0.0f, 1.0f, 0.0f), ++currentId });
		}
	}
	

	VkDeviceSize storageBufferSize = m_Spheres.size() * sizeof(Sphere);

	m_pSphereGeomBuffer = new vkw::Buffer(
		GetDevice(), GetCommandPool(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		size_t(m_Spheres.size()*sizeof(Sphere)), (void*)m_Spheres.data()
	);

	// Planes
	std::vector<Plane> planes;
	const float roomDim = (1.f / ((rows > cols) ? rows : cols));
	planes.push_back(Plane{ glm::vec3(0.0f, 1.0f, 0.0f), roomDim, glm::vec3(1.0f, 0.f, 0.f), ++currentId });
	storageBufferSize = planes.size() * sizeof(Plane);

	m_pPlaneGeomBuffer = new vkw::Buffer(
		GetDevice(), GetCommandPool(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		size_t(planes.size() * sizeof(Plane)), (void*)planes.data()
	);

	std::vector<Triangle> triangles;
	triangles = LoadModel("Models/Cube.obj", currentId);

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
	++m_UniformBufferData.currentLayer;
	m_UniformBufferData.currentLayer %= m_SampleCount;

	m_UniformBufferData.lightDir = glm::vec3(-0.5f, -1.f, 0.5f);
	m_UniformBufferData.lightDir = glm::normalize(m_UniformBufferData.lightDir);

	m_UniformBufferData.right = glm::vec4(glm::cross(glm::vec3(m_UniformBufferData.forward), glm::vec3(0.f, 1.f, 0.f)), 0.f);
	m_UniformBufferData.up = glm::vec4(glm::cross(glm::vec3(m_UniformBufferData.right), glm::vec3(m_UniformBufferData.forward)), 0.f);

	m_UniformBufferData.rayOffset.x = (2*rand() % 1000) / 1000.f - 1.f;
	m_UniformBufferData.rayOffset.y = (2*rand() % 1000) / 1000.f - 1.f;
	

	m_UniformBufferData.aspectRatio =  float(GetWindow()->GetSurfaceSize().width) / float(GetWindow()->GetSurfaceSize().height);
	m_pUniformBuffer->Update(&m_UniformBufferData , sizeof(UBOCompute), GetCommandPool());
}


void VulkanApp::CreateSampleTextures()
{
	m_pSampleTextures = new vkw::Texture(GetDevice(), GetCommandPool(), VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_LAYOUT_GENERAL, nullptr, GetWindow()->GetSurfaceSize().width, GetWindow()->GetSurfaceSize().height, m_SampleCount);
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

	std::vector<char> vertShaderCode = readFile("Shaders/texture.vert.spv");
	std::vector<char> fragShaderCode = readFile("Shaders/texture.frag.spv");

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
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 3;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[2].descriptorCount = 1;
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[3].descriptorCount = 1;

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
	
	VkWriteDescriptorSet texArrayDescriptorSet{};
	texArrayDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	texArrayDescriptorSet.dstSet = m_GraphicsDescriptorSet;
	texArrayDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texArrayDescriptorSet.dstBinding = 0;
	texArrayDescriptorSet.pImageInfo = &m_pSampleTextures->GetDescriptor();
	texArrayDescriptorSet.descriptorCount = 1;

	std::vector<VkWriteDescriptorSet> writeDescriptors
	(
		{ texArrayDescriptorSet }
	);

	vkUpdateDescriptorSets(GetDevice()->GetDevice(), writeDescriptors.size(), writeDescriptors.data(), 0, NULL);
}

void VulkanApp::CreateComputePipeline()
{
	//Create compute queue
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.pNext = NULL;
	queueCreateInfo.queueFamilyIndex = GetDevice()->GetComputeFamilyQueueId();
	queueCreateInfo.queueCount = 1;
	vkGetDeviceQueue(GetDevice()->GetDevice(), GetDevice()->GetComputeFamilyQueueId(), 0, &m_ComputeQueue);

	std::array<VkDescriptorSetLayoutBinding, 6> setLayoutBindings{};
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

	setLayoutBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setLayoutBindings[5].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	setLayoutBindings[5].binding = 5;
	setLayoutBindings[5].descriptorCount = 1;



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

	std::array<VkWriteDescriptorSet, 6> computeWriteDescriptorSets{};
	computeWriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	computeWriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	computeWriteDescriptorSets[0].descriptorCount = 1;
	computeWriteDescriptorSets[0].dstSet = m_ComputeDescriptorSet;
	computeWriteDescriptorSets[0].dstBinding = 0;
	computeWriteDescriptorSets[0].pImageInfo = &m_pSampleTextures->GetDescriptor();

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

	computeWriteDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	computeWriteDescriptorSets[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	computeWriteDescriptorSets[5].descriptorCount = 1;
	computeWriteDescriptorSets[5].dstBinding = 5;
	computeWriteDescriptorSets[5].dstSet = m_ComputeDescriptorSet;
	computeWriteDescriptorSets[5].pImageInfo = &m_CubeMap.descriptor;


	vkUpdateDescriptorSets(GetDevice()->GetDevice(), computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL);

	VkShaderModule computeShaderModule = CreateShaderModule(readFile("Shaders/raytracing.comp.spv"), GetDevice()->GetDevice());

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

	vkCmdDispatch(m_ComputeCommandBuffer, m_pSampleTextures->GetWidth() / 16, m_pSampleTextures->GetHeight() / 16, 1);

	vkEndCommandBuffer(m_ComputeCommandBuffer);
}

void VulkanApp::UpdateSpheres()
{
	for (size_t i = 0; i < m_Spheres.size(); i++)
	{
		m_Spheres[i].pos.y = sin((m_AccuTime/5.f)+i);
	}

	m_Spheres.size() * sizeof(Sphere), (void*)m_Spheres.data();
	m_pSphereGeomBuffer->Update((void*)m_Spheres.data(), m_Spheres.size() * sizeof(Sphere), GetCommandPool());
}

void VulkanApp::CreateCubeMap()
{
	//Pick image with supported format
	std::string filename;
	VkFormat format;
	if (GetDevice()->GetDeviceFeatures().textureCompressionBC) {
		filename = "cubemap_yokohama_bc3_unorm.ktx";
		format = VK_FORMAT_BC2_UNORM_BLOCK;
	}
	else if (GetDevice()->GetDeviceFeatures().textureCompressionASTC_LDR) {
		filename = "cubemap_yokohama_astc_8x8_unorm.ktx";
		format = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
	}
	else if (GetDevice()->GetDeviceFeatures().textureCompressionETC2) {
		filename = "cubemap_yokohama_etc2_unorm.ktx";
		format = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
	}
	else {
		assert("Device does not support required format!" && 0);
		std::exit(-1);
	}

	//TODO: move this into its own class
	gli::texture_cube texCube(gli::load("Textures/" + filename));

	assert(!texCube.empty());

	m_CubeMap.width = texCube.extent().x;
	m_CubeMap.height = texCube.extent().y;
	m_CubeMap.mipLevels = texCube.levels();


	CreateImage(GetDevice()->GetDevice(), GetDevice()->GetPhysicalDeviceMemoryProperties(),
		m_CubeMap.width, m_CubeMap.height,
		format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_CubeMap.image, m_CubeMap.memory, 6, m_CubeMap.mipLevels, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory; 


	CreateBuffer(GetDevice()->GetDevice(), GetDevice()->GetPhysicalDeviceMemoryProperties(), texCube.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	uint8_t *data;
	ErrorCheck(vkMapMemory(GetDevice()->GetDevice(), stagingBufferMemory, 0, texCube.size(), 0, (void **)&data));
	memcpy(data, texCube.data(), texCube.size());
	vkUnmapMemory(GetDevice()->GetDevice(), stagingBufferMemory);

	
	std::vector<VkBufferImageCopy> bufferCopyRegions;
	uint32_t offset = 0;

	for (uint32_t face = 0; face < 6; face++)
	{
		for (uint32_t level = 0; level < m_CubeMap.mipLevels; level++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = texCube[face][level].extent().x;
			bufferCopyRegion.imageExtent.height = texCube[face][level].extent().y;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;

			bufferCopyRegions.push_back(bufferCopyRegion);

			offset += texCube[face][level].size();
		}
	}


	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = m_CubeMap.mipLevels;
	subresourceRange.layerCount = 6;

	TransitionImageLayout(GetDevice()->GetDevice(), GetDevice()->GetQueue(), GetCommandPool()->GetHandle(), m_CubeMap.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, m_CubeMap.mipLevels);

	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(GetDevice()->GetDevice(), GetCommandPool()->GetHandle());

	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, m_CubeMap.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bufferCopyRegions.size(), bufferCopyRegions.data());

	EndSingleTimeCommands(GetDevice()->GetDevice(), GetDevice()->GetQueue(), GetCommandPool()->GetHandle(), commandBuffer);

	m_CubeMap.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	TransitionImageLayout(GetDevice()->GetDevice(), GetDevice()->GetQueue(), GetCommandPool()->GetHandle(), m_CubeMap.image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_CubeMap.imageLayout, 6, m_CubeMap.mipLevels);

	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	sampler.maxLod = m_CubeMap.mipLevels;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler.maxAnisotropy = 1.0f;
	
	ErrorCheck(vkCreateSampler(GetDevice()->GetDevice(), &sampler, nullptr, &m_CubeMap.sampler));

	VkImageViewCreateInfo view{};
	view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	view.format = format;
	view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	view.subresourceRange.layerCount = 6;
	view.subresourceRange.levelCount = m_CubeMap.mipLevels;
	view.image = m_CubeMap.image;
	ErrorCheck(vkCreateImageView(GetDevice()->GetDevice(), &view, nullptr, &m_CubeMap.imageView));

	vkFreeMemory(GetDevice()->GetDevice(), stagingBufferMemory, nullptr);
	vkDestroyBuffer(GetDevice()->GetDevice(), stagingBuffer, nullptr);
	
	m_CubeMap.descriptor.sampler = m_CubeMap.sampler;
	m_CubeMap.descriptor.imageLayout = m_CubeMap.imageLayout;
	m_CubeMap.descriptor.imageView = m_CubeMap.imageView;
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

void VulkanApp::DestroyCubeMap()
{
	vkDestroySampler(GetDevice()->GetDevice(), m_CubeMap.sampler, nullptr);
	vkDestroyImageView(GetDevice()->GetDevice(), m_CubeMap.imageView, nullptr);
	vkDestroyImage(GetDevice()->GetDevice(), m_CubeMap.image, nullptr);
	vkFreeMemory(GetDevice()->GetDevice(), m_CubeMap.memory, nullptr);
}

void VulkanApp::DestroyTextureSamples()
{
	delete m_pSampleTextures;
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

std::vector<VulkanApp::Triangle> VulkanApp::LoadModel(std::string filePath, uint32_t& currentId)
{
	std::vector<glm::vec3> vertices;
	std::vector<Triangle> tris;
	std::ifstream input{ filePath };
	if (input)
	{
		std::string line;
		while (std::getline(input, line, '\n'))
		{
			if (line[0] == 'v')
			{
				std::string junk;
				float x{};
				float y{};
				float z{};
				std::istringstream sLine{ line };
				sLine >> junk >> x >> y >> z;
				vertices.push_back({ x, y, z });
			}
			else
			{
				if (line[0] == 'f')
				{
					size_t idx1;
					size_t idx2;
					size_t idx3;
					std::istringstream sLine{ line };
					std::string junk;
					sLine >> junk >> idx1 >> idx2 >> idx3;
					glm::vec3 v1 = vertices[idx1 - 1];
					glm::vec3 v2 = vertices[idx2 - 1];
					glm::vec3 v3 = vertices[idx3 - 1];
					glm::vec3 n = glm::cross(v2 - v1, v3 - v1);
					tris.push_back({ v1, ++currentId, v2, 32.f, v3, 0, n, 0, { 0.65f, 0.77f, 0.97f } });
				}
			}

		}
		return tris;
	}else
	{
		assert("File not found" && 0);
		std::exit(-1);
	}
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
		imageMemoryBarrier.image = m_pSampleTextures->GetImage();
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
