#include <iostream>
#include <memory>
#include <string>

#include "PuffinEngine.hpp"

 //---------- Constructors and dectructors ---------- //

PuffinEngine::PuffinEngine() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Engine object created\n";
#endif 
}

PuffinEngine::~PuffinEngine() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Engine object destroyed\n";
#endif
}

std::string PuffinEngine::isUppercase(char l)
{
if(l & 0b00100000)
    return "This letter is lowercase!";
else
    return "This letter is uppercase!";
}

// ---------------- Main functions ------------------ //

void PuffinEngine::Run()
{
	InitWindow();
    InitVulkan();
    MainLoop();
    CleanUp();
}

void PuffinEngine::CreateDevice() {
	world_device = std::make_shared<Device>();
	world_device->InitDevice(window);
}

void PuffinEngine::CreateImGuiMenu() {
	console = std::make_shared<ImGuiMenu>();
	console->InitMenu(world_device);
}

void PuffinEngine::CreateStatusOverlay() {
	statusOverlay = new StatusOverlay();
	statusOverlay->InitStatusOverlay(world_device, console);
}

void PuffinEngine::CreateScene() {
	scene_1 = new Scene();
	scene_1->InitScene(world_device, window, console, statusOverlay);
}

void PuffinEngine::InitVulkan() {
	CreateDevice();
	CreateImGuiMenu();
	CreateStatusOverlay();
	CreateScene();
	CreateSemaphores();
}

void PuffinEngine::MainLoop() {
	double total_time = 0.0;
	double fixed_frame_time = 0.016666;

	double current_time = glfwGetTime();
	double accumulator = 0.0f;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		glfwGetCursorPos(window, &xpos, &ypos);
		glfwGetFramebufferSize(window, &fb_width, &fb_height);
		glfwGetWindowSize(window, &width, &height);
	
		double new_time = glfwGetTime();
		double frameTime = new_time - current_time;

		ImGuiIO& io = ImGui::GetIO(); 
		io.DisplaySize = ImVec2((float)width, (float)height);
		io.DisplayFramebufferScale = ImVec2(width > 0 ? ((float)fb_width / width) : 0, height > 0 ? ((float)fb_height / height) : 0);
		io.DeltaTime = (float)frameTime;

		if (frameTime > 0.15f)
			frameTime = 0.15f;

		current_time = glfwGetTime();//TODO
		
		accumulator += frameTime;

		while (accumulator >= fixed_frame_time) {
			scene_1->UpdateGUI((float)accumulator, (uint32_t)total_time);
			scene_1->UpdateScene((float)total_time);

			total_time += fixed_frame_time;
			accumulator -= fixed_frame_time;
		}

		//std::cout << "Inside Loop!" << '\n';
		UpdateCameras((float)frameTime);
		DrawFrame();		
	}

	vkDeviceWaitIdle(world_device->device);
}

void PuffinEngine::UpdateCameras(float dt) {
	scene_1->camera->UpdatePosition(static_cast<float>(dt)); // TODO not update when camera not moving
}


void PuffinEngine::CreateSemaphores() {
	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	ErrorCheck(vkCreateSemaphore(world_device->device, &semaphore_info, nullptr, &ImageAvailableSemaphore));
	ErrorCheck(vkCreateSemaphore(world_device->device, &semaphore_info, nullptr, &RenderFinishedSemaphore));
}

void PuffinEngine::DrawFrame(){
	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(world_device->device, world_device->swap_chain, std::numeric_limits<uint64_t>::max(), ImageAvailableSemaphore, VK_NULL_HANDLE, &image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		assert(0 && "Failed to acquire swap chain image!");
		std::exit(-1);
	}

	// Submitting the command buffer
	VkSubmitInfo submit_info = {}; // queue submission and synchronization is configured through parameters
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { ImageAvailableSemaphore };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = waitSemaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &scene_1->command_buffers[image_index];

	VkSemaphore signal_semaphores[] = { RenderFinishedSemaphore };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

	ErrorCheck(vkQueueSubmit(world_device->queue, 1, &submit_info, VK_NULL_HANDLE));

	statusOverlay->Submit(world_device->queue, image_index);
	
	/*std::array<VkCommandBuffer, 2>command_buffers;
	command_buffers[0] = scene_1->command_buffers[image_index];
	command_buffers[1] = stats_overlay->command_buffers[image_index];*/

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swap_chains[] = { world_device->swap_chain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swap_chains;
	present_info.pImageIndices = &image_index;

	result = vkQueuePresentKHR(world_device->present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		assert(0 && "Failed to present swap chain image!");
		std::exit(-1);
	}

	vkQueueWaitIdle(world_device->present_queue);
}

void PuffinEngine::RecreateSwapChain()
{
	glfwGetWindowSize(window, &width, &height);
	if (width == 0 || height == 0) return;

	vkDeviceWaitIdle(world_device->device);

	CleanUpSwapChain();

	world_device->InitSwapChain();
	scene_1->RecreateForSwapchain();
}


// ----------- Callbacks and GLFW ------------------- //

void PuffinEngine::InitWindow() {

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(800, 600, "PuffinEngine", nullptr, nullptr);

	glfwSetKeyCallback(window, PuffinEngine::KeyCallback);
	glfwSetMouseButtonCallback(window, PuffinEngine::MouseButtonCallback);
	glfwSetCharCallback(window, PuffinEngine::CharacterCallback);
	glfwSetCursorEnterCallback(window, PuffinEngine::CursorEnterCallback);
	glfwSetCursorPosCallback(window, PuffinEngine::CursorPositionCallback);
	glfwSetWindowUserPointer(window, this);
	glfwSetWindowSizeCallback(window, PuffinEngine::onWindowResized);
	glfwSetFramebufferSizeCallback(window, PuffinEngine::FramebufferSizeCallback);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_FALSE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwSetScrollCallback(window, PuffinEngine::ScrollCallback);
	glfwSetErrorCallback(PuffinEngine::ErrorCallback);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;   // We can honor GetMouseCursor() values (optional)
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos; // We can honor io.WantSetMousePos requests (optional, rarely used)

	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
}

void PuffinEngine::CharacterCallback(GLFWwindow* window, unsigned int codepoint) {
	ImGuiIO& io = ImGui::GetIO();
	if (codepoint > 0 && codepoint < 0x10000)
		io.AddInputCharacter((unsigned short)codepoint);
}

void PuffinEngine::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
	PuffinEngine* app = reinterpret_cast<PuffinEngine*>(glfwGetWindowUserPointer(window));
	app->scene_1->camera->MouseMove(xpos, ypos, app->width, app->height, 0.005f);

	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2((float)xpos, (float)ypos);
}


