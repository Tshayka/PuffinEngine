#pragma once

#include "Buffer.cpp"

class TextureLayout {
    public:
        TextureLayout();
	    ~TextureLayout();
       
        VkCommandBuffer BeginSingleTimeCommands();
        void CopyBufferToImage(VkBuffer buffer);
        void CreateImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageCreateFlags flag);
        void CreateImageView(VkImageAspectFlags aspectFlags, VkImageViewType type);
        void CreateTextureSampler(enum VkSamplerAddressMode mode); 
        void DeInit();
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
        void Init(Device* device, VkCommandPool& commandPool, VkFormat format, uint32_t baseMipLevel, uint32_t mipLevels, uint32_t layers);
        void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout); 

        std::vector<VkBufferImageCopy> bufferCopyRegions;//private?

        VkImage texture;
		VkImageView view;
		VkDeviceMemory memory;
		VkSampler sampler = nullptr;
        VkFormat format;
        VkDeviceSize size;
		uint32_t texWidth; 
        uint32_t texHeight; 
        uint32_t baseMipLevel; 
        uint32_t mipLevels;
        uint32_t layers; 
        int texChannels;

    private:
        bool HasStencilComponent();
        
        VkCommandPool* commandPool = nullptr;
        Device* logicalDevice;
    };