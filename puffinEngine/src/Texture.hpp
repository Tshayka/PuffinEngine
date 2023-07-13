#pragma once

#include "Device.hpp"

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

        VkImage m_FontImage;
		VkImageView view;
		VkDeviceMemory m_FontMemory;
		VkSampler sampler = nullptr;
        VkFormat format;
        VkDeviceSize size;
		int texWidth; 
        int texHeight;
        uint32_t baseMipLevel; 
        uint32_t mipLevels;
        uint32_t layers; 
        int texChannels;

    private:
        bool HasStencilComponent();
        
        VkCommandPool* commandPool = nullptr;
        Device* logicalDevice;
    };