void PuffinEngine::ErrorCallback(int error, const char* description) {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cerr << description << std::endl;
#endif
}

void PuffinEngine::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {}

void PuffinEngine::KeyCallback(GLFWwindow* window, int key, int scancode, int event, int mods) {
	PuffinEngine* app = reinterpret_cast<PuffinEngine*>(glfwGetWindowUserPointer(window));
	app->scene_1->PressKey(key);
	
	ImGuiIO& io = ImGui::GetIO();
	if (event == GLFW_PRESS)
		io.KeysDown[key] = true;
	if (event == GLFW_RELEASE)
		io.KeysDown[key] = false;

	(void)mods; // Modifiers are not reliable across systems
	io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
	io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
	io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
	io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}

void PuffinEngine::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	ImGuiIO& io = ImGui::GetIO();

#if BUILD_ENABLE_VULKAN_DEBUG
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		std::cout << "You clicked right mouse button" << std::endl;
	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		std::cout << "You clicked left mouse button" << std::endl;
			
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
		std::cout << "You clicked middle mouse button" << std::endl;
#endif

	if ((button >= 0) && (button < 3)) {
		ImGui::GetIO().MouseDown[button] = (action == GLFW_PRESS);
	}
}

void PuffinEngine::onWindowResized(GLFWwindow* window, int width, int height) {
	//if (width == 0 || height == 0) return;

	PuffinEngine* app = reinterpret_cast<PuffinEngine*>(glfwGetWindowUserPointer(window));
	app->RecreateSwapChain();
}

void PuffinEngine::CursorEnterCallback(GLFWwindow* window, int entered) {
#if BUILD_ENABLE_VULKAN_DEBUG
	if (entered) std::cout << "Entered window" << std::endl;
	else std::cout << "Left window" << std::endl;
#endif

	PuffinEngine* app = reinterpret_cast<PuffinEngine*>(glfwGetWindowUserPointer(window));

	ImGuiIO& io = ImGui::GetIO();
	io.NavActive = true;

	if (!entered) io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	else io.MousePos = ImVec2((float)app->xpos, (float)app->ypos);
}

void PuffinEngine::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	ImGuiIO& io = ImGui::GetIO();
	io.MouseWheelH += (float)xoffset;
	io.MouseWheel += (float)yoffset;
}

// ---------------- Deinitialisation ---------------- //

void PuffinEngine::CleanUp() {
	CleanUpSwapChain();

	ImGui::DestroyContext();
	DeInitSemaphores();
	DestroyGUI();
	DestroyScene();
	DestroyDevice();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void PuffinEngine::CleanUpSwapChain() {
	scene_1->CleanUpForSwapchain();
	world_device->DeInitSwapchainImageViews();
	world_device->DestroySwapchainKHR();
}

void PuffinEngine::DeInitSemaphores() {
	vkDestroySemaphore(world_device->device, RenderFinishedSemaphore, nullptr);
	vkDestroySemaphore(world_device->device, ImageAvailableSemaphore, nullptr);
}

void PuffinEngine::DestroyDevice() {
	world_device->DeInitDevice();
	world_device = nullptr;
}

void PuffinEngine::DestroyScene() {
	scene_1->DeInitScene();
	delete scene_1;
	scene_1 = nullptr;
}

void PuffinEngine::DestroyGUI() {
	statusOverlay->DeInitStatusOverlay();
	delete statusOverlay;
	statusOverlay = nullptr;
	console->DeInitMenu();
}