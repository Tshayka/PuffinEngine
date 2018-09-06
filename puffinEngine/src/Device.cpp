#include <iostream>

#define BUILD_ENABLE_VULKAN_DEBUG			1
#define BUILD_ENABLE_VULKAN_RUNTIME_DEBUG	1

#include "Device.hpp"


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
}

void Device::DeInitDevice() 
{
	vkDestroyDevice(device, nullptr);
}

void Device::CreateInstance()
{
	VkApplicationInfo applicationInfo = {}; 
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "PufinEngine";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pEngineName = "No Engine";
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_0;
	
	VkInstanceCreateInfo create_info = {}; // tells the Vulkan driver which global extensions and validation layers we want to use
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &applicationInfo;
	
	auto extensions = GetRequiredExtensions();
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
	
#if BUILD_ENABLE_VULKAN_DEBUG
	create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	create_info.ppEnabledLayerNames = validationLayers.data();
#else 
	create_info.enabledLayerCount = 0;
#endif // BUILD_ENABLE_VULKAN_DEBUG

	vkCreateInstance(&create_info, VK_NULL_HANDLE, &instance); // TODO tests
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