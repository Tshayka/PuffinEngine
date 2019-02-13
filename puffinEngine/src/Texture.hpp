#pragma once

#include "Device.hpp"

class TextureLayout {
    public:
        VkFormat format;
        VkDeviceSize size;
		VkImage texture;
		VkImageView view;
		VkDeviceMemory memory;
		VkSampler sampler;
		uint32_t texWidth; 
        uint32_t texHeight; 
        uint32_t baseMipLevel; 
        uint32_t mipLevels;
        uint32_t layers; 
        int texChannels;

        std::vector<VkBufferImageCopy> bufferCopyRegions;
        VkCommandPool commandPool;

        VkCommandBuffer BeginSingleTimeCommands();
        void CreateImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageCreateFlags flag);
        void CreateImageView(VkImageAspectFlags aspectFlags, VkImageViewType type);
        void CreateTextureSampler(enum VkSamplerAddressMode mode); 
        void DeInit();
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
        void Init(Device* device, VkFormat format, uint32_t baseMipLevel, uint32_t mipLevels, uint32_t layers);

        void CopyBufferToImage(VkBuffer buffer);
        void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout); 

    private:
        void CreateCommandPool();
        bool HasStencilComponent();
                
        Device* logicalDevice;
    };