#include <string>
#include "headers/Texture.hpp"

namespace enginetool {
	struct SceneMaterial {
		std::string name;
		TextureLayout skybox;
		TextureLayout irradiance;
		TextureLayout albedo;
		TextureLayout metallic;
		TextureLayout roughness;
		TextureLayout normal;
		TextureLayout ambientOcclucion;
		VkDescriptorSet descriptorSet;
		VkDescriptorSet reflectDescriptorSet;
		VkDescriptorSet refractDescriptorSet;
		VkPipeline *assignedPipeline;
	};
}