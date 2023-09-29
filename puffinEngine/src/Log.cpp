#include <string>

#include "../headers/Log.hpp"

std::string details;

// TODO log as macro to femove copletly cout from non debug!

void puffinengine::tool::checkResult(VkResult result){
	if (result != VK_SUCCESS) {
		switch (result) {
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			details = "VK_ERROR_OUT_OF_HOST_MEMORY";
			break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			details = "VK_ERROR_OUT_OF_DEVICE_MEMORY";
			break;
		case VK_ERROR_INITIALIZATION_FAILED:
			details = "VK_ERROR_INITIALIZATION_FAILED";
			break;
		case VK_ERROR_DEVICE_LOST:
			details = "VK_ERROR_DEVICE_LOST";
			break;
		case VK_ERROR_MEMORY_MAP_FAILED:
			details = "VK_ERROR_MEMORY_MAP_FAILED";
			break;
		case VK_ERROR_LAYER_NOT_PRESENT:
			details = "VK_ERROR_LAYER_NOT_PRESENT";
			break;
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			details = "VK_ERROR_EXTENSION_NOT_PRESENT";
			break;
		case VK_ERROR_FEATURE_NOT_PRESENT:
			details = "VK_ERROR_FEATURE_NOT_PRESENT";
			break;
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			details = "VK_ERROR_INCOMPATIBLE_DRIVER";
			break;
		case VK_ERROR_TOO_MANY_OBJECTS:
			details = "VK_ERROR_TOO_MANY_OBJECTS";
			break;
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			details = "VK_ERROR_FORMAT_NOT_SUPPORTED";
			break;
		case VK_ERROR_SURFACE_LOST_KHR:
			details = "VK_ERROR_SURFACE_LOST_KHR";
			break;
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			details = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
			break;
		case VK_SUBOPTIMAL_KHR:
			details = "VK_SUBOPTIMAL_KHR";
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			details = "VK_ERROR_OUT_OF_DATE_KHR";
			break;
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			details = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
			break;
		case VK_ERROR_VALIDATION_FAILED_EXT:
			details = "VK_ERROR_VALIDATION_FAILED_EXT";
			break;
		default:
			break;
		}
		throw std::runtime_error(details);
	}
}
