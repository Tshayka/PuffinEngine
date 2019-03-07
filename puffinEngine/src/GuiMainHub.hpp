#pragma once

#include "GuiMainUi.hpp"
#include "GuiTextOverlay.hpp"
#include "Ui.hpp"
#include "Device.hpp"

class GuiMainHub 
{
public:
	GuiMainHub();
	~GuiMainHub();

	struct GUISettings {
		bool display_dynamic_ub = true;
		bool display_scene_models = true;
		bool display_main_ui = true;
		bool display_stats_overlay = true;
		bool display_imgui = true;
	} ui_settings;

	bool guiOverlayVisible = true;
	void CleanUpForSwapchain();
	void DeInit();
	void Init(Device* device, GuiElement* console, GuiTextOverlay* textOverlay, GuiMainUi* mainUi, WorldClock* mainClock, enginetool::ThreadPool& threadPool);
	void RecreateForSwapchain();
	void Submit(VkQueue, uint32_t);
	void UpdateGui(); 
	
	std::vector<VkCommandBuffer> command_buffers;

private:
    void CreateCommandPool();
	void CreateRenderPass();
	void UpdateCommandBuffers(float frameTimer, uint32_t elapsedTime);

	uint32_t bufferIndex;
	VkRenderPass renderPass;
	VkCommandPool commandPool;

	VkRect2D scissor = {};
	VkViewport viewport = {}; 

	GuiMainUi* mainUi = nullptr;
	GuiElement* console = nullptr;
	GuiTextOverlay* textOverlay = nullptr;
	Device* logicalDevice = nullptr;
	enginetool::ThreadPool* threadPool = nullptr;
	WorldClock* mainClock = nullptr;	
};