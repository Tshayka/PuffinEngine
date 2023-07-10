#include <assert.h>
#include <cstring>
#include <string>
#include <memory>
#include <vulkan/vulkan.h>

namespace enginetool {
	class Buffer {
	public:
		VkBuffer& getBuffer() {
			return m_Buffer;
		}

		void* getMapped() const {
			return p_Mapped.get();
		}

		void Copy(VkDeviceSize size, void* data) {
			memcpy(p_Mapped.get(), data, size);
		}

		VkResult Map(VkDeviceSize size, VkDeviceSize offset = 0) {
			return vkMapMemory(m_Device, m_Memory, offset, size, 0, p_Mapped.get());
		}

		void Unmap() {
			if (p_Mapped) {
				vkUnmapMemory(m_Device, m_Memory);
			}
		}

		VkResult Bind(VkDeviceSize offset = 0) {
			return vkBindBufferMemory(m_Device, m_Buffer, m_Memory, offset);
		}

		void SetupDescriptor(VkDeviceSize size, VkDeviceSize offset = 0) {
			m_Descriptor.offset = offset;
			m_Descriptor.buffer = m_Buffer;
			m_Descriptor.range = size;
		}

		void CopyTo(void* data, VkDeviceSize size) {
			assert(p_Mapped);
			memcpy(p_Mapped.get(), data, size);
		}

		VkResult Flush(VkDeviceSize size, VkDeviceSize offset = 0) {
			VkMappedMemoryRange MappedRange = {};
			MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			MappedRange.memory = m_Memory;
			MappedRange.offset = offset;
			MappedRange.size = size;
			return vkFlushMappedMemoryRanges(m_Device, 1, &MappedRange);
		}

		VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)	{
			VkMappedMemoryRange MappedRange = {};
			MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			MappedRange.memory = m_Memory;
			MappedRange.offset = offset;
			MappedRange.size = size;
			return vkInvalidateMappedMemoryRanges(m_Device, 1, &MappedRange);
		}

		void Destroy() {
			if (m_Buffer) {
				vkDestroyBuffer(m_Device, m_Buffer, nullptr);
			}
			if (m_Memory) {
				vkFreeMemory(m_Device, m_Memory, nullptr);
			}
		}

		VkDevice m_Device;
		VkDeviceMemory m_Memory;
		VkDeviceSize m_Alignment = 0;

		VkDescriptorBufferInfo m_Descriptor;
		VkBufferUsageFlags m_UsageFlags;
		VkMemoryPropertyFlags m_MemoryPropertyFags;

	private:
		VkBuffer m_Buffer;
		std::unique_ptr<void*> p_Mapped;
	};
}