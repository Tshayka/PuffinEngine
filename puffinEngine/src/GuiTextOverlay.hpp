#pragma once

#include <glm/gtc/matrix_transform.hpp> 

#include "ErrorCheck.hpp"
#include "Device.hpp"

// Defines for the STB font used
// STB font files can be found at http://nothings.org/stb/font/
#include "../fonts/stb_font_consolas_24_latin1.inl"
#define STB_FONT_NAME stb_font_consolas_24_latin1
#define STB_FONT_WIDTH STB_FONT_consolas_24_latin1_BITMAP_WIDTH
#define STB_FONT_HEIGHT STB_FONT_consolas_24_latin1_BITMAP_HEIGHT 
#define STB_FIRST_CHAR STB_FONT_consolas_24_latin1_FIRST_CHAR
#define STB_NUM_CHARS STB_FONT_consolas_24_latin1_NUM_CHARS

static unsigned char font24pixels[STB_FONT_HEIGHT][STB_FONT_WIDTH];

enum TextAlignment { alignLeft, alignCenter, alignRight };

class GuiTextOverlay {
public:
	GuiTextOverlay();
	~GuiTextOverlay();

	void BeginTextUpdate();
    void CreateUniformBuffer(VkCommandBuffer);
    void DeInit();
	void EndTextUpdate();
	void Init(Device* device);
	void RenderText(std::string, float, float, enum TextAlignment);
	void SetUp();

	std::vector<VkCommandBuffer> command_buffers;

private:
	VkCommandBuffer BeginSingleTimeCommands();
	void CreateCommandPool();
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	VkShaderModule CreateShaderModule(const std::vector<char>&);
	void CreateViewAndSampler();
	void InitResources();
	void EndSingleTimeCommands(VkCommandBuffer);

	glm::vec4 *mapped = nullptr;
	uint32_t num_letters;
	stb_fontchar stbFontData[STB_NUM_CHARS];

	VkBuffer buffer; // mapping the vertex data
	VkDeviceMemory memory;

	uint32_t buffer_index;
	VkCommandPool command_pool;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;
	VkImage image;
	VkDeviceMemory image_memory;
	VkQueue queue;
	VkRenderPass render_pass;
	VkImageView image_view;
	VkSampler texture_sampler;
	VkPipelineLayout pipeline_layout;
	VkPipeline text_overlay_pipeline;
	VkPipelineCache pipeline_cache;
	
	VkRect2D scissor = {};
	VkViewport viewport = {}; 

	Device* logical_device = nullptr;
};