#pragma once

#include "../imgui/imgui.h"
#include "Buffer.hpp"
#include "ErrorCheck.hpp"
#include "PushConstant.hpp"
#include "Texture.hpp"
#include "RenderPass.hpp"

class GuiElement {
public:
	GuiElement();
	~GuiElement();

	void init(Device* logiclDevice, puffinengine::tool::RenderPass* renderPass, VkCommandPool* commandPool);
	void cleanUpForSwapchain();
	void recreateForSwapchain();
	void deInit();
	
	void createUniformBuffer(const VkCommandBuffer& command_buffer);
	void newFrame();
	void setUp();
	void updateDrawData();

	PushConstBlock m_PushConstBlock;

private:
	void createDescriptorPool();
	void createDescriptorSet();
	void createDescriptorSetLayout();

	VkShaderModule createVertShaderModule();
	VkShaderModule createFragShaderModule();
	
	void loadFontImage();
	void createGraphicsPipeline();
	void destroyPipeline();
	
	int32_t m_VertexCount = 0;
	int32_t m_IndexCount = 0;
	TextureLayout m_Font;

	VkDescriptorPool m_DescriptorPool;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorSet m_DescriptorSet;
	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_Pipeline;
	VkPipelineCache m_PipelineCache;

	enginetool::Buffer m_VertexBuffer;
	enginetool::Buffer m_IndexBuffer;

	VkViewport m_Viewport = {};
	VkRect2D m_Scissor = {};

	Device* p_LogicalDevice = nullptr;
	puffinengine::tool::RenderPass* p_RenderPass = nullptr;
	VkCommandPool* p_CommandPool = nullptr; 
};