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

	// TODO rule of 5!

	VkDevice get() const {
		return device;
	}

	VkPhysicalDevice getGpu() const {
		return m_Gpu;
	}

	VkSurfaceKHR getSurface() const {
		return m_Surface;
	}

	VkPhysicalDeviceProperties getGpuProperties() const {
		return m_GpuProperties;
	}

	void init(GLFWwindow*);
	void deInit();
	//void CreateOffscreenRenderPass(VkFormat format);
	VkShaderModule CreateShaderModule(const std::vector<char>&);
	uint32_t FindMemoryType(uint32_t, VkMemoryPropertyFlags);
	QueueFamilyIndices findQueueFamilies();
	VkFormat FindDepthFormat();

	VkQueue present_queue = nullptr;
	VkQueue queue = nullptr;

	int* width; 
	int* height;

	/*SWAPCHAIN*/
	// Functions that retrive INFO needed to create swapchain
	SwapChainSupportDetails querySwapChainSupport(); // populate "SwapChainSupportDetails" struct
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&); // find the right settings for the best possible swap chain - Surface format (color depth)
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>); // represents the actual conditions for showing images to the screen (4 possible modes)
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR&); // swap extent is the resolution of the swap chain images, it's almost always exactly equal to the resolution of the window that we're drawing to
	
	std::vector<VkFramebuffer> swap_chain_framebuffers;

	VkFramebuffer reflectionFramebuffer;
	VkFramebuffer refractionFramebuffer;

private:
	VkDevice device = nullptr;
	VkPhysicalDevice m_Gpu = nullptr;
	VkPhysicalDeviceProperties m_GpuProperties = {};
	VkSurfaceKHR m_Surface = nullptr;
	GLFWwindow* p_Window;	
	
	void createSurface();
	bool checkDeviceExtensionSupport(const VkPhysicalDevice& gpu);
	void CreateInstance();
	void CreateLogicalDevice();
	void CreateRenderPass();
	void InitDebug();
	void DeInitDebug();
	VkFormat FindSupportedFormat(const std::vector<VkFormat>&, VkImageTiling, VkFormatFeatureFlags);
	bool isDeviceSuitable(const VkPhysicalDevice& gpu);
	void PickPhysicalDevice();
	void SetupDebugCallback();
			
	VkInstance instance = VK_NULL_HANDLE;

	VkDebugReportCallbackEXT callback = VK_NULL_HANDLE;

	uint32_t extension_count = 0;
	uint32_t graphics_family_index = 0;
    
	std::vector<const char*> GetRequiredExtensions();
	const std::vector<const char*> validationLayers = {"VK_LAYER_LUNARG_standard_validation"};
	const std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDebugReportCallbackCreateInfoEXT debugCreateInfo = {};

	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
};