#pragma once

#include "glm/vec2.hpp"
#include "glm/vec4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "VulkanFunctions.h"
#include <vector>
#include <random>
#include <chrono>
#include "Helper.h"
#include "VulkanBase.h"

struct ParticleParameters {
	glm::vec2 position;
	glm::vec4 color;
	glm::vec2 velocity;
	//float LifeTime = 1.0f;
	//float size;
};

class ParticleSystem : public VulkanBase
{
public:
	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	}vertices;

// 	struct {
// 		VkDescriptorSetLayout descriptorSetLayout;
// 		VkDescriptorSet descriptorSet;
// 		VkPipelineLayout pipelineLayout;
// 		VkPipeline pipeline;
// 		VkSemaphore semaphore;
// 	}graphics;
// 
// 	struct {
// 		VkPipelineLayout pipelineLayout;
// 		VkPipeline pipeline;
// 		VkCommandPool commandpool;
// 		VkCommandBuffer commandBuffer;
// 		VkSemaphore semaphore;
// 	}compute;

	ParticleSystem();

	void InitParticlesData();
	void InitStorageTexelBuffer();
	void InitUniformBuffer();
	void UpdateUniformBuffer();
	void CopyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, uint32_t& bufferSize);

	void PrepareDescriptorSets();
	void PrepareGraphicsPipeline();
	void PrepareComputePipeline();

	void Update();
	void Render();
//	TODO: Wait for signal from Compute stage then render;

	void SetMemoryBarrier(VkBufferMemoryBarrier& memoryBarrier, BufferTransition& bufferTransition);
	void PrepareComputeCommandBuffer();
	void SetupRenderPass();
	void CreateFrameBuffers();
	void PrepareGraphicsCommandBuffer();
	bool PrepareFrame();
	bool SubmitFrame();
	void PrepareParticleSystem();
	void Draw();
//	TODO : 
//	1.Submit graphics commands - semaphores(wait-compute, present; signal-graphics,rendercomplete)
//	2. Wait for rendering finished
//	3.Submit compute commands - semaphores(wait-graphics ; signal-compute)

//	void Emit(const ParticleParameters& particleParameters);

private:
	struct Particle {
		glm::vec4 position;
		glm::vec4 color;
// 		glm::vec2 velocity;
		
// 		float size;
// 		float LifeTime = 1.0f;
// 		float LifeRemaning = 1.0f;
// 
// 		bool canEmit = false;
	};

	glm::mat4 m_MVP;

	std::vector<Particle> m_ParticlePool;
	uint32_t currentIndex = 0, maxIndex = 999;
	VkBuffer m_StorageTexelBuffer, m_UniformBuffer;
	VkBufferView m_StorageTexelBufferView;
	VkDeviceMemory m_StorageTexelBufferMemoryObject, m_UniformBufferMemoryObject;

// 	VkDescriptorSetLayout m_descriptorSetLayout;
// 	std::vector<VkDescriptorSet> m_descriptorSet;
// 	VkRenderPass m_RenderPass;
// 	std::vector<VkCommandBuffer> m_DrawCmdBuffers;
// 	std::vector<VkFramebuffer> m_FrameBuffers;
// 	uint32_t m_ImageIndex; //Index of Acquired Image from SwapChain
};

