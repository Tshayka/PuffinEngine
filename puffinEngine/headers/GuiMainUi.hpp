#pragma once

#include "Buffer.hpp"
#include "ErrorCheck.hpp"
#include "PushConstant.hpp"
#include "RenderPass.hpp"
#include "SwapChain.hpp"
#include "Texture.hpp"

#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#endif

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

struct Vertex {
	glm::vec2 pos;
	glm::vec2 uv;
	glm::vec4 col;
};

/// <summary>
/// Calculates points positions for a circle
/// </summary>
class CircleGenerator {
public:
	CircleGenerator(const float radius, const uint32_t quality) : m_Radius{radius}, m_Quality{quality}, m_DA{(2.0f * pi) / static_cast<float>(quality)} {}

	static float constexpr pi{ 3.1415927f };
	float m_Radius = 0.0f;
	//The number of points to use
	uint32_t const m_Quality = 0;
	// The angle variation beetween two consecutive ponts
	float const m_DA = 0.0f;

	glm::vec2 getPoint(const uint32_t i) const {
		float const angle{ m_DA * static_cast<float>(i) };
		return glm::vec2{ m_Radius * glm::vec2{cos(angle), sin(angle) } };
	}
};


struct UiComponent {
	glm::vec2 position;
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	glm::vec4 clipExtent;
};

struct DrawData {
	std::vector<UiComponent> componentsToDraw;
	int totalIndicesCount = 0;
	int totalVerticesCount = 0;
};

class GuiMainUi {
public:
	GuiMainUi();
	~GuiMainUi();

	void init(Device* device, puffinengine::tool::SwapChain* swapChain, puffinengine::tool::RenderPass* renderPass, VkCommandPool* commandPool);
	void cleanUpForSwapchain();
	void recreateForSwapchain();
	void deInit();

	void createUniformBuffer(const VkCommandBuffer& commandBuffer);
	void newFrame();
	void setUp();
	void updateDrawData();
	
	PushConstBlock m_PushConstBlock;
	DrawData m_DrawData;
 
private:
	void createDescriptorPool();
	void createDescriptorSet();
	void createDescriptorSetLayout();

	void loadImGuiImage();
    void getDrawData();

	void createGraphicsPipeline();
	void destroyPipeline();

	void generateCircle(std::vector<Vertex>& vertices, const glm::vec2& pos, const float radius, const uint32_t quality);

	
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
	puffinengine::tool::SwapChain* p_SwapChain = nullptr;
	puffinengine::tool::RenderPass* p_RenderPass = nullptr;
	VkCommandPool* p_CommandPool = nullptr;
};