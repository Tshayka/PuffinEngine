#pragma once

#include <vector>
#include <memory>

#ifdef UNIX
#define NOMINMAX
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> //#include <vulkan/vulkan.h> is not needed

#include "src/Threads.cpp"
#include "headers/WorldClock.hpp"

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool isComplete() { 
		return graphicsFamily >= 0 && presentFamily >= 0; 
	}
};

struct SwapChainSupportDetails {// struct to pass details around once they've been queried
	VkSurfaceCapabilitiesKHR capabilities; // basic surface capabilities
	std::vector<VkSurfaceFormatKHR> formats; // surface formats - pixel format, color space
	std::vector<VkPresentModeKHR> presentModes; // available presentation modes
};

class Device {
public:
	Device();
	~Device();

	Device(const Device&) = delete;
	Device(Device&&) = delete;
	Device& operator=(const Device&) = delete;
	Device& operator=(Device&&) = delete;

	VkDevice get() const;
	VkPhysicalDevice getGpu() const;
	VkSurfaceKHR getSurface() const;
	VkPhysicalDeviceProperties getGpuProperties() const;
	VkQueue getQueue() const;
	VkQueue getPresentQueue() const;

	void init(GLFWwindow* window);
	void deInit();
	VkShaderModule CreateShaderModule(const std::vector<char>&);
	uint32_t FindMemoryType(uint32_t, VkMemoryPropertyFlags);
	QueueFamilyIndices findQueueFamilies();
	VkFormat FindDepthFormat();
	SwapChainSupportDetails querySwapChainSupport(); // populate "SwapChainSupportDetails" struct

	std::vector<VkFramebuffer> m_SwapChainFramebuffers;
	VkFramebuffer m_ReflectionFramebuffer;
	VkFramebuffer m_RefractionFramebuffer;

private:
	bool checkDeviceExtensionSupport(const VkPhysicalDevice& gpu);
	void createSurface();
	void CreateInstance();
	void CreateLogicalDevice();
	void CreateRenderPass();
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	void InitDebug();
	void DeInitDebug();
	bool isDeviceSuitable(const VkPhysicalDevice& gpu);
	void PickPhysicalDevice();
	void SetupDebugCallback();

	VkDevice device = nullptr;
	VkPhysicalDevice m_Gpu = nullptr;
	VkPhysicalDeviceProperties m_GpuProperties = {};
	VkSurfaceKHR m_Surface = nullptr;
	GLFWwindow* p_Window = nullptr;	
	VkInstance m_Instance = nullptr;
	VkDebugReportCallbackEXT m_Callback;
	VkQueue m_Queue = nullptr;
	VkQueue m_PresentQueue = nullptr;

	uint32_t extension_count = 0;
	uint32_t graphics_family_index = 0;
    
	std::vector<const char*> GetRequiredExtensions();
	const std::vector<const char*> validationLayers = {"VK_LAYER_LUNARG_standard_validation"};
	const std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDebugReportCallbackCreateInfoEXT debugCreateInfo = {};

	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
};