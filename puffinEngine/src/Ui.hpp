#pragma once

#include <glm/glm.hpp>

#include "../imgui/imgui.h"
#include "Buffer.hpp"
#include "ErrorCheck.hpp"
#include "Texture.hpp"

class GuiElement {
public:

	GuiElement();
	~GuiElement();

	void CreateUniformBuffer(const VkCommandBuffer& command_buffer);
	void DeInit();
	void Init(Device* logiclDevice, VkCommandPool& commandPool);
	void LoadImage();
	void NewFrame();
	void RenderDrawData();
	void SetUp();

	// UI params are set via push constants
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

	//bool visible = true;

private:
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	VkShaderModule CreateVertShaderModule();
	VkShaderModule CreateFragShaderModule();
	void CreateViewAndSampler();
	
	enginetool::Buffer vertexBuffer;
	enginetool::Buffer indexBuffer;	
	int32_t vertexCount = 0;
	int32_t indexCount = 0;
	TextureLayout font;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkPipelineCache pipelineCache;

	VkViewport viewport = {};
	VkRect2D scissor = {};

	Device* logicalDevice = nullptr;
	VkCommandPool* commandPool = nullptr; 
};