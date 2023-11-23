#include <array>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "headers/Device.hpp"
#include "headers/ErrorCheck.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

// ------- Constructors and dectructors ------------- //

Device::Device() {
#if DEBUG_VERSION
	std::cout << "Device object created\n";
#endif 
}

Device::~Device() {
#if DEBUG_VERSION
	std::cout << "Device object destroyed\n";
#endif 
}

// --------------- Setters and getters -------------- //

VkDevice Device::get() const {
	return device;
}

VkPhysicalDevice Device::getGpu() const {
	return m_Gpu;
}

VkSurfaceKHR Device::getSurface() const {
	return m_Surface;
}

VkPhysicalDeviceProperties Device::getGpuProperties() const {
	return m_GpuProperties;
}

VkQueue Device::getQueue() const {
	return m_Queue;
}

VkQueue Device::getPresentQueue() const {
	return m_PresentQueue;
}

// ---------------- Main functions ------------------ //

void Device::init(GLFWwindow* window) {
	p_Window = window;

	CreateInstance();
    SetupDebugCallback();
    InitDebug();
    createSurface();
    PickPhysicalDevice();
	CreateLogicalDevice();
}

void Device::deInit(){
    DeInitDebug();
	vkDestroyDevice(device, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
}

void Device::createSurface() {
	if (glfwCreateWindowSurface(m_Instance, p_Window, nullptr, &m_Surface) != VK_SUCCESS) {
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

SwapChainSupportDetails Device::querySwapChainSupport() {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Gpu, m_Surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_Gpu, m_Surface, &format_count, nullptr); // querying the supported surface formats.

	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_Gpu, m_Surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_Gpu, m_Surface, &present_mode_count, nullptr); //querying the supported presentation modes

	if (present_mode_count != 0) {
		details.presentModes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_Gpu, m_Surface, &present_mode_count, details.presentModes.data());
	}

	return details;
}

void Device::CreateInstance() {
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
	
#if DEBUG_VERSION
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
#else 
	createInfo.enabledLayerCount = 0;
#endif // BUILD_ENABLE_VULKAN_DEBUG

	ErrorCheck(vkCreateInstance(&createInfo, VK_NULL_HANDLE, &m_Instance));
}

std::vector<const char*> Device::GetRequiredExtensions() {
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		extensions.push_back(glfwExtensions[i]);
	}

#if DEBUG_VERSION
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#else 
#endif 
	
	return extensions;
}

void Device::PickPhysicalDevice() {	
	uint32_t gpu_count = 0;
	vkEnumeratePhysicalDevices(m_Instance, &gpu_count, VK_NULL_HANDLE);
		if (gpu_count == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> gpuList(gpu_count); //allocate an array to hold all of the VkPhysicalDevice handles
		vkEnumeratePhysicalDevices(m_Instance, &gpu_count, gpuList.data());

		m_Gpu = gpuList[0];
		vkGetPhysicalDeviceProperties(m_Gpu, &m_GpuProperties);

		for (const auto& gpu : gpuList) {
			if (isDeviceSuitable(gpu)) {
				m_Gpu = gpu;
				break;
			}
		}

		if (m_Gpu == nullptr) {
			assert(0 && "Vulkan ERROR: Queue family supporting graphics not found.");
			std::exit(-1);
		}
}

void Device::CreateLogicalDevice() { // init device
	{
		uint32_t family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_Gpu, &family_count, VK_NULL_HANDLE);
		std::vector<VkQueueFamilyProperties> family_property_list(family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_Gpu, &family_count, family_property_list.data());
	}

	// diagnostics layers provided by the Vulkan SDK.
	{
		uint32_t layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, VK_NULL_HANDLE);
		std::vector<VkLayerProperties> layer_property_list(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, layer_property_list.data());
		std::cout << "Instance Layers: \n";
		for (const VkLayerProperties &i : layer_property_list) {
			std::cout << " " << i.layerName << "\t\t | " << i.description << std::endl;
		}
		std::cout << '\n';
	}
	// device layers are depricated in SDK since 1.0.13.0
	{
		uint32_t layer_count = 0;
		vkEnumerateDeviceLayerProperties(m_Gpu, &layer_count, VK_NULL_HANDLE);
		std::vector<VkLayerProperties> layer_property_list(layer_count);
		vkEnumerateDeviceLayerProperties(m_Gpu, &layer_count, layer_property_list.data());
		std::cout << "Device Layers: \n";
		for (auto &i : layer_property_list) {
			std::cout << " " << i.layerName << "\t\t | " << i.description << std::endl;
		}
		std::cout << '\n';
	}

	QueueFamilyIndices indices = findQueueFamilies();

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
	
