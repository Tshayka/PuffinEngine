#pragma once

#include "GuiMainUi.hpp"
#include "GuiTextOverlay.hpp"
#include "Ui.hpp"
#include "Device.hpp"
#include "RenderPass.hpp"

struct GUISettings {
	bool display_dynamic_ub = true;
	bool display_scene_models = true;
	bool display_main_ui = true;
	bool display_stats_overlay = true;
	bool display_imgui = true;
};

class GuiMainHub {
public:
	GuiMainHub();
	~GuiMainHub();

	void init(Device* device, puffinengine::tool::RenderPass* renderPass, GuiElement* console, GuiTextOverlay* textOverlay, GuiMainUi* mainUi, puffinengine::tool::WorldClock* mainClock, enginetool::ThreadPool* threadPool);
	void cleanUpForSwapchain();
	void recreateForSwapchain();
	void deinit();
	
	void submit(const VkQueue& queue, const int32_t& bufferIndex);
	void updateGui(); 
	
	bool m_Initialized = false;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	bool guiOverlayVisible = true;

	GUISettings m_GUISettings;

private:
    void createCommandPool();
	void createRenderPass();
	void updateCommandBuffers(const double &frameTime, uint32_t elapsedTime, const double& fps);

	void freeCommandBuffers();

	uint32_t m_BufferIndex;
	VkRenderPass m_RenderPass;
	VkCommandPool m_CommandPool;

	VkRect2D m_Scissor = {};
	VkViewport m_Viewport = {}; 

	GuiMainUi* p_MainUi = nullptr;
	GuiElement* p_Console = nullptr;
	GuiTextOverlay* p_TextOverlay = nullptr;
	Device* p_LogicalDevice = nullptr;
	puffinengine::tool::RenderPass* p_RenderPass = nullptr;
	enginetool::ThreadPool* p_ThreadPool = nullptr;
	puffinengine::tool::WorldClock* p_MainClock = nullptr;
};