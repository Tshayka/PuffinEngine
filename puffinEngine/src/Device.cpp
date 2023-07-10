#include <array>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "Device.hpp"
#include "ErrorCheck.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>


// ------- Constructors and dectructors ------------- //

Device::Device() 
{
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Device object created\n";
#endif 
}

Device::~Device()
{
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Device object destroyed\n";
#endif 
}

// ---------------- Main functions ------------------ //

void Device::InitDevice(GLFWwindow* window)
{
	this->window = window;

	CreateInstance();
    SetupDebugCallback();
    InitDebug();
    CreateSurface();
    PickPhysicalDevice();
	CreateLogicalDevice();
	// all stuff below must go to scene
    InitSwapChain();
	CreateRenderPass();
	CreateOffscreenRenderPass(VK_FORMAT_R8G8B8A8_UNORM);  
}

void Device::DeInitDevice(){
	DestroyOffscreenRenderPass();
	DestroyRenderPass();
    DeInitDebug();
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void Device::CreateSurface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

VkShaderModule Device::CreateShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        ErrorCheck(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

        return shaderModule;
}

// ------- Swapchain and neccesary functions -------- //

void Device::InitSwapChain() {
	SwapChainSupportDetails SwapChainSupport = QuerySwapChainSupport(gpu);

	VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(SwapChainSupport.formats);
	VkPresentModeKHR present_mode = ChooseSwapPresentMode(SwapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(SwapChainSupport.capabilities);

	// number of images in the swap chain, (queue length), specifies the minimum amount of images to function properly

	uint32_t image_count = SwapChainSupport.capabilities.minImageCount + 1;

	if (SwapChainSupport.capabilities.maxImageCount > 0 && image_count > SwapChainSupport.capabilities.maxImageCount) {
		image_count = SwapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR sc_create_info = {};
	sc_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	sc_create_info.surface = surface;

	sc_create_info.minImageCount = image_count;
	sc_create_info.imageFormat = surface_format.format;
	sc_create_info.imageColorSpace = surface_format.colorSpace;
	sc_create_info.imageExtent = extent;
	sc_create_info.imageArrayLayers = 1; // specifies the amount of layers each image consists of (this is always 1, it's not a stereoscopic 3D application)
	sc_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(gpu);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		sc_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		sc_create_info.queueFamilyIndexCount = 2;
		sc_create_info.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		sc_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	sc_create_info.preTransform = SwapChainSupport.capabilities.currentTransform;
	sc_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	sc_create_info.presentMode = present_mode;
	sc_create_info.clipped = VK_TRUE; // means that we don't care about the color of pixels
	sc_create_info.oldSwapchain = VK_NULL_HANDLE;

	ErrorCheck(vkCreateSwapchainKHR(device, &sc_create_info, nullptr, &swapchain));

	vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
	swapchainImages.resize(image_count);
	
	ErrorCheck(vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchainImages.data()));

	swapchainImageFormat = surface_format.format;
	swapchain_extent = extent;
}

VkSurfaceFormatKHR Device::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	// surface has no preferred format , which Vulkan indicates by only returning one VkSurfaceFormatKHR entry (best case scenario)
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	//  go through the list and see if the preferred combination is available (if previous "if" wasn't free to choose any format)
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) // minimum plan
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR Device::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR; // FIFO, IMMEDIATE or MAILBOX

	for (const auto& available_present_mode : availablePresentModes)
	{
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) // best for games
		{
			return available_present_mode;
		}

		else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			best_mode = available_present_mode;
		}
	}

	return best_mode;
}

VkExtent2D Device::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
	{
		return capabilities.currentExtent;
	}
	else 
	{
		glfwGetWindowSize(window, width, height);

		VkExtent2D actual_extent = { 
			static_cast<uint32_t>(*width),
			static_cast<uint32_t>(*height)
		};

		actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}

SwapChainSupportDetails Device::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr); // querying the supported surface formats.

	if (format_count != 0)
	{
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr); //querying the supported presentation modes

	if (present_mode_count != 0)
	{
		details.presentModes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.presentModes.data());
	}

	return details;
}

