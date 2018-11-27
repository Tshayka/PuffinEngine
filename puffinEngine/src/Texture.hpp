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
		uint32_t texWidth, texHeight; 
        int texChannels;

        void CreateImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
        void CreateImageView(VkImageAspectFlags aspect_flags);
        void CreateTextureSampler(enum VkSamplerAddressMode mode); 
        void DeInit();
        void Init(Device* device, VkFormat format);

        private:
        Device* logicalDevice;
    };