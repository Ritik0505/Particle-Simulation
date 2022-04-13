#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include "ParticleSystem.h"
#include <iostream>
#include<string.h>
#include "glm/gtx/string_cast.hpp"

ParticleSystem::ParticleSystem()
{
// 	m_StorageTexelBuffer = VK_NULL_HANDLE;
// 	m_UniformBuffer = VK_NULL_HANDLE;
// 	m_StorageTexelBufferView = VK_NULL_HANDLE;
// 	m_StorageTexelBufferMemoryObject = VK_NULL_HANDLE;
// 	m_UniformBufferMemoryObject = VK_NULL_HANDLE;

// 	m_RenderPass = VK_NULL_HANDLE;
// 	m_descriptorSetLayout = VK_NULL_HANDLE;

// 	graphics.pipeline = VK_NULL_HANDLE;
// 	graphics.pipelineLayout = VK_NULL_HANDLE;
// 	graphics.descriptorSet = VK_NULL_HANDLE;
// 	graphics.descriptorSetLayout = VK_NULL_HANDLE;
// 	graphics.semaphore = VK_NULL_HANDLE;
// 
// 	compute.commandBuffer = VK_NULL_HANDLE;
// 	compute.commandpool = VK_NULL_HANDLE;
// 	compute.pipeline = VK_NULL_HANDLE;
// 	compute.pipelineLayout = VK_NULL_HANDLE;
// 	compute.semaphore = VK_NULL_HANDLE;

	m_MVP = glm::mat4(1.0f);
	m_ParticlePool.resize(4*1024);

	if (!prepareVulkan()) {
		throw std::runtime_error("FAILED TO PREPARE VULKAN");
	}
	InitParticlesData();
	InitStorageTexelBuffer();
	UpdateUniformBuffer();
	InitUniformBuffer();

	PrepareDescriptorSets();
	SetupRenderPass();
	CreateFrameBuffers();
	PrepareGraphicsPipeline();
	PrepareComputePipeline();
	PrepareGraphicsCommandBuffer();
	PrepareComputeCommandBuffer();
}

void ParticleSystem::InitParticlesData()
{
	auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
	std::default_random_engine e((unsigned int)seed);
	std::uniform_real_distribution<float> rndDist(-10.0f, 10.0f);
	std::uniform_real_distribution<float> rndColor(0.0f, 1.0f);

	for (auto& Particle : m_ParticlePool) {
		Particle.position = glm::vec4(rndDist(e), rndDist(e), 0, 1.0);
 		Particle.color = glm::vec4(rndColor(e), rndColor(e), rndColor(e), rndColor(e));
// 		Particle.velocity = glm::vec2(0);
	}
// 	for (auto& Particle : m_ParticlePool) {
// 		std::cout << "Position: " << glm::to_string(Particle.position) << "Color: " << glm::to_string(Particle.color) << std::endl;
// 	}
}

