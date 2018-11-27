#include <string>
#include "Texture.hpp"

namespace enginetool {
	class SceneMaterial {
	public:
		SceneMaterial(Device* device) {
			logicalDevice = device;
		};

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

	private:
		Device* logicalDevice;
	};
}