#pragma once

#include <assert.h>
#include <vulkan/vulkan.h>
#include <iostream>

namespace puffinengine {
	namespace tool {
		void checkResult(VkResult result);
	}
}