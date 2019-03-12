#pragma once

#include <glm/glm.hpp>

#include "../imgui/imgui.h"
#include "ErrorCheck.hpp"
#include "Texture.hpp"

class GuiMainUi {
public:

	GuiMainUi();
	~GuiMainUi();

	void CreateUniformBuffer(VkCommandBuffer);
	void DeInit();
	void Init(Device* device, VkCommandPool& commandPool);
	void LoadImage();
	void NewFrame();
	void SetUp();
	void UpdateDrawData();

	// AngelCode .fnt format structs and classes
	struct bmchar {
		uint32_t x, y;
		uint32_t width;
		uint32_t height;
		int32_t xoffset;
		int32_t yoffset;
		int32_t xadvance;
		uint32_t page;
	};

	// UI params are set via push constants
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

    struct Vertex {
        glm::vec2 pos;
        glm::vec2 uv;
        glm::vec4 col;
    };

	struct UiComponent {
		glm::vec2 position;
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        glm::vec4 clipExtent;
	};

    struct DrawData {
		std::vector<UiComponent> componentsToDraw;
		int totalIndicesCount;
		int totalVerticesCount;
    };

	DrawData drawData;
 

	//bool visible = true;

private:
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateViewAndSampler();
    void GetDrawData();
	
	enginetool::Buffer<void> vertexBuffer;
	enginetool::Buffer<void> indexBuffer;	
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