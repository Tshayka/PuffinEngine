#pragma once

#include "Buffer.hpp"
#include "ErrorCheck.hpp"

using namespace enginetool;

Buffer::Buffer() {
	m_Descriptor = { 0,0,0 };
	m_UsageFlags = 0;
	m_MemoryPropertyFags = 0;
	m_Device = 0;
	m_Alignment = 0;
	m_Memory = 0;
	m_Buffer = VK_NULL_HANDLE;
};

Buffer::Buffer(Device* device) {
	m_Descriptor = {0,0,0};
	m_UsageFlags = 0;
	m_MemoryPropertyFags = 0;
	m_Device = device;
	m_Alignment = 0;
	m_Memory = 0;
	m_Buffer = VK_NULL_HANDLE;

#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Buffer object created\n";
#endif 
}

Buffer::~Buffer() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Buffer object destroyed\n";
#endif 
}

// TODO rule of five

VkBuffer& Buffer::getBuffer() {
	return m_Buffer;
}

void* Buffer::getMapped() const {
	return p_Mapped;
}

void Buffer::setMapped(void* map) {
	p_Mapped = map;
}

void Buffer::setDevice (Device *device) {
	m_Device = device;
}

void Buffer::Copy(VkDeviceSize size, void* data) {
	memcpy(p_Mapped, data, size);
}

void Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
	vkMapMemory(m_Device->get(), m_Memory, offset, size, 0, &p_Mapped);
}

void Buffer::Unmap() {
	if (p_Mapped) {
		vkUnmapMemory(m_Device->get(), m_Memory);
		p_Mapped = nullptr;
	}
}

VkResult Buffer::Bind(VkDeviceSize offset) {
	return vkBindBufferMemory(m_Device->get(), m_Buffer, m_Memory, offset);
}

void Buffer::SetupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
	m_Descriptor.offset = offset;
	m_Descriptor.buffer = m_Buffer;
	m_Descriptor.range = size;
}

VkResult Buffer::Flush(VkDeviceSize size, VkDeviceSize offset) {
	VkMappedMemoryRange MappedRange = {};
	MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	MappedRange.memory = m_Memory;
	MappedRange.offset = offset;
	MappedRange.size = size;
	return vkFlushMappedMemoryRanges(m_Device->get(), 1, &MappedRange);
}

VkResult Buffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)	{
	VkMappedMemoryRange MappedRange = {};
	MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	MappedRange.memory = m_Memory;
	MappedRange.offset = offset;
	MappedRange.size = size;
	return vkInvalidateMappedMemoryRanges(m_Device->get(), 1, &MappedRange);
}

void Buffer::Destroy() {
	if (m_Buffer) {
		vkDestroyBuffer(m_Device->get(), m_Buffer, nullptr);
	}
	if (m_Memory) {
		vkFreeMemory(m_Device->get(), m_Memory, nullptr);
	}
}

void Buffer::createStagedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, void* data) {
	VkBufferCreateInfo BufferInfo = {};
	BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferInfo.size = size;
	BufferInfo.usage = usage;
	BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	ErrorCheck(vkCreateBuffer(m_Device->get(), &BufferInfo, nullptr, &m_Buffer));

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(m_Device->get(), m_Buffer, &memory_requirements);

	VkMemoryAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocInfo.allocationSize = memory_requirements.size;
	// Find a memory type index that fits the properties of the buffer
	AllocInfo.memoryTypeIndex = m_Device->FindMemoryType(memory_requirements.memoryTypeBits, properties);

	ErrorCheck(vkAllocateMemory(m_Device->get(), &AllocInfo, nullptr, &m_Memory)); // in a real world application, you're not supposed to call vkAllocateMemory for every individual buffer! use VulkanMemoryAllocator

	m_Alignment = memory_requirements.alignment;
	//buffer->size = AllocInfo.allocationSize;
	m_UsageFlags = usage;
	m_MemoryPropertyFags = properties;

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (data != nullptr) {
		map(size);
		Copy(size, data);
		Unmap();
	}

	// Initialize a default descriptor that covers the whole buffer size
	SetupDescriptor(size);

	// Attach the memory to the buffer object
	Bind();
}

void Buffer::createUnstagedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
	VkBufferCreateInfo BufferInfo = {};
	BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferInfo.size = size;
	BufferInfo.usage = usage;
	BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	ErrorCheck(vkCreateBuffer(m_Device->get(), &BufferInfo, nullptr, &m_Buffer));

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(m_Device->get(), m_Buffer, &memory_requirements);

	VkMemoryAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocInfo.allocationSize = memory_requirements.size;
	// Find a memory type index that fits the properties of the buffer
	AllocInfo.memoryTypeIndex = m_Device->FindMemoryType(memory_requirements.memoryTypeBits, properties);
	ErrorCheck(vkAllocateMemory(m_Device->get(), &AllocInfo, nullptr, &m_Memory)); // in a real world application, you're not supposed to call vkAllocateMemory for every individual buffer! use VulkanMemoryAllocator

	m_Alignment = memory_requirements.alignment;
	//buffer->size = AllocInfo.allocationSize;
	m_UsageFlags = usage;
	m_MemoryPropertyFags = properties;

	// Initialize a default descriptor that covers the whole buffer size
	SetupDescriptor(size);

	// Attach the memory to the buffer object
	Bind();
}