void ParticleSystem::InitStorageTexelBuffer()
{
	//-------------STORAGE TEXEL BUFFER---------------//
	//Setting Buffer Parameters for Buffer Creation
	Buffer buffer;
	BufferParameters bufferParameters{};
	bufferParameters.bufferHandle = &m_StorageTexelBuffer;
	bufferParameters.formatFeaturesBits = VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT & VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
	bufferParameters.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	bufferParameters.usageBits = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferParameters.size = sizeof(m_ParticlePool[0]) * m_ParticlePool.size();

	VkMemoryPropertyFlags memoryPropertiesBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	buffer.CreateStorageTexelBuffer(m_PhysicalDevice, m_Device, bufferParameters, memoryPropertiesBits, m_StorageTexelBufferMemoryObject);

	
	//buffer.AllocateAndBindMemoryObject(m_PhysicalDevice, m_Device, memoryPropertiesBits, m_StorageTexelBuffer, m_StorageTexelBufferMemoryObject);
	buffer.CreateBufferView(m_Device, m_StorageTexelBuffer, bufferParameters.size, m_StorageTexelBufferView);

	//-------------STAGING BUFFER---------------//
	VkBuffer stagingBuffer;
	bufferParameters.bufferHandle = &stagingBuffer;
	bufferParameters.formatFeaturesBits = VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
	bufferParameters.usageBits = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferParameters.size = sizeof(m_ParticlePool[0]) * m_ParticlePool.size();

	VkDeviceMemory stagingBufferMemoryObject;
	memoryPropertiesBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	buffer.CreateBuffer(m_PhysicalDevice, m_Device, bufferParameters, memoryPropertiesBits, stagingBufferMemoryObject);
	//buffer.AllocateAndBindMemoryObject(m_PhysicalDevice, m_Device, memoryPropertiesBits, stagingBuffer, stagingBufferMemoryObject);

	//****** Memory Mapping ******//
	void* data;
	vkMapMemory(m_Device, stagingBufferMemoryObject, 0, bufferParameters.size, 0, &data);
	memcpy(data, m_ParticlePool.data(), (size_t)bufferParameters.size);
	vkUnmapMemory(m_Device, stagingBufferMemoryObject);

	//****** Copying staging Buffer data(Host Visible) to Storage Texel Buffer (Device Local) ******//
	CopyBuffer(stagingBuffer, m_StorageTexelBuffer, bufferParameters.size);
	vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
	vkFreeMemory(m_Device, stagingBufferMemoryObject, nullptr);
}

void ParticleSystem::InitUniformBuffer()
{
	Buffer buffer;
	BufferParameters bufferParameters{};
	//-------------UNIFORM BUFFER---------------//
	//Setting Buffer Parameters for Buffer Creation
	bufferParameters.bufferHandle = &m_UniformBuffer;
	bufferParameters.usageBits = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferParameters.size = sizeof(m_MVP);

	VkMemoryPropertyFlags memoryPropertiesBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	buffer.CreateBuffer(m_PhysicalDevice, m_Device, bufferParameters, memoryPropertiesBits, m_UniformBufferMemoryObject);

	//buffer.AllocateAndBindMemoryObject(m_PhysicalDevice, m_Device, memoryPropertiesBits, m_UniformBuffer, m_UniformBufferMemoryObject);

	//****** Memory Mapping ******//
	void* data;
	vkMapMemory(m_Device, m_UniformBufferMemoryObject, 0, bufferParameters.size, 0, &data);
	memcpy(data, &m_MVP, (size_t)bufferParameters.size);
	vkUnmapMemory(m_Device, m_UniformBufferMemoryObject);
}

void ParticleSystem::UpdateUniformBuffer()
{
	m_MVP = glm::mat4(1.0f);

	// Vulkan clip space has inverted Y and half Z.
	m_MVP *= glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, -1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.5f, 0.0f,
					0.0f, 0.0f, 0.5f, 1.0f);
	
	//Projection Matrix
	m_MVP *= glm::perspective(glm::radians(50.0f), static_cast<float>(1280/720), 1.0f, 100.0f);
	
	//View Matrix
	m_MVP *= glm::lookAt(
		glm::vec3(0, 0, 25), // Camera is at (-5,3,-10), in World Space
		glm::vec3(0, 0, 0),    // and looks at the origin
		glm::vec3(0, 1, 0)    // Head is up (set to 0,-1,0 to look upside-down)
	);

	//Model Matrix
	m_MVP *= glm::mat4(1.0f);
}

