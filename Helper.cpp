#include "Helper.h"

bool Buffer::CreateBuffer(VkPhysicalDevice& m_PhysicalDevice, VkDevice& m_Device, BufferParameters& bufferParameters, VkMemoryPropertyFlags& memoryPropertyFlags, VkDeviceMemory& memoryObject)
{   
// 	if (!(format_Properties.bufferFeatures & bufferParameters.formatFeaturesBits)) {
// 		throw std::runtime_error(" Provided format not supported for STORAGE TEXEL BUFFER");
// 		return false;
// 	}

	VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = bufferParameters.size;
    bufferCreateInfo.usage = bufferParameters.usageBits;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_Device, &bufferCreateInfo, nullptr, bufferParameters.bufferHandle) != VK_SUCCESS) {
        throw std::runtime_error("Buffer could not be created");
        return false;
    }

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_Device, *bufferParameters.bufferHandle, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(m_PhysicalDevice, memRequirements.memoryTypeBits, memoryPropertyFlags);

	if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &memoryObject) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(m_Device, *bufferParameters.bufferHandle, memoryObject, 0);

    return true;
}

bool Buffer::CreateStorageTexelBuffer(VkPhysicalDevice& m_PhysicalDevice, VkDevice& m_Device, BufferParameters& bufferParameters, VkMemoryPropertyFlags& memoryPropertyFlags, VkDeviceMemory& memoryObject)
{
	VkFormatProperties format_Properties;

	vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, bufferParameters.format, &format_Properties);
	if (!(format_Properties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT)) {
		throw std::runtime_error(" Provided format not supported for STORAGE TEXEL BUFFER");
		return false;
	}

	CreateBuffer(m_PhysicalDevice, m_Device, bufferParameters, memoryPropertyFlags, memoryObject);
	return true;
}

uint32_t Buffer::findMemoryType(VkPhysicalDevice& m_PhysicalDevice,uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}
// bool Buffer::AllocateAndBindMemoryObject(VkPhysicalDevice& m_PhysicalDevice, VkDevice& m_Device, VkMemoryPropertyFlags& memoryPropertyFlags, VkBuffer& buffer, VkDeviceMemory& memoryObject)
// {
// 	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
// 	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &physicalDeviceMemoryProperties);
// 
// 	// 	we need to know how much storage a given buffer requires (the buffer's memory may
// 	// 		need to be bigger than the buffer's size) 
// 	VkMemoryRequirements memoryRequirements;
// 	vkGetBufferMemoryRequirements(m_Device, buffer, &memoryRequirements);
// 
// 	memoryObject = VK_NULL_HANDLE;
// 
// 	for (uint32_t type = 0; type < physicalDeviceMemoryProperties.memoryTypeCount; type++) {
// 		if ((memoryRequirements.memoryTypeBits & (1 << type)) &&
// 			((physicalDeviceMemoryProperties.memoryTypes[type].propertyFlags) == memoryPropertyFlags)) {
// 
// 			VkMemoryAllocateInfo memoryAllocateInfo;
// 			memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
// 			memoryAllocateInfo.pNext = nullptr;
// 			memoryAllocateInfo.allocationSize = memoryRequirements.size;
// 			memoryAllocateInfo.memoryTypeIndex = type;
// 
// 			if (!(vkAllocateMemory(m_Device, &memoryAllocateInfo, nullptr, &memoryObject) == VK_SUCCESS)) {
// 				std::runtime_error("****Could not Allocate MEMORY for BUFFER****");
// 				return false;
// 			}
// 			break;
// 		}
// 	}
// 
// 	if (!(vkBindBufferMemory(m_Device, buffer, memoryObject, 0) == VK_SUCCESS)) {
// 		std::runtime_error("****Error binding Buffer with Memory Object****");
// 		return false;
// 	}
//     return true;
// }

bool Buffer::CreateBufferView(VkDevice& device, VkBuffer& buffer, const VkDeviceSize& size, VkBufferView& bufferView)
{
	VkBufferViewCreateInfo bufferViewCreateInfo;
	bufferViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
	bufferViewCreateInfo.pNext = nullptr;
	bufferViewCreateInfo.flags = 0;
	bufferViewCreateInfo.buffer = buffer;

	// For how the buffer's contents should be interpreted
	bufferViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	bufferViewCreateInfo.offset = 0;
	bufferViewCreateInfo.range = VK_WHOLE_SIZE;

	if ((vkCreateBufferView(device, &bufferViewCreateInfo, nullptr, &bufferView)) != VK_SUCCESS) {
		throw std::runtime_error("COULD NOT CREATE BUFFER VIEW");
	}
	return true;
}

