#include <assert.h> // If the argument equal to zero, a message is written to the standard error device and abort is called
#include <vulkan/vulkan.h> 

#define BUILD_ENABLE_VULKAN_DEBUG			1
#define BUILD_ENABLE_VULKAN_RUNTIME_DEBUG	1

void ErrorCheck(VkResult result);