#if DEBUG_VERSION
	device_create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	device_create_info.ppEnabledLayerNames = validationLayers.data();
#else 
	device_create_info.enabledLayerCount = 0;
#endif // BUILD_ENABLE_VULKAN_DEBUG

	ErrorCheck(vkCreateDevice(m_Gpu, &device_create_info, VK_NULL_HANDLE, &device));

	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &m_Queue);
	vkGetDeviceQueue(device, indices.presentFamily, 0, &m_PresentQueue);
}

bool Device::isDeviceSuitable(const VkPhysicalDevice& gpu) { // evaluate if GPU is suitable for the operations we want to perform
	QueueFamilyIndices indices = findQueueFamilies();
	
	bool extensionsSupported = checkDeviceExtensionSupport(gpu);
	
	bool swap_chain_adequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport();
		swap_chain_adequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(gpu, &supported_features);
	
	return indices.isComplete() && extensionsSupported && swap_chain_adequate && supported_features.samplerAnisotropy;
}

bool Device::checkDeviceExtensionSupport(const VkPhysicalDevice& gpu) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

VkFormat Device::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_Gpu, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)	{
			return format;
		}
	}

	assert(0 && "Vulkan ERROR: failed to find supported format!");
	std::exit(-1);
}

VkFormat Device::FindDepthFormat() {
	return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

QueueFamilyIndices Device::findQueueFamilies() {
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_Gpu, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_Gpu, &queueFamilyCount, queueFamilies.data());

	// we need to find at least one queue family that supports VK_QUEUE_GRAPHICS_BIT
	int i = 0;
	for (const auto& queueFamily : queueFamilies) 
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_Gpu, i, m_Surface, &presentSupport);

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
	vkGetPhysicalDeviceMemoryProperties(m_Gpu, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	assert(0 && "Vulkan ERROR: failed to find suitable memory type!");
	std::exit(-1);
}

//void Device::CreateStagedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, enginetool::Buffer *buffer, void *data = nullptr) {
//	VkBufferCreateInfo BufferInfo = {};
//	BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//	BufferInfo.size = size;
//	BufferInfo.usage = usage;
//	BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//	ErrorCheck(vkCreateBuffer(device, &BufferInfo, nullptr, &buffer->getBuffer()));
//
//	VkMemoryRequirements memory_requirements;
//	vkGetBufferMemoryRequirements(device, buffer->getBuffer(), &memory_requirements);
//
//	VkMemoryAllocateInfo AllocInfo = {};
//	AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//	AllocInfo.allocationSize = memory_requirements.size;
//	// Find a memory type index that fits the properties of the buffer
//	AllocInfo.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, properties);
//	
//	ErrorCheck(vkAllocateMemory(device, &AllocInfo, nullptr, &buffer->m_Memory)); // in a real world application, you're not supposed to call vkAllocateMemory for every individual buffer! use VulkanMemoryAllocator
//
//	buffer->m_Alignment = memory_requirements.alignment;
//	//buffer->size = AllocInfo.allocationSize;
//	buffer->m_UsageFlags = usage;
//	buffer->m_MemoryPropertyFags = properties;
//
//	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
//	if (data != nullptr)
//	{
//		buffer->Map(size);
//		buffer->Copy(size, data);
//		buffer->Unmap();
//	}
//
//	// Initialize a default descriptor that covers the whole buffer size
//	buffer->SetupDescriptor(size);
//
//	// Attach the memory to the buffer object
//	buffer->Bind();
//}

// --------------------- DEBUG ---------------------- //

#if DEBUG_VERSION

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