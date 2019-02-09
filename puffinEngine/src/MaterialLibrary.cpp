#include <iostream>

#include "MaterialLibrary.hpp"
#include "ErrorCheck.hpp"

// ------- Constructors and dectructors ------------- //

MaterialLibrary::MaterialLibrary() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Material library created\n";
#endif
}

MaterialLibrary::~MaterialLibrary() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Material library destroyed\n";
#endif 
}

// ---------------- Main functions ------------------ //

void MaterialLibrary::Init(Device* device) {
	logicalDevice = device; 
}


void MaterialLibrary::DeInit() {
	logicalDevice = nullptr;
}