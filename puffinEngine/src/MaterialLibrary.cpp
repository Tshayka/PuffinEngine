#include <gli/gli.hpp>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>

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

	std::filesystem::path p = std::filesystem::current_path().parent_path();

	for (auto& m : materials) {
		std::filesystem::path albedoFileName(m.first + "Albedo.jpg");
		std::filesystem::path metallicFileName(m.first + "Metallic.jpg");
		std::filesystem::path roughnessFileName(m.first + "Roughness.jpg");
		std::filesystem::path normalFileName(m.first + "Normal.jpg");
		std::filesystem::path ambientOcclucionFileName(m.first + "Ao.jpg");

		std::filesystem::path albedoFullPath = p / "puffinEngine" / "assets" / "textures" / albedoFileName;
		LoadTexture(albedoFullPath.string(), m.second.albedo);

		std::filesystem::path metallicFullPath = p / "puffinEngine" / "assets" / "textures" / metallicFileName;
		LoadTexture(metallicFullPath.string(), m.second.metallic);

		std::filesystem::path roughnessFullPath = p / "puffinEngine" / "assets" / "textures" / roughnessFileName;
		LoadTexture(roughnessFullPath.string(), m.second.roughness);

		std::filesystem::path normalFullPath = p / "puffinEngine" / "assets" / "textures" / normalFileName;
		LoadTexture(normalFullPath.string(), m.second.normal);

		std::filesystem::path ambientOcclucionFullPath = p / "puffinEngine" / "assets" / "textures" / ambientOcclucionFileName;
		LoadTexture(ambientOcclucionFullPath.string(), m.second.ambientOcclucion);
	}


	enginetool::SceneMaterial character;
	std::filesystem::path characterAlbedo = p / "puffinEngine" / "assets" / "textures" / "icons" / "characterIcon.jpg";
	LoadTexture(characterAlbedo.string(),  character.albedo);

	std::filesystem::path characterMetallic = p / "puffinEngine" / "assets" / "textures" / "defaultMetallic.jpg";
	LoadTexture(characterMetallic.string(), character.metallic);

	std::filesystem::path characterRoughness = p / "puffinEngine" / "assets" / "textures" / "defaultRoughness.jpg";
	LoadTexture(characterRoughness.string(), character.roughness);

	std::filesystem::path characterNormal = p / "puffinEngine" / "assets" / "textures" / "defaultNormal.jpg";
	LoadTexture(characterNormal.string(), character.normal);

	std::filesystem::path characterAo = p / "puffinEngine" / "assets" / "textures" / "defaultAo.jpg";
	LoadTexture(characterAo.string(), character.ambientOcclucion);

	materials.insert(std::make_pair("character", character));   
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
	std::filesystem::path p = std::filesystem::current_path().parent_path();
	std::filesystem::path texture = p / "puffinEngine" / "assets" / "skybox" / "car_cubemap.ktx";
	gli::texture_cube texCube(gli::load(texture.string()));
	
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