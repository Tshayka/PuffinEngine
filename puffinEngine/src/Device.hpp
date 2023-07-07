#pragma once

#include <vector>
#include <memory>

#ifdef UNIX
#define NOMINMAX
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> //#include <vulkan/vulkan.h> is not needed

#include "Buffer.cpp"
#include "Threads.cpp"
#include "WorldClock.hpp"

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

	void DeInitDevice();
	void CreateOffscreenRenderPass(VkFormat format);
	VkShaderModule CreateShaderModule(const std::vector<char>&);
	void CreateStagedBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, enginetool::Buffer*, void*);
	void CreateUnstagedBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, enginetool::Buffer*);
	void CreateBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	uint32_t FindMemoryType(uint32_t, VkMemoryPropertyFlags);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice);
	void InitDevice(GLFWwindow*);
	void InitSwapChain(); // queue of images that are waiting to be presented to the screen, swap chain synchronize the presentation of images with the refresh rate of the screen
	void DeInitSwapchainImageViews();
	void DestroyOffscreenRenderPass();
	void DestroyRenderPass();
	void DestroySwapchainKHR();
	VkFormat FindDepthFormat();

	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice gpu = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties gpu_properties = {};
	VkQueue present_queue = VK_NULL_HANDLE;
	VkQueue queue = VK_NULL_HANDLE;
	VkRenderPass offscreenRenderPass;
	VkRenderPass renderPass;
	VkFormat depthFormat;
	int* width; 
	int* height;

	/*SWAPCHAIN*/
	// Functions that retrive INFO needed to create swapchain
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice); // populate "SwapChainSupportDetails" struct
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&); // find the right settings for the best possible swap chain - Surface format (color depth)
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>); // represents the actual conditions for showing images to the screen (4 possible modes)
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR&); // swap extent is the resolution of the swap chain images, it's almost always exactly equal to the resolution of the window that we're drawing to
	
	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchain_extent;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swap_chain_framebuffers;
	VkFramebuffer reflectionFramebuffer;
	VkFramebuffer refractionFramebuffer;

private:
	GLFWwindow* window;	
	bool CheckDeviceExtensionSupport(VkPhysicalDevice);
	void CreateInstance();
	void CreateLogicalDevice();
	void CreateRenderPass();
	void CreateSurface();
	void DeInitDebug();
	VkFormat FindSupportedFormat(const std::vector<VkFormat>&, VkImageTiling, VkFormatFeatureFlags);
	void InitDebug();
	bool IsDeviceSuitable(VkPhysicalDevice);
	void PickPhysicalDevice();
	void SetupDebugCallback();
			
	VkInstance instance = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

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