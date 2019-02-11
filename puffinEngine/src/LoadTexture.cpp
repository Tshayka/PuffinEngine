#include <string>
#include "Texture.hpp"

namespace enginetool {
	class SceneMaterial {
	public:
		// SceneMaterial(Device* device) {
		// 	logicalDevice = device;
		// };

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

	// private:
	// 	Device* logicalDevice;
	};
}