void ParticleSystem::CopyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, uint32_t& bufferSize)
{
	CreateCommandPool(m_Device, m_PresentationQueueFamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, m_PresentQueueCommandPool);
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = m_PresentQueueCommandPool;
	allocateInfo.commandBufferCount = 1;

	VkCommandBuffer copyCommandBuffer;
	vkAllocateCommandBuffers(m_Device, &allocateInfo, &copyCommandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pNext = nullptr;
	beginInfo.pInheritanceInfo = nullptr;

	//******* Recording Command Buffer for Copy *******
	vkBeginCommandBuffer(copyCommandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = bufferSize; 

	vkCmdCopyBuffer(copyCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	//vkEndCommandBuffer(copyCommandBuffer);

	/******* Submitting Command Buffer *******/
// 	VkSubmitInfo submitInfo = {};
// 	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
// 	submitInfo.commandBufferCount = 1;
// 	submitInfo.pCommandBuffers = &copyCommandBuffer;
// 
// 	vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	flushCommandBuffer(m_Device, copyCommandBuffer, m_GraphicsQueue, m_PresentQueueCommandPool, true);
	vkQueueWaitIdle(m_GraphicsQueue);
	
//	vkFreeCommandBuffers(m_Device, m_PresentQueueCommandPool, 1, &copyCommandBuffer);

}

void ParticleSystem::PrepareDescriptorSets()
{
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	// Binding = 0 : Storage Texel Buffer 
	descriptorSetLayoutBindings.push_back({
		0, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
		});
	// Binding = 1 : Uniform Buffer 
	descriptorSetLayoutBindings.push_back({
		1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, nullptr
		});

	VkDescriptorSetLayoutCreateInfo descriptorLayout;
	descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext = nullptr;
	descriptorLayout.flags = 0;
	descriptorLayout.bindingCount = descriptorSetLayoutBindings.size();
	descriptorLayout.pBindings = descriptorSetLayoutBindings.data();

	if ((vkCreateDescriptorSetLayout(m_Device, &descriptorLayout, nullptr, &m_descriptorSetLayout)) != VK_SUCCESS) {
		throw std::runtime_error("COULD NOT CREATE DESCRIPTOR SET LAYOUT");
	}

	std::vector<VkDescriptorPoolSize> descriptorPoolSize;
	descriptorPoolSize.push_back(
		{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1}
	);
	descriptorPoolSize.push_back(
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
	);

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = nullptr;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = 2;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSize.size());
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize.data();

	if ((vkCreateDescriptorPool(m_Device, &descriptorPoolCreateInfo, nullptr, &m_DescriptorPool)) != VK_SUCCESS) {
		throw std::runtime_error("COULD NOT CREATE DESCRIPTOR POOL");
	}

	//Allocating descriptor sets from Descriptor Pool
	VkDescriptorSetAllocateInfo allocInfo;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_descriptorSetLayout;

	m_descriptorSet.resize(1);
	if ((vkAllocateDescriptorSets(m_Device, &allocInfo, m_descriptorSet.data())) != VK_SUCCESS) {
		throw std::runtime_error("COULD NOT ALLOCATE DESCRIPTOT SETS FROM DESCRITOR POOL");
	}

	//Updating descriptor sets with buffers
	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = m_UniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	std::vector<VkWriteDescriptorSet> writeDescriptorSet;
	writeDescriptorSet.resize(2);

	writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
	writeDescriptorSet[0].descriptorCount = 1;
	writeDescriptorSet[0].dstSet = m_descriptorSet.at(0);
	writeDescriptorSet[0].dstBinding = 0;
	writeDescriptorSet[0].pTexelBufferView = &m_StorageTexelBufferView;

	writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet[1].descriptorCount = 1;
	writeDescriptorSet[1].dstSet = m_descriptorSet.at(0);
	writeDescriptorSet[1].dstBinding = 1;
	writeDescriptorSet[1].pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(m_Device, writeDescriptorSet.size(), writeDescriptorSet.data(), 0, nullptr);
}

void ParticleSystem::PrepareGraphicsPipeline()
{
	auto vertShaderModule = loadShader("Shaders/shader.vert.spv", m_Device);
	auto fragShaderModule = loadShader("Shaders/shader.frag.spv", m_Device);
	auto geomShaderModule = loadShader("Shaders/shader.geom.spv", m_Device);

// 	VkShaderModule vertShaderModule, fragShaderModule;
// 	CreateShaderModule(m_Device, vertexShaderCode, vertShaderModule);
// 	CreateShaderModule(m_Device, fragmentShaderCode, fragShaderModule);

	std::array<VkPipelineShaderStageCreateInfo,3> shaderStages;
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].pNext = nullptr;
	shaderStages[0].flags = 0;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].pName = "main";
	shaderStages[0].module = vertShaderModule;
	shaderStages[0].pSpecializationInfo = nullptr;

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].pNext = nullptr;
	shaderStages[1].flags = 0;
	shaderStages[1].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	shaderStages[1].pName = "main";
	shaderStages[1].module = geomShaderModule;
	shaderStages[1].pSpecializationInfo = nullptr;

	shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[2].pNext = nullptr;
	shaderStages[2].flags = 0;
	shaderStages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[2].pName = "main";
	shaderStages[2].module = fragShaderModule;
	shaderStages[2].pSpecializationInfo = nullptr;

	//VERTEX INPUT BINDING DESCRIPTION
	vertices.bindingDescriptions.resize(1);
	vertices.bindingDescriptions[0].binding = 0;
	vertices.bindingDescriptions[0].stride = sizeof(Particle);
	vertices.bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	//VERTEX ATTRIBUTE DESCRIPTION from the given binding
	//(index at which vertex buffer is bind i.e from which binding to retrieve these attributes)
	vertices.attributeDescriptions.resize(2);

	// Position : Location = 0
	vertices.attributeDescriptions[0].binding = 0;
	vertices.attributeDescriptions[0].location = 0;
	vertices.attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertices.attributeDescriptions[0].offset = 0;

	// Color : Location = 1
	vertices.attributeDescriptions[1].binding = 0;
	vertices.attributeDescriptions[1].location = 1;
	vertices.attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertices.attributeDescriptions[1].offset = 4 * sizeof(float);

	// Velocity : Location = 2
