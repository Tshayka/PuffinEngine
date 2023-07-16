#pragma once

#include <glm/gtc/matrix_transform.hpp> 

#include "ErrorCheck.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"

// Defines for the STB font used
// STB font files can be found at http://nothings.org/stb/font/
#include "../fonts/stb_font_consolas_24_latin1.inl"
#define STB_FONT_NAME stb_font_consolas_24_latin1
#define STB_FONT_WIDTH STB_FONT_consolas_24_latin1_BITMAP_WIDTH
#define STB_FONT_HEIGHT STB_FONT_consolas_24_latin1_BITMAP_HEIGHT 
#define STB_FIRST_CHAR STB_FONT_consolas_24_latin1_FIRST_CHAR
#define STB_NUM_CHARS STB_FONT_consolas_24_latin1_NUM_CHARS

enum TextAlignment { alignLeft, alignCenter, alignRight };

class GuiTextOverlay {
public:
	GuiTextOverlay();
	~GuiTextOverlay();

	void init(Device* device, VkCommandPool& commandPool);
	void cleanUpForSwapchain();
	void recreateForSwapchain();
    void deInit();
	
	void createUniformBuffer(const VkCommandBuffer& commandBuffer);
	void beginTextUpdate();
	void renderText(const std::string& text, float x, float y, const TextAlignment& align);
	void endTextUpdate();

private:
	void createDescriptorPool();
	void createDescriptorSet();
	void createDescriptorSetLayout();

	void loadFontImage();

	void createGraphicsPipeline();
	void destroyPipeline();
	
	stb_fontchar m_StbFontData[STB_NUM_CHARS];
	uint32_t m_NumLetters;
	TextureLayout m_Font;

	VkDescriptorPool m_DescriptorPool;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorSet m_DescriptorSet;
	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_Pipeline;
	VkPipelineCache m_PipelineCache;

	VkRect2D m_Scissor = {};
	VkViewport m_Viewport = {};

	glm::vec4* p_Mapped = nullptr;
	enginetool::Buffer m_VertexBuffer;

	Device* p_LogicalDevice = nullptr;
	VkCommandPool* p_CommandPool = nullptr;
};