#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "LoadMesh.cpp"
#include "StatusOverlay.hpp"


#define TEXTOVERLAY_MAX_CHAR_COUNT 2048

//---------- Constructors and dectructors ---------- //

StatusOverlay::StatusOverlay() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui overlay created\n";
#endif 
}

StatusOverlay::~StatusOverlay() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui overlay destroyed\n";
#endif
}

// --------------- Setters and getters -------------- //



// ---------------- Main functions ------------------ //

void StatusOverlay::Init(Device* device, GuiElement* console, GuiTextOverlay* textOverlay)
{
	this->console = console;
	this->textOverlay = textOverlay;
	logical_device = device; 
	
	CreateRenderPass();
	CreateCommandPool();

}

void StatusOverlay::Submit(VkQueue queue, uint32_t bufferindex) {
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

void StatusOverlay::CreateRenderPass() {
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = logical_device->swapchain_image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Don't clear the framebuffer!
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depth_attachment = {};
	depth_attachment.format = logical_device->depthFormat;
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

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.pNext = nullptr;
	render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	render_pass_info.pAttachments = attachments.data();
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &SubpassDescription;
	render_pass_info.dependencyCount = 2;
	render_pass_info.pDependencies = SubpassDependencies;

	ErrorCheck(vkCreateRenderPass(logical_device->device, &render_pass_info, nullptr, &render_pass));
}

void StatusOverlay::CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = logical_device->FindQueueFamilies(logical_device->gpu);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow command buffers to be rerecorded individually, optional

	ErrorCheck(vkCreateCommandPool(logical_device->device, &poolInfo, nullptr, &command_pool));
}

void StatusOverlay::UpdateCommandBuffers(float frameTimer, uint32_t elapsedTime) {

	textOverlay->BeginTextUpdate();
	textOverlay->RenderText("Some random title", 5.0f, 5.0f, TextAlignment::alignLeft);
	std::stringstream ss;
	ss << std::fixed << std::setprecision(2) << (1000.0f*frameTimer) << " elapsed time (" << elapsedTime << " fps)";
	textOverlay->RenderText(ss.str(), 5.0f, 25.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press \"1\" to toggle GUI", 5.0f, 65.0f, TextAlignment::alignLeft);
	textOverlay->RenderText("Press WSAD to move camera", 5.0f, 85.0f, TextAlignment::alignLeft);
	textOverlay->EndTextUpdate();

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
	renderPassInfo.renderPass = render_pass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = logical_device->swapchain_extent;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	command_buffers.resize(logical_device->swap_chain_framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = command_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if the allocated command buffers are primary or secondary, here "primary" can be submitted to a queue for execution, but cannot be called from other command buffers
	allocInfo.commandBufferCount = (uint32_t)command_buffers.size();

	ErrorCheck(vkAllocateCommandBuffers(logical_device->device, &allocInfo, command_buffers.data()));
	
	// starting command buffer recording
	for (size_t i = 0; i < command_buffers.size(); i++)
	{
		// Set target frame buffer
		renderPassInfo.framebuffer = logical_device->swap_chain_framebuffers[i];
		vkBeginCommandBuffer(command_buffers[i], &beginInfo);
		vkCmdBeginRenderPass(command_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

void StatusOverlay::DeInit() {
	vkDestroyCommandPool(logical_device->device, command_pool, nullptr);
	vkDestroyRenderPass(logical_device->device, render_pass, nullptr);
}