// 	vertices.attributeDescriptions[2].binding = 0;
// 	vertices.attributeDescriptions[2].location = 2;
// 	vertices.attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
// 	vertices.attributeDescriptions[2].offset = 8 * sizeof(float);

	// VERTEX INPUT STATE assigned to vertex buffer
	vertices.inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertices.inputState.pNext = nullptr;
	vertices.inputState.flags = 0;
	vertices.inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertices.bindingDescriptions.size());
	vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
	vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices.attributeDescriptions.size());
	vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.pNext = nullptr;
	inputAssemblyStateCreateInfo.flags = 0;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	std::vector<VkViewport> viewPort;
	viewPort.resize(1);
	viewPort[0].x = 0.0f;
	viewPort[0].y = 0.0f;
	//kept Same as SwapChain ImageExtent
	viewPort[0].width = (float)m_SwapChainExent.width;
	viewPort[0].height = (float)m_SwapChainExent.height;
	viewPort[0].minDepth = 0.0f;
	viewPort[0].maxDepth = 1.0f;

	std::vector<VkRect2D> scissor;
	scissor.resize(viewPort.size());
	scissor[0].offset = { 0,0 };
	scissor[0].extent = m_SwapChainExent;

	VkPipelineViewportStateCreateInfo viewPortState{};
	viewPortState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewPortState.pNext = nullptr;
	viewPortState.flags = 0;
	viewPortState.viewportCount = viewPort.size();
	viewPortState.pViewports = viewPort.data();
	viewPortState.scissorCount = scissor.size();
	viewPortState.pScissors = scissor.data();

	VkPipelineRasterizationStateCreateInfo rasterizationState{};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.pNext = nullptr;
	rasterizationState.flags = 0;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.lineWidth = 1.0f;
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.pNext = nullptr;
	multisampling.flags = 0;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.colorWriteMask = 0xF;
	blendAttachmentState.blendEnable = VK_TRUE;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.flags = 0;
	colorBlending.pNext = nullptr;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &blendAttachmentState;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

// 	A pipeline layout defines the set of resources that can be accessed from shaders of a given pipeline.
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &graphics.pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create GRAPHICS pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pVertexInputState = &vertices.inputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlending;
	pipelineCreateInfo.pMultisampleState = &multisampling;
	pipelineCreateInfo.pViewportState = &viewPortState;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.layout = graphics.pipelineLayout;
 	pipelineCreateInfo.subpass = 0;
 	pipelineCreateInfo.renderPass = m_RenderPass;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	if ((vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphics.pipeline)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_Device, geomShaderModule, nullptr);
}

