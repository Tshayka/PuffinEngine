#include <assert.h>
#include <cstring>
#include <string>
#include <vulkan/vulkan.h>

namespace enginetool
{
	struct Buffer
	{
		VkDevice device = VK_NULL_HANDLE;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDeviceSize size = 0;
		VkDeviceSize alignment = 0;
		void* mapped = nullptr;

		VkDescriptorBufferInfo descriptor;
		VkBufferUsageFlags usage_flags;
		VkMemoryPropertyFlags memory_property_flags;

		VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) {
			return vkMapMemory(device, memory, offset, size, 0, &mapped);
		}

		void Unmap() {
			if (mapped) {
				vkUnmapMemory(device, memory);
				mapped = nullptr;
			}
		}

		VkResult Bind(VkDeviceSize offset = 0) {
			return vkBindBufferMemory(device, buffer, memory, offset);
		}

		void SetupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) {
			descriptor.offset = offset;
			descriptor.buffer = buffer;
			descriptor.range = size;
		}

		void CopyTo(void* data, VkDeviceSize size)
		{
			assert(mapped);
			memcpy(mapped, data, size);
		}


		VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) {
			VkMappedMemoryRange MappedRange = {};
			MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			MappedRange.memory = memory;
			MappedRange.offset = offset;
			MappedRange.size = size;
			return vkFlushMappedMemoryRanges(device, 1, &MappedRange);
		}

		VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)	{
			VkMappedMemoryRange MappedRange = {};
			MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			MappedRange.memory = memory;
			MappedRange.offset = offset;
			MappedRange.size = size;
			return vkInvalidateMappedMemoryRanges(device, 1, &MappedRange);
		}

		void Destroy() {
			if (buffer) {
				vkDestroyBuffer(device, buffer, nullptr);
			}
			if (memory) {
				vkFreeMemory(device, memory, nullptr);
			}
		}

	};
}