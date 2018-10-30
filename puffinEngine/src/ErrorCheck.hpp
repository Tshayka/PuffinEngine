#pragma once

#include <assert.h> // If the argument equal to zero, a message is written to the standard error device and abort is called
#include <vulkan/vulkan.h> 

void ErrorCheck(VkResult result);