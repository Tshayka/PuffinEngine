#pragma once

#include <glm/glm.hpp>

#include "../imgui/imgui.h"
#include "Device.hpp"
#include "ErrorCheck.hpp"

class GuiElement {
public:

	GuiElement();
	~GuiElement();

	void CreateUniformBuffer(VkCommandBuffer);
	void DeInitMenu();
	void InitMenu(Device*);
	void InitResources();
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
	VkCommandBuffer BeginSingleTimeCommands();
	void CreateCommandPool();
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	VkShaderModule CreateVertShaderModule();
	VkShaderModule CreateFragShaderModule();
	void CreateViewAndSampler();
	void EndSingleTimeCommands(VkCommandBuffer);
	
	// Vulkan resources for rendering the UI
	VkCommandPool command_pool;
	VkSampler ui_texture_sampler;
	
	enginetool::Buffer vertex_buffer;
	enginetool::Buffer index_buffer;	
	int32_t vertex_count = 0;
	int32_t index_count = 0;
	VkImage font_image = VK_NULL_HANDLE;
	VkDeviceMemory font_memory = VK_NULL_HANDLE;
	VkImageView ui_image_view = VK_NULL_HANDLE;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet gui_descriptor_set;
	VkPipelineLayout gui_pipeline_layout;
	VkPipeline pipeline;
	VkPipelineCache pipeline_cache;

	Device* logical_device;

	VkViewport viewport = {};
	VkRect2D scissor = {};
};