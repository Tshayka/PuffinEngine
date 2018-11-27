#include "ErrorCheck.hpp"
#include "Texture.hpp"

void TextureLayout::Init(Device* device, VkFormat format) {
	logicalDevice = device;
	this->format = format;
};

void TextureLayout::CreateImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
	VkImageCreateInfo ImageInfo = {};
	ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageInfo.format = format;
	ImageInfo.extent.width = texWidth;
	ImageInfo.extent.height = texHeight;
	ImageInfo.extent.depth = 1;
	ImageInfo.mipLevels = 1;
	ImageInfo.arrayLayers = 1;
	ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageInfo.tiling = tiling;
	ImageInfo.usage = usage;
	ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	ErrorCheck(vkCreateImage(logicalDevice->device, &ImageInfo, nullptr, &texture));

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(logicalDevice->device, texture, &memory_requirements); // TODO
	
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memory_requirements.size;
	allocInfo.memoryTypeIndex = logicalDevice->FindMemoryType(memory_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(logicalDevice->device, &allocInfo, nullptr, &texture_image_memory) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to allocate image memory!");
		std::exit(-1);
	}

	ErrorCheck(vkBindImageMemory(logicalDevice->device, texture, texture_image_memory, 0));
}

void TextureLayout::CreateImageView(VkImageAspectFlags aspect_flags) {
	VkImageViewCreateInfo ViewInfo = {};
	ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ViewInfo.image = texture;
	ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	ViewInfo.format = format;
	ViewInfo.subresourceRange.aspectMask = aspect_flags;
	ViewInfo.subresourceRange.baseMipLevel = 0;
	ViewInfo.subresourceRange.levelCount = 1;
	ViewInfo.subresourceRange.baseArrayLayer = 0;
	ViewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(logicalDevice->device, &ViewInfo, nullptr, &texture_image_view) != VK_SUCCESS) {
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
	SamplerInfo.maxAnisotropy = 16;
	SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	SamplerInfo.unnormalizedCoordinates = VK_FALSE;
	SamplerInfo.compareEnable = VK_FALSE;
	SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(logicalDevice->device, &SamplerInfo, nullptr, &texture_sampler) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to create texture sampler!");
		std::exit(-1);
	}
}

void TextureLayout::DeInit() {
	vkDestroySampler(logicalDevice->device, texture_sampler, nullptr);
	vkDestroyImageView(logicalDevice->device, texture_image_view, nullptr);
	vkDestroyImage(logicalDevice->device, texture, nullptr);
	vkFreeMemory(logicalDevice->device, texture_image_memory, nullptr);
};