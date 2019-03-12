#include <assert.h>
#include <cstring>
#include <string>
#include <vulkan/vulkan.h>

#include "Device.hpp"
#include "ErrorCheck.hpp"

namespace enginetool {
template <typename T>
	class Buffer {
		public:
		Device* logicalDevice = nullptr;
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDeviceSize size = 0;
		VkDeviceSize alignment = 0;
		T* mapped = nullptr;

		VkDescriptorBufferInfo descriptor;
		VkBufferUsageFlags usageFlags;
		VkMemoryPropertyFlags memoryPropertyFlags;

		VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) {
			return vkMapMemory(logicalDevice->device, memory, offset, size, 0, (void**)&mapped);
		}

		void Unmap() {
			if (mapped) {
				vkUnmapMemory(logicalDevice->device, memory);
				mapped = nullptr;
			}
		}

		VkResult Bind(VkDeviceSize offset = 0) {
			return vkBindBufferMemory(logicalDevice->device, buffer, memory, offset);
		}

		void CopyTo(void* data, VkDeviceSize size) {
			assert(mapped);
			memcpy(mapped, data, size);
		}

		void CreateBuffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, void* data){
			logicalDevice = device;

			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			ErrorCheck(vkCreateBuffer(logicalDevice->device, &bufferInfo, nullptr, &buffer));

			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(logicalDevice->device, buffer, &memoryRequirements);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memoryRequirements.size;
			allocInfo.memoryTypeIndex = logicalDevice->FindMemoryType(memoryRequirements.memoryTypeBits, properties);
			
			ErrorCheck(vkAllocateMemory(logicalDevice->device, &allocInfo, nullptr, &memory)); // in a real world application, you're not supposed to call vkAllocateMemory for every individual buffer! use VulkanMemoryAllocator

			alignment = memoryRequirements.alignment;
			this->size = allocInfo.allocationSize;
			usageFlags = usage;
			memoryPropertyFlags = properties;

			// If a pointer to the buffer data has been passed, map the buffer and copy over the data
			if (data != nullptr) {
				ErrorCheck(Map());
				memcpy(mapped, data, size);
				Unmap();
			}

			SetupDescriptor();
			Bind();
		}

		void SetupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) {
			descriptor.offset = offset;
			descriptor.buffer = buffer;
			descriptor.range = size;
		}

		VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) {
			VkMappedMemoryRange MappedRange = {};
			MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			MappedRange.memory = memory;
			MappedRange.offset = offset;
			MappedRange.size = size;
			return vkFlushMappedMemoryRanges(logicalDevice->device, 1, &MappedRange);
		}

		VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)	{
			VkMappedMemoryRange MappedRange = {};
			MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			MappedRange.memory = memory;
			MappedRange.offset = offset;
			MappedRange.size = size;
			return vkInvalidateMappedMemoryRanges(logicalDevice->device, 1, &MappedRange);
		}

		void Destroy() {
			if (mapped) {
				vkUnmapMemory(logicalDevice->device, memory);
				mapped = nullptr;
			}
			if (buffer) {
				vkDestroyBuffer(logicalDevice->device, buffer, nullptr);
			}
			if (memory) {
				vkFreeMemory(logicalDevice->device, memory, nullptr);
			}
			logicalDevice = nullptr;
		}
	};
}