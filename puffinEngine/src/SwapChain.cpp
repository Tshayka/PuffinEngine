#include <algorithm>
#include <iostream>

#include "../headers/SwapChain.hpp"
#include "../headers/Log.hpp"

using namespace puffinengine::tool;

//[[--------------- Constructors and dectructors ---------------]]

SwapChain::SwapChain() {
	p_Device = nullptr;
	p_Window = nullptr;
	m_SwapChain = nullptr;
	m_SwapchainImages = {};
	m_SwapchainImageViews = {};
	m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
	m_SwapChainExtent = {0, 0};
	m_ImageIndex = 0;

#if DEBUG_VERSION
	std::cout << "SwapChain object created\n";
#endif 
}

SwapChain::~SwapChain() {
#if DEBUG_VERSION
	std::cout << "SwapChain object destroyed\n";
#endif 
}

//[[------------------ Setters and getters ---------------------]]

VkSwapchainKHR SwapChain::get() const {
	return m_SwapChain;
}

const std::vector<VkImageView>& SwapChain::getSwapchainImageViews() {
	return m_SwapchainImageViews;
}

VkFormat SwapChain::getSwapchainImageFormat() const {
	return m_SwapchainImageFormat;
}

VkExtent2D SwapChain::getExtent() const {
	return m_SwapChainExtent; // TODO rename
}

uint32_t& SwapChain::getImageIndex() {
	return m_ImageIndex;
}

//[[--------------------- Main functions -----------------------]]

bool SwapChain::init(Device* device, GLFWwindow* window) {
	p_Device = device;
	p_Window = window;

	if (!createSwapChain()) {
		return false;
	}

	m_Initialized = true;
	return true;
}

void SwapChain::deInit() {
	vkDestroySwapchainKHR(p_Device->get(), m_SwapChain, nullptr);
	p_Device = nullptr;
	m_Initialized = false;
}


bool SwapChain::createSwapChain() {
	SwapChainSupportDetails swapChainSupport = p_Device->querySwapChainSupport();
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	// number of images in the swap chain, (queue length), specifies the minimum amount of images to function properly

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = p_Device->getSurface();
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.imageArrayLayers = 1; // specifies the amount of layers each image consists of (this is always 1, it's not a stereoscopic 3D application)
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = p_Device->findQueueFamilies();
	uint32_t queueFamilyIndices[] = { static_cast<uint32_t>(indices.graphicsFamily), static_cast<uint32_t>(indices.presentFamily) };

	if (indices.graphicsFamily != indices.presentFamily) {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapChainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE; // means that we don't care about the color of pixels
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	checkResult(vkCreateSwapchainKHR(p_Device->get(), &swapChainCreateInfo, nullptr, &m_SwapChain));

	vkGetSwapchainImagesKHR(p_Device->get(), m_SwapChain, &imageCount, nullptr);
	m_SwapchainImages.resize(imageCount);

	checkResult(vkGetSwapchainImagesKHR(p_Device->get(), m_SwapChain, &imageCount, m_SwapchainImages.data()));

	m_SwapchainImageFormat = surfaceFormat.format;
	m_SwapChainExtent = extent;

	return true;
}

VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	// surface has no preferred format, which Vulkan indicates by only returning one VkSurfaceFormatKHR entry (best case scenario)
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	//  go through the list and see if the preferred combination is available (if previous "if" wasn't free to choose any format)
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {// minimum plan	
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR; // FIFO, IMMEDIATE or MAILBOX

	for (const auto& available_present_mode : availablePresentModes) {
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {// best for games
			return available_present_mode;
		}

		else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			best_mode = available_present_mode;
		}
	}

	return best_mode;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int actualWidth;
	    int actualHeigth;
		glfwGetWindowSize(p_Window, &actualWidth, &actualHeigth);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(actualWidth),
			static_cast<uint32_t>(actualHeigth)
		};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void SwapChain::initSwapchainImageViews() {
	m_SwapchainImageViews.resize(m_SwapchainImages.size());

	for (size_t i = 0; i < m_SwapchainImages.size(); ++i) {
		m_SwapchainImageViews[i] = createImageView(m_SwapchainImages[i], m_SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void SwapChain::deInitSwapchainImageViews() {
	for (auto imageView : m_SwapchainImageViews) {
		vkDestroyImageView(p_Device->get(), imageView, nullptr);
	}
}

VkImageView SwapChain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags) {//TODO use in swapchain textureLayout class
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspect_flags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(p_Device->get(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to create texture image view!");
		std::exit(-1);
	}

	return imageView;
}

VkResult SwapChain::acquireNextImage(VkSemaphore& imageAvailableSemaphore) {
	return vkAcquireNextImageKHR(p_Device->get(), m_SwapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, nullptr, &m_ImageIndex);
}