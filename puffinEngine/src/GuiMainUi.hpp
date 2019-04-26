#pragma once

#include <glm/glm.hpp>

#include "../imgui/imgui.h"
#include "ErrorCheck.hpp"
#include "MaterialLibrary.hpp"
#include "Scene.hpp"
#include "Texture.hpp"


class GuiMainUi {
public:
	GuiMainUi();
	~GuiMainUi();

	void CreateUniformBuffer(VkCommandBuffer);
	void DeInit();
	void Init(Device* device, MaterialLibrary* materialLibrary, VkCommandPool& commandPool, VkRenderPass& renderPass);
	void NewFrame();
	void SetUp();
	void UpdateDrawData();
	void UpdateFontSettingsUniformBuffer();
	void UpdateFontUniformBuffer();

	bool splitScreen = true;

	// UI params are set via push constants
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

    struct Vertex {
        glm::vec2 pos;
        glm::vec2 uv;
    };

	struct UiComponent {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        glm::vec4 clipExtent;
		VkPipeline *assignedPipeline = nullptr;
		TextureLayout* assignedTexture = nullptr;
		VkDescriptorSet descriptorSet;
	};

	enum class TextAlignment { left, center, right, justify };

	struct InfoBox {
		glm::vec2 position;
		glm::vec2 dimension;
		glm::vec4 backgroundColor;
		glm::vec2 textSize;
		TextAlignment textAlign = TextAlignment::left;

		GuiMainUi::UiComponent background;
		GuiMainUi::UiComponent text;
		GuiMainUi::UiComponent icon;
	};

    struct DrawData {
		std::vector<UiComponent> componentsToDraw;
		int totalIndicesCount;
		int totalVerticesCount;
    };

	DrawData drawData;

	//bool visible = true;

private:
	void CreateBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateViewAndSampler();
    void GetDrawData();
	void GenerateText(UiComponent& word, std::string text, TextAlignment align, glm::vec2 position, float scale, float maxWidth, float lineHeight, glm::vec2 padding);

	struct UBOF {
		glm::mat4 proj;
		glm::mat4 model;
	} UBOF;

	struct UBOS {
		glm::vec4 outlineColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec2 offset = glm::vec2(1.0f, 1.0f);
		float outlineWidth = 0.6f; // Between 1.0 and 0.0, 1.0 = thick outline, 0.0 = no outline
		float outline = true;
	} UBOS;

	struct {
		enginetool::Buffer<void> fontMatrices;
		enginetool::Buffer<void> fontSettings;
	} uniformBuffers;

	
	enginetool::Buffer<void> vertexBuffer;
	enginetool::Buffer<void> indexBuffer;	
	int32_t vertexCount = 0;
	int32_t indexCount = 0;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline imagePipeline;
	VkPipeline textPipeline;
	VkPipeline backgroundPipeline;
	VkPipelineCache pipelineCache;

	//TextureLayout font;

	VkViewport viewport = {};
	VkRect2D scissor = {};

	Device* logicalDevice = nullptr;
	VkCommandPool* commandPool = nullptr;
	MaterialLibrary* materialLibrary = nullptr;
	VkRenderPass* renderPass = nullptr;	
};