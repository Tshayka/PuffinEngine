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

	void Init(Device* device, VkCommandPool& commandPool);
    void DeInit();
	
	void BeginTextUpdate();
    void CreateUniformBuffer(const VkCommandBuffer& command_buffer);
	void EndTextUpdate();
	void RenderText(std::string, float, float, enum TextAlignment);

private:
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void LoadFontImage();

	glm::vec4 *mapped = nullptr;
	stb_fontchar stbFontData[STB_NUM_CHARS];
	uint32_t num_letters;

	enginetool::Buffer m_VertexBuffer; // mapping the vertex data

	TextureLayout font;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkPipelineCache pipelineCache;
	
	VkRect2D scissor = {};
	VkViewport viewport = {}; 

	Device* logical_device = nullptr;
	VkCommandPool* commandPool = nullptr;
};