void ParticleSystem::PrepareComputePipeline()
{
	auto computeShaderModule = loadShader("Shaders/shader.comp.spv", m_Device);
	
// 	VkShaderModule computeShaderModule;
// 	CreateShaderModule(m_Device, computeShaderCode, computeShaderModule);

	VkPipelineShaderStageCreateInfo computeShaderStage{};
	computeShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStage.pName = "main";
	computeShaderStage.module = computeShaderModule;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstantRange.size = sizeof(float);
	pushConstantRange.offset = 0;

	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, nullptr, &compute.pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("COULD NOT CREATE COMPUTE PIPELINE LAYOUT");
	}

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.stage = computeShaderStage;
	computePipelineCreateInfo.layout = compute.pipelineLayout;

	if (vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &compute.pipeline)
		!= VK_SUCCESS) {
		throw std::runtime_error("failed to create compute pipeline!");
	}

	vkDestroyShaderModule(m_Device, computeShaderModule, nullptr);
}

void ParticleSystem::Update()
{
}

void ParticleSystem::SetMemoryBarrier(VkBufferMemoryBarrier& memoryBarrier, BufferTransition& bufferTransition)
{
	memoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.buffer = bufferTransition.buffer;
	memoryBarrier.srcAccessMask = bufferTransition.currentAccess;
	memoryBarrier.dstAccessMask = bufferTransition.newAccess;
	memoryBarrier.size = VK_WHOLE_SIZE;
	memoryBarrier.srcQueueFamilyIndex = bufferTransition.currentQueueFamily;
	memoryBarrier.dstQueueFamilyIndex = bufferTransition.newQueueFamily;
	memoryBarrier.offset = 0;
}

void ParticleSystem::PrepareComputeCommandBuffer()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &compute.semaphore);

	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = m_QueueFamilyIndex;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkCreateCommandPool(m_Device, &cmdPoolInfo, nullptr, &compute.commandpool);

	CreateCommandBuffer(m_Device, compute.commandpool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, &compute.commandBuffer);

	//Dispatching compute commands
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkBufferMemoryBarrier memoryBarrier{};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

	BufferTransition bufferTransition;
	vkBeginCommandBuffer(compute.commandBuffer, &beginInfo);

	bufferTransition.buffer = m_StorageTexelBuffer;
	bufferTransition.currentAccess = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	bufferTransition.newAccess = VK_ACCESS_SHADER_WRITE_BIT;
	bufferTransition.currentQueueFamily = m_QueueFamilyIndex;	
	bufferTransition.newQueueFamily = m_QueueFamilyIndex;

	SetMemoryBarrier(memoryBarrier, bufferTransition);

	vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
 		0, 0, nullptr, 1, &memoryBarrier, 0, nullptr);

	// Dispatching Compute job
	vkCmdBindPipeline(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
	vkCmdBindDescriptorSets(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelineLayout, 0, 1, &m_descriptorSet.at(0), 0, 0);
	
	auto previousTime = std::chrono::high_resolution_clock::now();
	auto deltaTime = static_cast<float>((std::chrono::high_resolution_clock::now() - previousTime).count());
	std::cout << "delta time: " << deltaTime;
	vkCmdPushConstants(compute.commandBuffer, compute.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &deltaTime);
	vkCmdDispatch(compute.commandBuffer, m_ParticlePool.size(), 1, 1);

	bufferTransition.currentAccess = VK_ACCESS_SHADER_WRITE_BIT;
	bufferTransition.newAccess = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	bufferTransition.currentQueueFamily = m_QueueFamilyIndex;
	bufferTransition.newQueueFamily = m_QueueFamilyIndex;

	SetMemoryBarrier(memoryBarrier, bufferTransition);

	vkCmdPipelineBarrier(compute.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		0, 0, nullptr, 1, &memoryBarrier, 0, nullptr);
	vkEndCommandBuffer(compute.commandBuffer);
}

