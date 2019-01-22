#pragma once

#include "Device.hpp"

class TextureLayout {
        public:
        VkFormat format;
        VkDeviceSize image_size;
		VkImage texture;
		VkImageView texture_image_view;
		VkDeviceMemory texture_image_memory;
		VkSampler texture_sampler;
		uint32_t texWidth; 
        uint32_t texHeight; 
        uint32_t baseMipLevel; 
        uint32_t mipLevels;
        uint32_t layers; 
        int texChannels;

        std::vector<VkBufferImageCopy> bufferCopyRegions;

        void CreateImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageCreateFlags flag);
        void CreateImageView(VkImageAspectFlags aspect_flags, VkImageViewType type);
        void CreateTextureSampler(enum VkSamplerAddressMode mode); 
        void DeInit();
        void Init(Device* device, VkFormat format);

        void CopyBufferToImage(VkBuffer buffer);
        void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout); 

        //private:
        VkCommandBuffer BeginSingleTimeCommands();
        void CreateCommandPool();
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
        bool HasStencilComponent();
        

        VkCommandPool commandPool;
        Device* logicalDevice;
    };