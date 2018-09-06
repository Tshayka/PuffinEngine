#include <iostream>
#include <memory>
#include <string>

#include "PuffinEngine.hpp"

 //---------- Constructors and dectructors ---------- //

PuffinEngine::PuffinEngine(){}

PuffinEngine::~PuffinEngine(){}

std::string PuffinEngine::isUppercase(char l)
{
if(l & 0b00100000)
    return "This letter is lowercase!";
else
    return "This letter is uppercase!";
}

void PuffinEngine::Run()
{
	InitWindow();
    InitVulkan();
    MainLoop();
    CleanUp();
}

void PuffinEngine::InitWindow() {

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(800, 600, "PuffinEngine", nullptr, nullptr);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
}

void PuffinEngine::InitVulkan()
{
	CreateDevice();
}

void PuffinEngine::CreateDevice()
{
	world_device = std::make_shared<Device>();
	world_device->InitDevice(window);
}

void PuffinEngine::MainLoop()
{
	while (!glfwWindowShouldClose(window)){		
	}

	vkDeviceWaitIdle(world_device->device);
}

// ---------------- Deinitialisation ---------------- //

void PuffinEngine::CleanUp()
{
	DestroyDevice();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void PuffinEngine::DestroyDevice()
{
	world_device->DeInitDevice();
	world_device = nullptr;
}