void ParticleSystem::SetupRenderPass()
{
	std::array<VkAttachmentDescription, 1> attachments;

	//Color Attachment
	attachments[0].flags = 0;
	attachments[0].format = swapChainParameters.Format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;

	VkAttachmentReference colorAttachmentReference;
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription;
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pDepthStencilAttachment = nullptr;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;
	subpassDescription.pResolveAttachments = nullptr;

	//Subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass)!= VK_SUCCESS) {
		throw std::runtime_error("failed to create RENDER PASS!");
	}
}

void ParticleSystem::CreateFrameBuffers()
{
	CreateSwapChainImageViews();

	std::vector<VkImageView> attachments;
	attachments.resize(1); // As we have only Color attachment

	VkFramebufferCreateInfo frameBufferCreateInfo;
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = nullptr;
	frameBufferCreateInfo.flags = 0;
	frameBufferCreateInfo.renderPass = m_RenderPass;
	frameBufferCreateInfo.attachmentCount = attachments.size();
	frameBufferCreateInfo.pAttachments = attachments.data();
	frameBufferCreateInfo.height = m_SwapChainExent.height;
	frameBufferCreateInfo.width = m_SwapChainExent.width;
	frameBufferCreateInfo.layers = 1;

	m_FrameBuffers.resize(m_SwapChainImages.size());

	for (uint32_t i = 0; i < m_FrameBuffers.size(); i++)
	{
		attachments[0] = m_ImageViews[i];
		if (vkCreateFramebuffer(m_Device, &frameBufferCreateInfo, nullptr, &m_FrameBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create FRAME BUFFER!");
		}
	}
}

void ParticleSystem::PrepareGraphicsCommandBuffer()
{
	CreateCommandPool(m_Device, m_GraphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphics.commandpool);
	
	m_DrawCmdBuffers.resize(m_SwapChainImages.size());
	CreateCommandBuffer(m_Device, graphics.commandpool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(m_SwapChainImages.size()), m_DrawCmdBuffers.data());

	SetupRenderPass();
	CreateFrameBuffers();
	
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &graphics.semaphore);

	//Recording Render Passes and Drawing commands
	VkCommandBufferBeginInfo cmdBuffBeginInfo{};
	cmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBuffBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkClearValue clearValue{
		{25.0f / 256.0f, 40.0f / 256.0f, 32.0f / 256.0f, 0.0f}
	};

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.renderArea.extent = m_SwapChainExent;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearValue;
	
	VkDeviceSize offsets[] = { 0 };
	for (uint32_t i = 0; i < m_DrawCmdBuffers.size(); i++) {
		// SET TARGET FRAME BUFFER
		renderPassBeginInfo.framebuffer = m_FrameBuffers[i];

		vkBeginCommandBuffer(m_DrawCmdBuffers[i], &cmdBuffBeginInfo);

		vkCmdBeginRenderPass(m_DrawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(m_DrawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineLayout, 0, 1, m_descriptorSet.data(), 0, NULL);

		vkCmdBindPipeline(m_DrawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipeline);
		vkCmdBindVertexBuffers(m_DrawCmdBuffers[i], 0, 1, &m_StorageTexelBuffer, offsets);
		vkCmdDraw(m_DrawCmdBuffers[i], m_ParticlePool.size(), 1, 0, 0);

		vkCmdEndRenderPass(m_DrawCmdBuffers[i]);

		vkEndCommandBuffer(m_DrawCmdBuffers[i]);

	}
}

bool ParticleSystem::PrepareFrame()
{
	// Acquire the next image from the swap chain
	VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT32_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &m_ImageIndex);
	
	switch (result)
	{
	case VK_SUCCESS: std::cout << " acquired image" << std::endl;
	case VK_SUBOPTIMAL_KHR:
		break;
	case VK_ERROR_OUT_OF_DATE_KHR:
		return OnWindowSizeChanged();

	default:
		throw std::runtime_error("COULD NOT ACQUIRE SWAPCHAIN IMAGE ");
		return false;
	}
	return true;
}