void Device::DeInitSwapchainImageViews()
{
	for (size_t i = 0; i < swapchainImageViews.size(); i++) {
		vkDestroyImageView(device, swapchainImageViews[i], nullptr);
	}
}

void Device::DestroySwapchainKHR()
{
	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Device::CreateInstance()
{
	VkApplicationInfo application_info = {}; // this data is optional, but it may provide some useful information to the driver to optimize for specific application
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pApplicationName = "GLFW with Vulkan";
	application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	application_info.pEngineName = "No Engine";
	application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	application_info.apiVersion = VK_API_VERSION_1_0;
	
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &application_info;
	
	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
	
#if BUILD_ENABLE_VULKAN_DEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
#else 
	createInfo.enabledLayerCount = 0;
#endif // BUILD_ENABLE_VULKAN_DEBUG

	ErrorCheck(vkCreateInstance(&createInfo, VK_NULL_HANDLE, &instance)); 
}

std::vector<const char*> Device::GetRequiredExtensions() 
{
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		extensions.push_back(glfwExtensions[i]);
	}

#if BUILD_ENABLE_VULKAN_DEBUG
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#else 
#endif 
	
	return extensions;
}

void Device::PickPhysicalDevice() 
{	
	uint32_t gpu_count = 0;
	vkEnumeratePhysicalDevices(instance, &gpu_count, VK_NULL_HANDLE);
		if (gpu_count == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> gpu_list(gpu_count); //allocate an array to hold all of the VkPhysicalDevice handles
		vkEnumeratePhysicalDevices(instance, &gpu_count, gpu_list.data());

		gpu = gpu_list[0];
		vkGetPhysicalDeviceProperties(gpu, &gpu_properties);

		for (const auto& device : gpu_list)
		{
			if (IsDeviceSuitable(device))
			{
				gpu = device;
				break;
			}
		}

		if (gpu == VK_NULL_HANDLE)
		{
			assert(0 && "Vulkan ERROR: Queue family supporting graphics not found.");
			std::exit(-1);
		}
}

void Device::CreateLogicalDevice() // init device
{
	{
		uint32_t family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, VK_NULL_HANDLE);
		std::vector<VkQueueFamilyProperties> family_property_list(family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, family_property_list.data());
	}

	// diagnostics layers provided by the Vulkan SDK.
	{
		uint32_t layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, VK_NULL_HANDLE);
		std::vector<VkLayerProperties> layer_property_list(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, layer_property_list.data());
		std::cout << "Instance Layers: \n";
		for (auto &i : layer_property_list) {
			std::cout << " " << i.layerName << "\t\t | " << i.description << std::endl;
		}
		std::cout << '\n';
	}
	// device layers are depricated in SDK since 1.0.13.0
	{
		uint32_t layer_count = 0;
		vkEnumerateDeviceLayerProperties(gpu, &layer_count, VK_NULL_HANDLE);
		std::vector<VkLayerProperties> layer_property_list(layer_count);
		vkEnumerateDeviceLayerProperties(gpu, &layer_count, layer_property_list.data());
		std::cout << "Device Layers: \n";
		for (auto &i : layer_property_list) {
			std::cout << " " << i.layerName << "\t\t | " << i.description << std::endl;
		}
		std::cout << '\n';
	}

	QueueFamilyIndices indices = FindQueueFamilies(gpu);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	float queue_priorities = { 1.0f };
	for (int graphics_family_index : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo device_queue_create_info{};
		device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		device_queue_create_info.queueFamilyIndex = graphics_family_index;//(queueFamily)
		device_queue_create_info.queueCount = 1;
		device_queue_create_info.pQueuePriorities = &queue_priorities;
		queueCreateInfos.push_back(device_queue_create_info);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.shaderClipDistance = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.wideLines = VK_TRUE;

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	device_create_info.pQueueCreateInfos = queueCreateInfos.data();

	device_create_info.pEnabledFeatures = &deviceFeatures;
	
	device_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	device_create_info.ppEnabledExtensionNames = extensions.data();
	
#if BUILD_ENABLE_VULKAN_DEBUG
	device_create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	device_create_info.ppEnabledLayerNames = validationLayers.data();
#else 
	device_create_info.enabledLayerCount = 0;
#endif // BUILD_ENABLE_VULKAN_DEBUG

	ErrorCheck(vkCreateDevice( gpu, &device_create_info, VK_NULL_HANDLE, &device));

	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &queue);
	vkGetDeviceQueue(device, indices.presentFamily, 0, &present_queue);
}

