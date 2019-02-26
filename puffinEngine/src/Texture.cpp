#include <iostream>
#include "ErrorCheck.hpp"
#include "Texture.hpp"

//---------- Constructors and dectructors ---------- //

TextureLayout::TextureLayout() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Texture created\n";
#endif 
}

TextureLayout::~TextureLayout() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Texture destroyed\n";
#endif
}

// --------------- Setters and getters -------------- //



// ---------------- Main functions ------------------ //

void TextureLayout::Init(Device* device, VkCommandPool& commandPool, VkFormat format, uint32_t baseMipLevel, uint32_t mipLevels, uint32_t layers) {
	logicalDevice = device;
	this->format = format;
	this->baseMipLevel = baseMipLevel;
	this->mipLevels = mipLevels;
    this->layers = layers;
	this->commandPool = &commandPool;
};

void TextureLayout::CreateImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageCreateFlags flag) {
	VkImageCreateInfo ImageInfo = {};
	ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageInfo.format = format;
	ImageInfo.extent.width = texWidth;
	ImageInfo.extent.height = texHeight;
	ImageInfo.extent.depth = 1;
	ImageInfo.mipLevels = mipLevels;
	ImageInfo.arrayLayers = layers;
	ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageInfo.tiling = tiling;
	ImageInfo.usage = usage;
	ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ImageInfo.flags = flag;

	ErrorCheck(vkCreateImage(logicalDevice->device, &ImageInfo, nullptr, &texture));

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(logicalDevice->device, texture, &memory_requirements); // TODO
	
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memory_requirements.size;
	allocInfo.memoryTypeIndex = logicalDevice->FindMemoryType(memory_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(logicalDevice->device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to allocate image memory!");
		std::exit(-1);
	}

	ErrorCheck(vkBindImageMemory(logicalDevice->device, texture, memory, 0));
}

void TextureLayout::CreateImageView(VkImageAspectFlags aspect_flags, VkImageViewType type) {
	VkImageViewCreateInfo ViewInfo = {};
	ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ViewInfo.image = texture;
	ViewInfo.viewType = type;
	ViewInfo.format = format;
	ViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	ViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	ViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	ViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	ViewInfo.subresourceRange.aspectMask = aspect_flags;
	ViewInfo.subresourceRange.baseMipLevel = baseMipLevel;
	ViewInfo.subresourceRange.levelCount = mipLevels;
	ViewInfo.subresourceRange.baseArrayLayer = 0;
	ViewInfo.subresourceRange.layerCount = layers;

	if (vkCreateImageView(logicalDevice->device, &ViewInfo, nullptr, &view) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to create texture image view!");
		std::exit(-1);
	}
}

void TextureLayout::CreateTextureSampler(enum VkSamplerAddressMode mode) {
	VkSamplerCreateInfo SamplerInfo = {};
	SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	SamplerInfo.magFilter = VK_FILTER_LINEAR;
	SamplerInfo.minFilter = VK_FILTER_LINEAR;
	SamplerInfo.addressModeU = mode;
	SamplerInfo.addressModeV = mode;
	SamplerInfo.addressModeW = mode; 
	SamplerInfo.anisotropyEnable = VK_TRUE;
	SamplerInfo.maxAnisotropy = 16.0f;
	SamplerInfo.minLod = 0.0f;
	SamplerInfo.maxLod = static_cast<float>(baseMipLevel);
	SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	SamplerInfo.unnormalizedCoordinates = VK_FALSE;
	SamplerInfo.compareEnable = VK_FALSE;
	SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(logicalDevice->device, &SamplerInfo, nullptr, &sampler) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to create texture sampler!");
		std::exit(-1);
	}
}

void TextureLayout::CopyBufferToImage(VkBuffer buffer) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	uint32_t offset = 0;
	
	for (uint32_t face = 0; face < layers; face++) {
		for (uint32_t level = 0; level < mipLevels; level++) {
			VkBufferImageCopy Region = {};
			Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			Region.imageSubresource.mipLevel = level;
			Region.imageSubresource.baseArrayLayer = face;
			Region.imageSubresource.layerCount = 1;
			Region.imageExtent.width = texWidth; 
			Region.imageExtent.height = texHeight;
			Region.imageExtent.depth = 1;
			Region.bufferOffset = offset;
			Region.bufferRowLength = 0;
			Region.bufferImageHeight = 0;
			Region.imageOffset = { 0, 0, 0 };

			bufferCopyRegions.emplace_back(Region);
		}
	}

	vkCmdCopyBufferToImage(commandBuffer, buffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());
	
	EndSingleTimeCommands(commandBuffer);
}

bool TextureLayout::HasStencilComponent() {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void TextureLayout::TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = texture;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (HasStencilComponent()) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layers;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		assert(0 && "Vulkan ERROR: unsupported layout transition!");
		std::exit(-1);
	}

	vkCmdPipelineBarrier(command_buffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	EndSingleTimeCommands(command_buffer);
}

VkCommandBuffer TextureLayout::BeginSingleTimeCommands() {
	VkCommandBufferAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocInfo.commandPool = *commandPool;
	AllocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDevice->device, &AllocInfo, &commandBuffer);

	VkCommandBufferBeginInfo BeginInfo = {};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &BeginInfo);

	return commandBuffer;
}

void TextureLayout::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(logicalDevice->queue, 1, &SubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(logicalDevice->queue);

	vkFreeCommandBuffers(logicalDevice->device, *commandPool, 1, &commandBuffer);
}

void TextureLayout::DeInit() {
	if(sampler) vkDestroySampler(logicalDevice->device, sampler, nullptr);
	vkDestroyImageView(logicalDevice->device, view, nullptr);
	vkDestroyImage(logicalDevice->device, texture, nullptr);
	vkFreeMemory(logicalDevice->device, memory, nullptr);
};