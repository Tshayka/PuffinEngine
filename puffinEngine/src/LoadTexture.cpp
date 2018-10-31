#include <string>
#include <vulkan/vulkan.h>

namespace enginetool {
	struct TextureLayout {
		VkImage texture;
		VkImageView texture_image_view;
		VkDeviceMemory texture_image_memory;
		VkSampler texture_sampler;
		int texture_width, texture_height, texture_channels;
	};

	struct SceneMaterial {
		std::string name;
		TextureLayout skybox_texture;
		TextureLayout irradiance_map;
		TextureLayout albedo;
		TextureLayout metallic;
		TextureLayout roughness;
		TextureLayout normal_map;
		TextureLayout ambient_occlucion_map;
		VkDescriptorSet descriptor_set;
		VkPipeline *assigned_pipeline;
	};
}