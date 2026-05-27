#include <array>

#include "../headers/RenderPass.hpp"

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

VkRenderPass RenderPass::getOffscreen() const {
	return m_OffscreenRenderPass;
}

//[[--------------------- Main functions -----------------------]]

// Specify number of color and depth buffers, how many samples to use for each of them and how their contents should be handled throughout the rendering operations

bool RenderPass::init(Device* device) {
	p_Device = device;

	m_Initialized = true;

	return true;
}

bool RenderPass::createRenderPass() {

}

VkFormat RenderPass::findSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling& tiling, const VkFormatFeatureFlags& features) {

}

void RenderPass::deInit() {
	vkDestroyRenderPass(p_Device->getDevice(), m_RenderPass, nullptr);

	p_Device = nullptr;

	m_Initialized = false;
}