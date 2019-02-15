#include <gli/gli.hpp>
#include <iostream>
#include <stb_image.h>

#include "LoadTexture.cpp"
#include "MaterialLibrary.hpp"
#include "ErrorCheck.hpp"

// ------- Constructors and dectructors ------------- //

MaterialLibrary::MaterialLibrary() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Material library created\n";
#endif
}

MaterialLibrary::~MaterialLibrary() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Material library destroyed\n";
#endif 
}

// ---------------- Main functions ------------------ //

void MaterialLibrary::Init(Device* device) {
	logicalDevice = device;

	CreateCommandPool();
  FillLibrary();  
}

void MaterialLibrary::CreateCommandPool() {
		QueueFamilyIndices queueFamilyIndices = logicalDevice->FindQueueFamilies(logicalDevice->gpu);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow command buffers to be rerecorded individually, optional
	
		ErrorCheck(vkCreateCommandPool(logicalDevice->device, &poolInfo, nullptr, &commandPool));
}

void MaterialLibrary::FillLibrary() {
    enginetool::SceneMaterial defaultGray;
		enginetool::SceneMaterial rust;
		enginetool::SceneMaterial chrome;
		enginetool::SceneMaterial plastic;
		enginetool::SceneMaterial gold; 

    materials = {
			{"default", defaultGray},
			{"gold", gold},
			{"plastic", plastic},
			{"rust", rust},
			{"chrome", chrome}
    };

    for (auto& m : materials) {
		m.second.name = m.first;
        LoadTexture("puffinEngine/assets/textures/" + m.first + "Albedo.jpg", m.second.albedo);
        LoadTexture("puffinEngine/assets/textures/" + m.first + "Metallic.jpg", m.second.metallic); 
        LoadTexture("puffinEngine/assets/textures/" + m.first + "Roughness.jpg", m.second.roughness); 
        LoadTexture("puffinEngine/assets/textures/" + m.first + "Normal.jpg", m.second.normal); 
        LoadTexture("puffinEngine/assets/textures/" + m.first + "Ao.jpg", m.second.ambientOcclucion);     
    }
}

void MaterialLibrary::LoadTexture(std::string texture, TextureLayout& layer) {
	stbi_uc* pixels = stbi_load(texture.c_str(), (int*)&layer.texWidth, (int*)&layer.texHeight, &layer.texChannels, STBI_rgb_alpha);

	if (!pixels) {
		assert(0 && "Vulkan ERROR: failed to load texture image!");
		std::exit(-1);
	}

	VkDeviceSize imageSize = layer.texWidth * layer.texHeight * 4;
	enginetool::Buffer stagingBuffer;
	logicalDevice->CreateStagedBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, pixels);

	stbi_image_free(pixels);
	
	layer.Init(logicalDevice, commandPool, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 1);
	layer.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	layer.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	layer.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);
	layer.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	layer.CopyBufferToImage(stagingBuffer.buffer);
	stagingBuffer.Destroy();
	layer.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void MaterialLibrary::LoadSkyboxTexture(TextureLayout& layer) {
	std::string texture = "puffinEngine/assets/skybox/car_cubemap.ktx";
	gli::texture_cube texCube(gli::load(texture));
	
	layer.Init(logicalDevice, commandPool, VK_FORMAT_R8G8B8A8_UNORM, 0, texCube.levels(), 6);
	layer.texWidth = texCube.extent().x;
	layer.texHeight = texCube.extent().y;
	layer.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
	layer.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE);
	layer.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	layer.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkDeviceSize imageSize = texCube.size();
	enginetool::Buffer stagingBuffer;
	logicalDevice->CreateStagedBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, texCube.data());

	VkCommandBuffer command_buffer = layer.BeginSingleTimeCommands();
	uint32_t offset = 0;

	for (uint32_t face = 0; face < layer.layers; face++) {
		for (uint32_t level = 0; level < layer.mipLevels; level++)	{
			VkBufferImageCopy Region = {};
			Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			Region.imageSubresource.mipLevel = level;
			Region.imageSubresource.baseArrayLayer = face;
			Region.imageSubresource.layerCount = 1;
			Region.imageExtent.width = texCube[face][level].extent().x;
			Region.imageExtent.height = texCube[face][level].extent().y;
			Region.imageExtent.depth = 1;
			Region.bufferOffset = offset;

			layer.bufferCopyRegions.emplace_back(Region);

			offset += static_cast<uint32_t>(texCube[face][level].size());
		}
	}

	vkCmdCopyBufferToImage(command_buffer, stagingBuffer.buffer, layer.texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(layer.bufferCopyRegions.size()), layer.bufferCopyRegions.data());
	layer.EndSingleTimeCommands(command_buffer);
	stagingBuffer.Destroy();

	layer.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void MaterialLibrary::DeInit() {
	for (auto& m : materials) {
		m.second.albedo.DeInit();
		m.second.metallic.DeInit();
		m.second.roughness.DeInit();
		m.second.normal.DeInit();
		m.second.ambientOcclucion.DeInit();
	}

	vkDestroyCommandPool(logicalDevice->device, commandPool, nullptr);
	
	logicalDevice = nullptr;
}