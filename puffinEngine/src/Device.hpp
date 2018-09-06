#pragma once

#include <vector>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> //#include <vulkan/vulkan.h> is not needed

#include "Buffer.cpp"

class Device
{
public:
	Device();
	~Device();
    
    VkDevice device = VK_NULL_HANDLE;

    void InitDevice(GLFWwindow*);
    void DeInitDevice();

private:
    
    VkInstance instance = VK_NULL_HANDLE;
	GLFWwindow* window;

	void CreateInstance();
    std::vector<const char*> GetRequiredExtensions();

    const std::vector<const char*> validationLayers = {"VK_LAYER_LUNARG_standard_validation"};
};