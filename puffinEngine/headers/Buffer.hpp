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
		Buffer(const Buffer& src) = delete;
		~Buffer();

		// TODO rule of five

		VkBuffer& getBuffer();
		VkDeviceMemory& getMemory();
		void setMemory(const VkDeviceMemory& memory);
		void* getMapped() const;
		void setMapped(void* map);
		void setDevice(Device* device);
		Device* getDevice();

		void copy(VkDeviceSize size, void* data);
		void map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void unmap();
		VkResult bind(VkDeviceSize offset = 0);
		void setupDescriptor(VkDeviceSize size, VkDeviceSize offset = 0);
		VkResult flush(VkDeviceSize size, VkDeviceSize offset = 0);
		VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void destroy();
		void createStagedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, void* data = nullptr);
		void createUnstagedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

		VkDeviceSize m_Alignment;
		VkDescriptorBufferInfo m_Descriptor;
		VkBufferUsageFlags m_UsageFlags;
		VkMemoryPropertyFlags m_MemoryPropertyFags;

	private:
		VkBuffer m_Buffer;
		Device* m_Device;
		void* p_Mapped;
		VkDeviceMemory m_Memory;
	};
}