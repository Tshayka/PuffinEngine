#include <array>
#include <iostream>

#include "../headers/RenderPass.hpp"
#include "../headers/Log.hpp"

using namespace puffinengine::tool;

//[[--------------- Constructors and dectructors ---------------]]

RenderPass::RenderPass() {
	m_RenderPass = {};
	m_DepthFormat = VK_FORMAT_UNDEFINED;
#if DEBUG_VERSION
	std::cout << "RenderPass object created\n";
#endif 
}

RenderPass::~RenderPass() {
#if DEBUG_VERSION
	std::cout << "RenderPass object destroyed\n";
#endif 
}

//[[------------------ Setters and getters ---------------------]]

VkRenderPass RenderPass::get() const {
	return m_RenderPass;
}

VkFormat RenderPass::getFormat() const {
	return m_DepthFormat;
}

//[[--------------------- Main functions -----------------------]]

// Specify number of color and depth buffers, how many samples to use for each of them and how their contents should be handled throughout the rendering operations

bool RenderPass::init(Device* device, const RenderPass::Type type) {
	p_Device = device;

	createRenderPass(type);

	m_Initialized = true;

	return true;
}

bool RenderPass::createRenderPass(const RenderPass::Type type) {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = (type == Type::SCREEN) ? p_Device->swapchainImageFormat : VK_FORMAT_R8G8B8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = (type == Type::SCREEN) ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	m_DepthFormat = p_Device->FindDepthFormat();

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = m_DepthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
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

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_references;
	subpass.pDepthStencilAttachment = &depth_attachment_references;

	// specify memory and execution dependencies between subpasses

	std::vector<VkSubpassDependency> dependencies;

	VkSubpassDependency dep0;
	dep0.srcSubpass = VK_SUBPASS_EXTERNAL;
	dep0.dstSubpass = 0;
	dep0.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep0.srcStageMask = (type == Type::SCREEN) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dep0.srcAccessMask = (type == Type::SCREEN) ? 0 : VK_ACCESS_MEMORY_READ_BIT;
	dep0.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies.emplace_back(dep0);

	if (type == Type::OFFSCREEN) {
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkSubpassDependency dep1;
		dep1.srcSubpass = 0;
		dep1.dstSubpass = VK_SUBPASS_EXTERNAL;
		dep1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep1.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dep1.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dep1.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dep1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependencies.emplace_back(dep1);
	}

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = dependencies.data();

	checkResult(vkCreateRenderPass(p_Device->get(), &renderPassInfo, nullptr, &m_RenderPass));

	return true;
}

void RenderPass::deInit() {
	vkDestroyRenderPass(p_Device->get(), m_RenderPass, nullptr);

	p_Device = nullptr;

	m_Initialized = false;
}