bool ParticleSystem::SubmitFrame()
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_RenderingFinishedSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_SwapChain;
	presentInfo.pImageIndices = &m_ImageIndex;

	//	pResults – A pointer to an array of at least swapchainCount element; this parameter is optional and can be set to null,
	//	but if we provide such an array, the result of the presenting operation will be stored in each of its elements, for each swap chain respectively; 
	//	a single value returned by the whole function is the same as the worst result value from all swap chains.
	presentInfo.pResults = nullptr;

	VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
	switch (result)
	{
	case VK_SUCCESS: std::cout << " presented" << std::endl;
	case VK_SUBOPTIMAL_KHR:
		break;
	case VK_ERROR_OUT_OF_DATE_KHR:
		return OnWindowSizeChanged();

	default:
		std::runtime_error("COULD NOT ACQUIRE SWAPCHAIN IMAGE ");
		return false;
	}
	return true;
}

void ParticleSystem::PrepareParticleSystem()
{
// 	if (!prepareVulkan()) {
// 		throw std::runtime_error("FAILED TO PREPARE VULKAN").what();
// 	}
// 	InitParticlesData();
//  	InitStorageTexelBuffer();
// 	InitUniformBuffer();
// 
//  	PrepareDescriptorSets();
//  	PrepareGraphicsPipeline();
// 	PrepareComputePipeline();
// 	PrepareGraphicsCommandBuffer();
// 	PrepareComputeCommandBuffer();
}

void ParticleSystem::Draw()
{
	PrepareFrame();
	UpdateUniformBuffer();
// 	If submitted commands should wait for other commands to end, initialize the vector with pipeline stages at which the queue
// 	should wait for a corresponding semaphore
	VkPipelineStageFlags graphicsWaitSemaphoreStageMask[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore graphicsWaitSemaphores[] = { compute.semaphore,m_ImageAvailableSemaphore };
	VkSemaphore graphicsSignalSemaphores[] = { graphics.semaphore, m_RenderingFinishedSemaphore };

	//Submit Graphics Commands
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_DrawCmdBuffers[m_ImageIndex];
	submitInfo.waitSemaphoreCount = 2;
	submitInfo.pWaitSemaphores = graphicsWaitSemaphores;
	submitInfo.pWaitDstStageMask = graphicsWaitSemaphoreStageMask;
	submitInfo.signalSemaphoreCount = 2;
	submitInfo.pSignalSemaphores = graphicsSignalSemaphores;

	if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE)==VK_SUCCESS) {
		std::cout << "graphics submit successful" << std::endl;
	}
	vkQueueWaitIdle(m_GraphicsQueue);
	// Submitting frame for Presentation
	SubmitFrame();
	
	// Wait for rendering finished
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

	// Submit Compute commands
	VkSubmitInfo computeSubmitInfo = {};
	computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	computeSubmitInfo.pNext = nullptr;
	computeSubmitInfo.commandBufferCount = 1;
	computeSubmitInfo.pCommandBuffers = &compute.commandBuffer;
	computeSubmitInfo.waitSemaphoreCount = 1;
	computeSubmitInfo.pWaitSemaphores = &graphics.semaphore;
	computeSubmitInfo.pWaitDstStageMask = &waitStageMask;
	computeSubmitInfo.signalSemaphoreCount = 1;
	computeSubmitInfo.pSignalSemaphores = &compute.semaphore;

	if (vkQueueSubmit(m_GraphicsQueue, 1, &computeSubmitInfo, VK_NULL_HANDLE)==VK_SUCCESS) {
		std::cout << "computer submit successful" << std::endl;
	}
	//vkQueueWaitIdle(m_GraphicsQueue);
}

// void ParticleSystem::Emit(const ParticleParameters& particleParameters)
// {
// 	Particle& particle = m_ParticlePool[currentIndex];
// 
// 	particle.canEmit = true;
// 	particle.position = particleParameters.position;
// 
// 	//Velocity
// 	particle.velocity = particleParameters.velocity;
// 	//particle.velocity.x += ;
// 	//particle.velocity.y += ;
// 
// 	//Color
// 	particle.color = particleParameters.color;
// 	
// 	particle.LifeTime = particleParameters.LifeTime;
// 	particle.LifeRemaning = particleParameters.LifeTime;
// 	particle.size = particleParameters.size;
// 
// 	currentIndex == --currentIndex % m_ParticlePool.size();
// }
