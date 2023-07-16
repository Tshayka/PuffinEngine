#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "MeshLayout.cpp"
#include "GuiMainHub.hpp"


#define TEXTOVERLAY_MAX_CHAR_COUNT 2048

//---------- Constructors and dectructors ---------- //

GuiMainHub::GuiMainHub() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui - main hub - created\n";
#endif 
}

GuiMainHub::~GuiMainHub() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui - main hub - destroyed\n";
#endif
}

// --------------- Setters and getters -------------- //



// ---------------- Main functions ------------------ //

void GuiMainHub::Init(Device* device, GuiElement* console, GuiTextOverlay* textOverlay, GuiMainUi* mainUi, WorldClock* mainClock, enginetool::ThreadPool& threadPool) {
	logicalDevice = device; 
	this->mainUi = mainUi;
	this->console = console;
	this->textOverlay = textOverlay;
	this->threadPool = &threadPool;
	this->mainClock = mainClock;
		
	CreateRenderPass();
	CreateCommandPool();

	console->Init(logicalDevice, commandPool);
	textOverlay->init(logicalDevice, commandPool);
	mainUi->Init(logicalDevice, commandPool);
}

void GuiMainHub::UpdateGui() {
	updateCommandBuffers(mainClock->frameTime, (uint32_t)mainClock->totalElapsedTime, mainClock->fps);
}

void GuiMainHub::CleanUpForSwapchain() {}

void GuiMainHub::RecreateForSwapchain() {}

void GuiMainHub::Submit(VkQueue queue, uint32_t bufferindex) {
	if (!guiOverlayVisible) {
		return;
	}
	
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pCommandBuffers = &command_buffers[bufferindex];
	submitInfo.commandBufferCount = 1;
	
	ErrorCheck(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	ErrorCheck(vkQueueWaitIdle(queue));
}

void GuiMainHub::CreateRenderPass() {
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = logicalDevice->swapchainImageFormat;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Don't clear the framebuffer!
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depth_attachment = {};
	depth_attachment.format = logicalDevice->depthFormat;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_references = {};
	color_attachment_references.attachment = 0;
	color_attachment_references.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_references = {};
	depth_attachment_references.attachment = 1;
	depth_attachment_references.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// specify memory and execution dependencies between subpasses
	VkSubpassDependency SubpassDependencies[2] = {};
	
	SubpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	SubpassDependencies[0].dstSubpass = 0;
	SubpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	SubpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	SubpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	SubpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	SubpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	SubpassDependencies[1].srcSubpass = 0;
	SubpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	SubpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	SubpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	SubpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	SubpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	SubpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	
	VkSubpassDescription SubpassDescription = {};
	SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	SubpassDescription.flags = 0;
	SubpassDescription.inputAttachmentCount = 0;
	SubpassDescription.pInputAttachments = nullptr;
	SubpassDescription.colorAttachmentCount = 1;
	SubpassDescription.pColorAttachments = &color_attachment_references;
	SubpassDescription.pResolveAttachments = nullptr;
	SubpassDescription.pDepthStencilAttachment = &depth_attachment_references;
	SubpassDescription.preserveAttachmentCount = 0;
	SubpassDescription.pPreserveAttachments = nullptr;
		
	std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

	VkRenderPassCreateInfo renderPass_info = {};
	renderPass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPass_info.pNext = nullptr;
	renderPass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPass_info.pAttachments = attachments.data();
	renderPass_info.subpassCount = 1;
	renderPass_info.pSubpasses = &SubpassDescription;
	renderPass_info.dependencyCount = 2;
	renderPass_info.pDependencies = SubpassDependencies;

	ErrorCheck(vkCreateRenderPass(logicalDevice->get(), &renderPass_info, nullptr, &renderPass));
}

void GuiMainHub::CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = logicalDevice->FindQueueFamilies(logicalDevice->gpu);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow command buffers to be rerecorded individually, optional

	ErrorCheck(vkCreateCommandPool(logicalDevice->get(), &poolInfo, nullptr, &commandPool));
}

void GuiMainHub::updateCommandBuffers(const double &frameTime, uint32_t elapsedTime, const double &fps) {
	textOverlay->beginTextUpdate();
	textOverlay->renderText("Some random title", 5.0f, 5.0f, TextAlignment::alignLeft);
	std::stringstream ss;
	ss << std::fixed << std::setprecision(4) << "Frame Time : " << (frameTime) << "ms | " << " elapsed time : " << elapsedTime << "s | " << fps << " FPS)";
	textOverlay->renderText(ss.str(), 5.0f, 25.0f, TextAlignment::alignLeft);
	textOverlay->renderText("Press \"1\" to turn on or off all GUI components", 5.0f, 65.0f, TextAlignment::alignLeft);
	textOverlay->renderText("Press \"WSAD\" to move camera", 5.0f, 85.0f, TextAlignment::alignLeft);
	textOverlay->renderText("Press \"2-4\" to toggle GUI components", 5.0f, 105.0f, TextAlignment::alignLeft);
	textOverlay->renderText("Press \"V\" to toggle wireframe mode", 5.0f, 125.0f, TextAlignment::alignLeft);
	textOverlay->renderText("Press \"B\" to toggle AABB boxes", 5.0f, 145.0f, TextAlignment::alignLeft);
	textOverlay->renderText("Press \"R\" to reset camera position", 5.0f, 165.0f, TextAlignment::alignLeft);
	textOverlay->renderText("Press \"T\" to reset selected actor position", 5.0f, 185.0f, TextAlignment::alignLeft);
	textOverlay->endTextUpdate();

	mainUi->NewFrame();
	mainUi->UpdateDrawData();
	console->NewFrame();
	console->RenderDrawData();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = logicalDevice->swapchain_extent;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	command_buffers.resize(logicalDevice->swap_chain_framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if the allocated command buffers are primary or secondary, here "primary" can be submitted to a queue for execution, but cannot be called from other command buffers
	allocInfo.commandBufferCount = (uint32_t)command_buffers.size();

	ErrorCheck(vkAllocateCommandBuffers(logicalDevice->get(), &allocInfo, command_buffers.data()));
	
	// starting command buffer recording
	for (size_t i = 0; i < command_buffers.size(); i++) {
		// Set target frame buffer
		renderPassInfo.framebuffer = logicalDevice->swap_chain_framebuffers[i];
		vkBeginCommandBuffer(command_buffers[i], &beginInfo);
		vkCmdBeginRenderPass(command_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		if (ui_settings.display_main_ui) {
			mainUi->CreateUniformBuffer(command_buffers[i]);
		}

		if (ui_settings.display_stats_overlay) {
			textOverlay->createUniformBuffer(command_buffers[i]);
		}

		if (ui_settings.display_imgui) {
			console->CreateUniformBuffer(command_buffers[i]);
		};

		vkCmdEndRenderPass(command_buffers[i]);
		ErrorCheck(vkEndCommandBuffer(command_buffers[i]));
	}
}

void GuiMainHub::DeInit() {
	vkDestroyCommandPool(logicalDevice->get(), commandPool, nullptr);
	vkDestroyRenderPass(logicalDevice->get(), renderPass, nullptr);
}