bool Device::IsDeviceSuitable(VkPhysicalDevice device) // evaluate if GPU is suitable for the operations we want to perform
{
	QueueFamilyIndices indices = FindQueueFamilies(device);
	
	bool extensionsSupported = CheckDeviceExtensionSupport(device);
	
	bool swap_chain_adequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
		swap_chain_adequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(device, &supported_features);
	
	return indices.isComplete() && extensionsSupported && swap_chain_adequate && supported_features.samplerAnisotropy;
}

bool Device::CheckDeviceExtensionSupport(VkPhysicalDevice device) 
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

VkFormat Device::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(gpu, format, &props);


		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	assert(0 && "Vulkan ERROR: failed to find supported format!");
	std::exit(-1);
}

VkFormat Device::FindDepthFormat() {
	return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

QueueFamilyIndices Device::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	// we need to find at least one queue family that supports VK_QUEUE_GRAPHICS_BIT
	int i = 0;
	for (const auto& queueFamily : queueFamilies) 
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport) 
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}
		i++;
	}
	return indices;
}

// ---------------- Buffers ------------------------- //
	
uint32_t Device::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(gpu, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	assert(0 && "Vulkan ERROR: failed to find suitable memory type!");
	std::exit(-1);
}

void Device::CreateStagedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, enginetool::Buffer *buffer, void *data = nullptr)
{
	buffer->m_Device = device;

	VkBufferCreateInfo BufferInfo = {};
	BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferInfo.size = size;
	BufferInfo.usage = usage;
	BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	ErrorCheck(vkCreateBuffer(device, &BufferInfo, nullptr, &buffer->getBuffer()));

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, buffer->getBuffer(), &memory_requirements);

	VkMemoryAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocInfo.allocationSize = memory_requirements.size;
	// Find a memory type index that fits the properties of the buffer
	AllocInfo.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, properties);
	
	ErrorCheck(vkAllocateMemory(device, &AllocInfo, nullptr, &buffer->m_Memory)); // in a real world application, you're not supposed to call vkAllocateMemory for every individual buffer! use VulkanMemoryAllocator

	buffer->m_Alignment = memory_requirements.alignment;
	//buffer->size = AllocInfo.allocationSize;
	buffer->m_UsageFlags = usage;
	buffer->m_MemoryPropertyFags = properties;

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (data != nullptr)
	{
		buffer->Map(size);
		buffer->Copy(AllocInfo.allocationSize, data);
		buffer->Unmap();
	}

	// Initialize a default descriptor that covers the whole buffer size
	buffer->SetupDescriptor(size);

	// Attach the memory to the buffer object
	buffer->Bind();
}

void Device::CreateUnstagedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, enginetool::Buffer *buffer)
{
	buffer->m_Device = device;

	VkBufferCreateInfo BufferInfo = {};
	BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferInfo.size = size;
	BufferInfo.usage = usage;
	BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	ErrorCheck(vkCreateBuffer(device, &BufferInfo, nullptr, &buffer->getBuffer()));

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, buffer->getBuffer(), &memory_requirements);

	VkMemoryAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocInfo.allocationSize = memory_requirements.size;
	// Find a memory type index that fits the properties of the buffer
	AllocInfo.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, properties);
	ErrorCheck(vkAllocateMemory(device, &AllocInfo, nullptr, &buffer->m_Memory)); // in a real world application, you're not supposed to call vkAllocateMemory for every individual buffer! use VulkanMemoryAllocator
	
	buffer->m_Alignment = memory_requirements.alignment;
	//buffer->size = AllocInfo.allocationSize;
	buffer->m_UsageFlags = usage;
	buffer->m_MemoryPropertyFags = properties;

	// Initialize a default descriptor that covers the whole buffer size
	buffer->SetupDescriptor(size);

	// Attach the memory to the buffer object
	buffer->Bind();
}

