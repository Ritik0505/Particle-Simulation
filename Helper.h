#pragma once
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR

#include"Vulkan/Vulkan.h"
#include "VulkanFunctions.h"
#include<string>
#include <iostream>
#include<fstream>
#include<vector>
#include<array>

struct VulkanHandles {
protected:
	VkInstance m_Instance = VK_NULL_HANDLE;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device = VK_NULL_HANDLE;
	uint32_t m_QueueFamilyIndex = VK_NULL_HANDLE;
	VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
	VkQueue m_PresentQueue = VK_NULL_HANDLE;
	uint32_t m_GraphicsQueueFamilyIndex = 0;
	uint32_t m_PresentationQueueFamilyIndex = 0;
	VkSurfaceKHR m_PresentationSurface = VK_NULL_HANDLE;
	VkExtent2D m_SwapChainExent;
	VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
	VkSemaphore m_ImageAvailableSemaphore = VK_NULL_HANDLE;
	VkSemaphore m_RenderingFinishedSemaphore = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_PresentQueueCommandBuffers;
	VkCommandPool m_PresentQueueCommandPool = VK_NULL_HANDLE;
	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

	struct {
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkCommandPool commandpool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkSemaphore semaphore = VK_NULL_HANDLE;
	}graphics;

	struct {
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkCommandPool commandpool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		VkSemaphore semaphore = VK_NULL_HANDLE;
	}compute;

	VkDescriptorSetLayout m_descriptorSetLayout;
	std::vector<VkDescriptorSet> m_descriptorSet;
	VkRenderPass m_RenderPass;
	std::vector<VkCommandBuffer> m_DrawCmdBuffers;
	std::vector<VkFramebuffer> m_FrameBuffers;

	std::vector<VkImage> m_SwapChainImages;
	std::vector<VkImageView> m_ImageViews;
	uint32_t m_ImageIndex; //Index of Acquired Image from SwapChain
};

// struct QueueParameters {
// 	VkQueue                       Handle;
// 	uint32_t                      FamilyIndex;
// 
// 	QueueParameters() :
// 		Handle(VK_NULL_HANDLE),
// 		FamilyIndex(0) {
// 	}
// };
// 	
struct ImageParameters {
	VkImage                       Handle;
	VkImageView                   View;
	VkSampler                     Sampler;
	VkDeviceMemory                Memory;

	ImageParameters() :
		Handle(VK_NULL_HANDLE),
		View(VK_NULL_HANDLE),
		Sampler(VK_NULL_HANDLE),
		Memory(VK_NULL_HANDLE) {
	}
};

struct SwapChainParameters {
	VkSwapchainKHR                Handle;
	VkFormat                      Format;
	VkExtent2D                    Extent;

	SwapChainParameters() :
		Handle(VK_NULL_HANDLE),
		Format(VK_FORMAT_UNDEFINED),
		Extent() {
	}
}static swapChainParameters;
// 
// struct VulkanCommonParameters {
// 	VkInstance                    Instance;
// 	VkPhysicalDevice              PhysicalDevice;
// 	VkDevice                      Device;
// 	QueueParameters               GraphicsQueue;
// 	QueueParameters               PresentQueue;
// 	VkSurfaceKHR                  PresentationSurface;
// 	SwapChainParameters           SwapChain;
// 
// 	VulkanCommonParameters() :
// 		Instance(VK_NULL_HANDLE),	
// 		PhysicalDevice(VK_NULL_HANDLE),
// 		Device(VK_NULL_HANDLE),
// 		GraphicsQueue(),
// 		PresentQueue(),
// 		PresentationSurface(VK_NULL_HANDLE),
// 		SwapChain() {
// 	}
// };

// For Holding Parameters for Memory Barrier
struct BufferTransition {
	VkBuffer buffer;
	VkAccessFlags currentAccess;	// how the buffer has been used so far
	VkAccessFlags newAccess;	// how the buffer will be used from now on 
	uint32_t currentQueueFamily;
	uint32_t newQueueFamily;
};

struct BufferParameters {
	VkBuffer* bufferHandle;
	VkFormat format;
	VkFormatFeatureFlags formatFeaturesBits;
	VkBufferUsageFlags usageBits;
	uint32_t size;
};

//Custom structures for Updating Descriptor Sets
struct BufferDescriptorInfo {
	VkDescriptorSet TargetDescriptorSet;
	uint32_t TargetDescriptorBindidng;
	uint32_t TargetArrayElement;
	VkDescriptorType TargetDescriptorType;
	std::vector<VkDescriptorBufferInfo> BufferInfos;
};

struct TexelBufferDescriptorInfo {
	VkDescriptorSet TargetDescriptorSet;
	uint32_t TargetDescriptorBinding;
	uint32_t TargetArrayElement;
	VkDescriptorType TargetDescriptorType;
	std::vector<VkBufferView> TexelBufferViews;
};

