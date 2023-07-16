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

	void Init(Device* device, GuiElement* console, GuiTextOverlay* textOverlay, GuiMainUi* mainUi, WorldClock* mainClock, enginetool::ThreadPool& threadPool);
	void cleanUpForSwapchain();
	void recreateForSwapchain();
	void DeInit();
	
	void Submit(VkQueue, uint32_t);
	void UpdateGui(); 
	

	std::vector<VkCommandBuffer> command_buffers;
	bool guiOverlayVisible = true;

private:
    void CreateCommandPool();
	void CreateRenderPass();
	void updateCommandBuffers(const double &frameTime, uint32_t elapsedTime, const double& fps);

	void freeCommandBuffers();

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