// ----------------- Render pass -------------------- //

// Specify number of color and depth buffers, how many samples to use for each of them and how their contents should be handled throughout the rendering operations

void Device::CreateRenderPass() {
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = swapchainImageFormat;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	depthFormat = FindDepthFormat();

	VkAttachmentDescription depth_attachment = {};
	depth_attachment.format = depthFormat;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
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

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_references;
	subpass.pDepthStencilAttachment = &depth_attachment_references;

	// specify memory and execution dependencies between subpasses
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	ErrorCheck(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void Device::CreateOffscreenRenderPass(VkFormat format) {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	depthFormat = FindDepthFormat();

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkAttachmentReference colorAttachmentReferences = {};
	colorAttachmentReferences.attachment = 0;
	colorAttachmentReferences.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReferences = {};
	depthAttachmentReferences.attachment = 1;
	depthAttachmentReferences.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReferences;
	subpassDescription.pDepthStencilAttachment = &depthAttachmentReferences;

	std::array<VkSubpassDependency, 2> dependencies;	
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = dependencies.data();

	ErrorCheck(vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenRenderPass));
}

void Device::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory) {
	VkBufferCreateInfo BufferInfo = {};
	BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferInfo.size = size;
	BufferInfo.usage = usage;
	BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	ErrorCheck(vkCreateBuffer(device, &BufferInfo, nullptr, &buffer));

	VkMemoryRequirements  memory_requirements;
	vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

	VkMemoryAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocInfo.allocationSize = memory_requirements.size;
	AllocInfo.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, properties);

	ErrorCheck(vkAllocateMemory(device, &AllocInfo, nullptr, &buffer_memory)); // in a real world application, you're not supposed to call vkAllocateMemory for every individual buffer! use VulkanMemoryAllocator

	vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

// --------------------- DEBUG ---------------------- //

#if BUILD_ENABLE_VULKAN_DEBUG

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT obj_type,
	uint64_t stc_obj,
	size_t location,
	int32_t msg_code,
	const char * layer_prefix,
	const char * msg,
	void * user_data) {
	std::ostringstream stream;
	stream << "VKDBG: ";
	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) { stream << "INFO: "; }
	if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) { stream << "WARNING: "; }
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) { stream << "PERFORMANCE: "; }
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) { stream << "ERROR: "; }
	if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) { stream << "DEBUG: "; }  
	

	stream << "@[" << layer_prefix << "]" << std::endl;
	stream << msg << std::endl;
	std::cout << stream.str();

#ifdef WIN32
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		MessageBox(nullptr, stream.str().c_str(), "Vulkan Error!", 0);
	}
#endif 

	return VK_FALSE;
}

void Device::SetupDebugCallback()
{
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debugCreateInfo.pfnCallback = VulkanDebugCallback;
	debugCreateInfo.flags =	VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT | 0; 
}

void Device::InitDebug()
{
	CreateDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
	DestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
	if (VK_NULL_HANDLE == CreateDebugReportCallback || VK_NULL_HANDLE == DestroyDebugReportCallback) {
		assert(0 && "Vulkan ERROR: Can't fetch debug function pointers.");
		std::exit(-1);
	}

	CreateDebugReportCallback(instance, &debugCreateInfo, VK_NULL_HANDLE, &callback);

	//	vkCreateDebugReportCallbackEXT( _instance, nullptr, nullptr, nullptr );
}

void Device::DeInitDebug() {
	DestroyDebugReportCallback(instance, callback, VK_NULL_HANDLE);
	callback = VK_NULL_HANDLE;
}

#else 

void Device::SetupDebugCallback() {};
void Device::InitDebug() {};
void Device::DeInitDebug() {};

#endif // BUILD_ENABLE_VULKAN_DEBUG

void Device::DestroyRenderPass() {
	vkDestroyRenderPass(device, renderPass, nullptr);
}

void Device::DestroyOffscreenRenderPass() {
	vkDestroyRenderPass(device, offscreenRenderPass, nullptr);
}