#pragma once

#include <vector>
#include <chrono> 
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> 

class Device
{
public:
	Device();
    ~Device();
};