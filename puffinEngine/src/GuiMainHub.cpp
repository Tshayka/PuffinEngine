#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

//#include "MeshLayout.cpp"
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

void GuiMainHub::Init(Device* device, 
					  GuiElement* console, 
					  GuiTextOverlay* textOverlay, 
					  GuiMainUi* mainUi, 
					  WorldClock* mainClock, 
					  enginetool::ThreadPool& threadPool, 
					  MaterialLibrary* materialLibrary, 
					  Scene* currentScene) {
	
	logicalDevice = device; 
	this->mainUi = mainUi;
	this->console = console;
	this->currentScene = currentScene;
	this->textOverlay = textOverlay;
	this->threadPool = &threadPool;
	this->mainClock = mainClock;
	this->materialLibrary = materialLibrary;
		
	CreateRenderPass();
	CreateCommandPool();

	console->Init(logicalDevice, commandPool, renderPass);
	textOverlay->Init(logicalDevice, commandPool, renderPass);
	mainUi->Init(logicalDevice, materialLibrary, commandPool, renderPass);

	UpdateGui();
}

void GuiMainHub::UpdateGui() {
	UpdateCommandBuffers((float)mainClock->accumulator, (uint32_t)mainClock->totalTime);
}

void GuiMainHub::CleanUpForSwapchain() {}

void GuiMainHub::RecreateForSwapchain() {}

void GuiMainHub::CreateRenderPass() {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = logicalDevice->swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Don't clear the framebuffer!
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = logicalDevice->depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_references = {};
	color_attachment_references.attachment = 0;
	color_attachment_references.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_references = {};
	depth_attachment_references.attachment = 1;
	depth_attachment_references.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// specify memory and execution dependencies between subpasses
	VkSubpassDependency subpassDependencies[2] = {};
	
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	
	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.flags = 0;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &color_attachment_references;
	subpassDescription.pResolveAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = &depth_attachment_references;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;
		
	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = subpassDependencies;

	ErrorCheck(vkCreateRenderPass(logicalDevice->device, &renderPassInfo, nullptr, &renderPass));
}

void GuiMainHub::CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = logicalDevice->FindQueueFamilies(logicalDevice->gpu);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow command buffers to be rerecorded individually, optional

	ErrorCheck(vkCreateCommandPool(logicalDevice->device, &poolInfo, nullptr, &commandPool));
}

void GuiMainHub::UpdateCommandBuffers(float frameTimer, uint32_t elapsedTime) {
	textOverlay->BeginTextUpdate();
	textOverlay->RenderText("Some random title", 5.0f, 5.0f, TextAlignment::alignLeft);
	std::stringstream ss;
	ss << std::fixed << std::setprecision(2) << (1000.0f*frameTimer) << " elapsed time (" << elapsedTime << " fps)";
	textOverlay->RenderText(ss.str(), 5.0f, 25.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"1\" to turn on or off all GUI components", 5.0f, 65.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"WSADQE\" to move camera", 5.0f, 85.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"2-4\" to toggle GUI components", 5.0f, 105.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"V\" to toggle wireframe mode", 5.0f, 125.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"B\" to toggle AABB boxes", 5.0f, 145.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"R\" to reset camera position", 5.0f, 165.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"T\" to test new functionality", 5.0f, 185.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"Z\" to toggle trigger area boxes", 5.0f, 205.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"X\" to toggle selection indicator", 5.0f, 225.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"F\" to pick up stuff with character box", 5.0f, 245.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"IJKLUO\" to move selected item", 5.0f, 265.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"Arrows and space\" to move character", 5.0f, 285.0f, TextAlignment::alignLeft);
	textOverlay->EndTextUpdate();

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

	ErrorCheck(vkAllocateCommandBuffers(logicalDevice->device, &allocInfo, command_buffers.data()));
	
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
			textOverlay->CreateUniformBuffer(command_buffers[i]);
		}

		if (ui_settings.display_imgui) {
			console->CreateUniformBuffer(command_buffers[i]);
		};

		vkCmdEndRenderPass(command_buffers[i]);
		ErrorCheck(vkEndCommandBuffer(command_buffers[i]));
	}
}

void GuiMainHub::DeInit() {
	vkDestroyCommandPool(logicalDevice->device, commandPool, nullptr);
	vkDestroyRenderPass(logicalDevice->device, renderPass, nullptr);

	mainUi = nullptr;
	console = nullptr;
	currentScene = nullptr;
	textOverlay = nullptr;
	logicalDevice = nullptr;
	threadPool = nullptr;
	mainClock = nullptr;
	materialLibrary = nullptr;
}
