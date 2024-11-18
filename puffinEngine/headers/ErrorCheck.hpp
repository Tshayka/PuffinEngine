#pragma once

#include <assert.h> // If the argument equal to zero, a message is written to the standard error device and abort is called

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> //#include <vulkan/vulkan.h> is not needed

#define DEBUG_VERSION			0
#define BUILD_ENABLE_VULKAN_RUNTIME_DEBUG	1

void ErrorCheck(VkResult result);