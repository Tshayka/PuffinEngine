#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "MeshLayout.cpp"
#include "headers/GuiMainHub.hpp"

using namespace puffinengine::tool;

#define TEXTOVERLAY_MAX_CHAR_COUNT 2048

//---------- Constructors and dectructors ---------- //

GuiMainHub::GuiMainHub() {
#if DEBUG_VERSION
	std::cout << "Gui - main hub - created\n";
#endif 
}

GuiMainHub::~GuiMainHub() {
#if DEBUG_VERSION
	std::cout << "Gui - main hub - destroyed\n";
#endif
}

// --------------- Setters and getters -------------- //



// ---------------- Main functions ------------------ //

void GuiMainHub::init(Device* device, SwapChain* swapChain, RenderPass* renderPass, GuiElement* console, GuiTextOverlay* textOverlay, GuiMainUi* mainUi, puffinengine::tool::WorldClock* mainClock, enginetool::ThreadPool* threadPool) {
	p_Device = device;
	p_SwapChain = swapChain;
	p_RenderPass = renderPass;
	p_MainUi = mainUi;
	p_Console = console;
	p_TextOverlay = textOverlay;
	p_ThreadPool = threadPool;
	p_MainClock = mainClock;
		
	createRenderPass();
	createCommandPool();

	p_Console->init(p_Device, p_SwapChain, p_RenderPass, &m_CommandPool);
	p_TextOverlay->init(p_Device, p_SwapChain, p_RenderPass, &m_CommandPool);
	p_MainUi->init(p_Device, p_SwapChain, p_RenderPass, &m_CommandPool);

	m_Initialized = true;
}

void GuiMainHub::updateGui() {
	updateCommandBuffers(p_MainClock->frameTime, (uint32_t)p_MainClock->totalElapsedTime, p_MainClock->fps);
}

void GuiMainHub::cleanUpForSwapchain() {
	freeCommandBuffers();
	p_Console->cleanUpForSwapchain();
	p_TextOverlay->cleanUpForSwapchain();
	p_MainUi->cleanUpForSwapchain();
}

void GuiMainHub::recreateForSwapchain() {
	p_Console->recreateForSwapchain();
	p_TextOverlay->recreateForSwapchain();
	p_MainUi->recreateForSwapchain();
}

void GuiMainHub::submit(const VkQueue& queue, const int32_t &bufferIndex) {
	if (!guiOverlayVisible) {
		return;
	}
	
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pCommandBuffers = &m_CommandBuffers[bufferIndex];
	submitInfo.commandBufferCount = 1;
	
	ErrorCheck(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	ErrorCheck(vkQueueWaitIdle(queue));
}

void GuiMainHub::createRenderPass() {
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = p_SwapChain->getSwapchainImageFormat();
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Don't clear the framebuffer!
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depth_attachment = {};
	depth_attachment.format = p_RenderPass->getFormat();
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

	ErrorCheck(vkCreateRenderPass(p_Device->get(), &renderPass_info, nullptr, &m_RenderPass));
}

void GuiMainHub::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = p_Device->findQueueFamilies();

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow command buffers to be rerecorded individually, optional

	ErrorCheck(vkCreateCommandPool(p_Device->get(), &poolInfo, nullptr, &m_CommandPool));
}

void GuiMainHub::updateCommandBuffers(const double &frameTime, uint32_t elapsedTime, const double &fps) {
	p_TextOverlay->beginTextUpdate();
	p_TextOverlay->renderText("Some random title", 5.0f, 5.0f, TextAlignment::alignLeft);
	std::stringstream ss;
	ss << std::fixed << std::setprecision(4) << "Frame Time : " << (frameTime) << "ms | " << " elapsed time : " << elapsedTime << "s | " << fps << " FPS)";
	p_TextOverlay->renderText(ss.str(), 5.0f, 25.0f, TextAlignment::alignLeft);
	p_TextOverlay->renderText("Press \"1\" to turn on or off all GUI components", 5.0f, 65.0f, TextAlignment::alignLeft);
	p_TextOverlay->renderText("Press \"WSAD\" to move camera", 5.0f, 85.0f, TextAlignment::alignLeft);
	p_TextOverlay->renderText("Press \"2-4\" to toggle GUI components", 5.0f, 105.0f, TextAlignment::alignLeft);
	p_TextOverlay->renderText("Press \"V\" to toggle wireframe mode", 5.0f, 125.0f, TextAlignment::alignLeft);
	p_TextOverlay->renderText("Press \"B\" to toggle AABB boxes", 5.0f, 145.0f, TextAlignment::alignLeft);
	p_TextOverlay->renderText("Press \"R\" to reset camera position", 5.0f, 165.0f, TextAlignment::alignLeft);
	p_TextOverlay->renderText("Press \"T\" to reset selected actor position", 5.0f, 185.0f, TextAlignment::alignLeft);
	p_TextOverlay->endTextUpdate();

	p_MainUi->newFrame();
	p_MainUi->updateDrawData();
	p_Console->newFrame();
	p_Console->updateDrawData();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_RenderPass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = p_SwapChain->getExtent();
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	m_CommandBuffers.resize(p_Device->m_SwapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if the allocated command buffers are primary or secondary, here "primary" can be submitted to a queue for execution, but cannot be called from other command buffers
	allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

	ErrorCheck(vkAllocateCommandBuffers(p_Device->get(), &allocInfo, m_CommandBuffers.data()));
	
	// starting command buffer recording
	for (size_t i = 0; i < m_CommandBuffers.size(); i++) {
		// Set target frame buffer
		renderPassInfo.framebuffer = p_Device->m_SwapChainFramebuffers[i];
		vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo);
		vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		if (m_GUISettings.display_main_ui) {
			p_MainUi->createUniformBuffer(m_CommandBuffers[i]);
		}

		if (m_GUISettings.display_stats_overlay) {
			p_TextOverlay->createUniformBuffer(m_CommandBuffers[i]);
		}

		if (m_GUISettings.display_imgui) {
			p_Console->createUniformBuffer(m_CommandBuffers[i]);
		};

		vkCmdEndRenderPass(m_CommandBuffers[i]);
		ErrorCheck(vkEndCommandBuffer(m_CommandBuffers[i]));
	}
}

void GuiMainHub::freeCommandBuffers() {
	vkFreeCommandBuffers(p_Device->get(), m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
}

void GuiMainHub::deinit() {
	vkDestroyCommandPool(p_Device->get(), m_CommandPool, nullptr);
	vkDestroyRenderPass(p_Device->get(), m_RenderPass, nullptr);
}
