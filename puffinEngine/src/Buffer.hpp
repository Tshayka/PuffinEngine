#pragma once

#include <assert.h>
#include <cstring>
#include <string>
#include <memory>

#include "Device.hpp"

namespace enginetool {
	class Buffer {
	public:
		Buffer();
		Buffer(Device* device);
		~Buffer();

		// TODO rule of five

		VkBuffer& getBuffer();

		void* getMapped() const;
		void setMapped(void* map);
		void setDevice(Device* device);
		void Copy(VkDeviceSize size, void* data);
		void map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void Unmap();
		VkResult Bind(VkDeviceSize offset = 0);
		void SetupDescriptor(VkDeviceSize size, VkDeviceSize offset = 0);
		VkResult Flush(VkDeviceSize size, VkDeviceSize offset = 0);
		VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void Destroy();
		void createStagedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, void* data = nullptr);
		void createUnstagedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

		VkDeviceMemory m_Memory;
		VkDeviceSize m_Alignment;

		VkDescriptorBufferInfo m_Descriptor;
		VkBufferUsageFlags m_UsageFlags;
		VkMemoryPropertyFlags m_MemoryPropertyFags;

		Device* m_Device;

	private:
		VkBuffer m_Buffer;
		void* p_Mapped = nullptr;
	};
}