struct Buffer{
	bool CreateBuffer(VkPhysicalDevice& m_PhysicalDevice, VkDevice& m_Device, BufferParameters& bufferParameters, VkMemoryPropertyFlags& memoryPropertyFlags, VkDeviceMemory& memoryObject);
	bool CreateStorageTexelBuffer(VkPhysicalDevice& m_PhysicalDevice, VkDevice& m_Device, BufferParameters& bufferParameters, VkMemoryPropertyFlags& memoryPropertyFlags, VkDeviceMemory& memoryObject);
	//bool AllocateAndBindMemoryObject(VkPhysicalDevice& m_PhysicalDevice, VkDevice& m_Device, VkMemoryPropertyFlags& memoryProperties, VkBuffer& buffer, VkDeviceMemory& memoryObject);
	uint32_t findMemoryType(VkPhysicalDevice& m_PhysicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	bool CreateBufferView(VkDevice& device, VkBuffer& buffer, const VkDeviceSize& size, VkBufferView& bufferView);

	~Buffer() {
		// destroy bufferview-> free memory-> destroy buffer;
	}
};

// inline std::vector<char> LoadShader(const std::string& Filename) {
// 
// //		ate: Start reading at the end of the file
// //		binary : Read the file as binary file(avoid text transformations)
// 
// // The advantage of starting to read at the end of the file is that 
// //	we can use the read position to determine the size of the file and allocate a buffer :
// 
// 	std::ifstream file(Filename, std::ios::in | std::ios::ate | std::ios::binary);
// 
// 	if (!file.is_open()) {
// 		std::cerr << "Failed to Load Shader" << std::endl;
// 	}
// 
// 	size_t fileSize = (size_t)file.tellg();
// 	std::vector<char> buffer(fileSize);
// 
// //	Setting Pointer to Beginning
// 	file.seekg(0);
// 	file.read(buffer.data(), fileSize);
// 
// 	file.close();
// 	return buffer;
// }
// 
// inline void CreateShaderModule(VkDevice& device, std::vector<char>sourceCode, VkShaderModule& module) {
// 	VkShaderModuleCreateInfo moduleCreateInfo;
// 	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
// 	moduleCreateInfo.pNext = nullptr;
// 	moduleCreateInfo.flags = 0;
// 	moduleCreateInfo.codeSize = sourceCode.size();
// 	moduleCreateInfo.pCode = reinterpret_cast<uint32_t const*>(sourceCode.data());
// 
// 	if (vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &module) != VK_SUCCESS) {
// 		throw "COULD NOT CREATE SHADER MODULE";
// 	}
// }

inline void CreateCommandPool(VkDevice& Device, uint32_t QueuefamilyIndex, VkCommandPoolCreateFlags CreateFlags, VkCommandPool& CmdPool) {
	VkCommandPoolCreateInfo CmdPoolCreateInfo{};
	CmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CmdPoolCreateInfo.flags = CreateFlags;

	if ((vkCreateCommandPool(Device, &CmdPoolCreateInfo, nullptr, &CmdPool)) != VK_SUCCESS) {
		throw std::runtime_error("COULD NOT CREATE COMMAND POOL");
	}
}

inline void CreateCommandBuffer(VkDevice& Device, VkCommandPool& commandPool, VkCommandBufferLevel cmdBuffLevel, const uint32_t BufferCount, VkCommandBuffer* CmdBuffer) {
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = cmdBuffLevel;
	commandBufferAllocateInfo.commandBufferCount = BufferCount;

	if ((vkAllocateCommandBuffers(Device, &commandBufferAllocateInfo, CmdBuffer)) != VK_SUCCESS) {
		throw std::runtime_error("COULD NOT ALLOCATE COMMAND BUFFER");
	}
}

inline void flushCommandBuffer(VkDevice& m_Device, VkCommandBuffer& commandBuffer, VkQueue& queue, VkCommandPool& pool, bool free)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;

	VkFence fence;
	vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &fence);

	// Submit to the queue
	vkQueueSubmit(queue, 1, &submitInfo, fence);
	
	// Wait for the fence to signal that command buffer has finished executing
	vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX);
	vkDestroyFence(m_Device, fence, nullptr);
	if (free)
	{
		vkFreeCommandBuffers(m_Device, pool, 1, &commandBuffer);
	}
}

inline VkShaderModule loadShader(const char* fileName, VkDevice device)
{
	std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

	if (is.is_open())
	{
		size_t size = is.tellg();
		is.seekg(0, std::ios::beg);
		char* shaderCode = new char[size];
		is.read(shaderCode, size);
		is.close();

		//assert(size > 0);

		VkShaderModule shaderModule;
		VkShaderModuleCreateInfo moduleCreateInfo{};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;

		vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule);

		delete[] shaderCode;

		return shaderModule;
	}
	else
	{
		std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << "\n";
		return VK_NULL_HANDLE;
	}
}