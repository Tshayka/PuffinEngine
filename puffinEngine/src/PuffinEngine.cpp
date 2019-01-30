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

template<typename T>
T clamp(const T& what, const T& a, const T& b) {
return std::min(b, std::max(what, a)); 
}

// ---------------- Main functions ------------------ //

void PuffinEngine::Run() {
	InitWindow();
	InitDefaultKeysBindings(functions);
    InitVulkan();
    MainLoop();
    CleanUp();
}

void PuffinEngine::CreateDevice() {
	world_device = new Device();
	world_device->InitDevice(window);
}

void PuffinEngine::CreateImGuiMenu() {
	console = new GuiElement();
	console->InitMenu(world_device);
}

void PuffinEngine::CreateGuiTextOverlay() {
	guiStatistics = new GuiTextOverlay();
	guiStatistics->Init(world_device);
}

void PuffinEngine::CreateMainUi() {
	mainUi = new GuiMainUi();
	mainUi->Init(world_device);
}

void PuffinEngine::CreateGuiMainHub() {
	guiMainHub = new GuiMainHub();
	guiMainHub->Init(world_device, console, guiStatistics, mainUi);
}

void PuffinEngine::CreateMousePicker() {
	mousePicker = new MousePicker();
	mousePicker->Init(world_device);
}

void PuffinEngine::CreateMainCharacter() {
	//mainCharacter = std::make_unique<MainCharacter>("Temp", "Brave hero", glm::vec3(0.0f, 0.0f, 0.0f), ActorType::MainCharacter);
	//dynamic_cast<MainCharacter*>(mainCharacter.get())->Init(1000, 1000, 100);
}

void PuffinEngine::CreateScene() {
	scene_1 = new Scene();
	scene_1->InitScene(world_device, guiMainHub, mousePicker);
}

void PuffinEngine::InitVulkan() {
	CreateDevice();
	CreateImGuiMenu();
	CreateGuiTextOverlay();
	CreateMainUi();
	CreateGuiMainHub();
	CreateMousePicker();
	CreateScene();
	CreateSemaphores();
}

void PuffinEngine::MainLoop() {
	double totalTime = 0.0;
	const double fixedTimeValue = 0.016666;

	double currentTime = glfwGetTime();
	double accumulator = 0.0;

	const double MAX_ACCUMULATED_TIME = 1.0
;

	while (!glfwWindowShouldClose(window)) {
		double newTime = glfwGetTime();
		double frameTime = newTime - currentTime;
		if (frameTime > 0.15)
			frameTime = 0.15;
		currentTime = newTime;
		
		accumulator += frameTime;
		accumulator = clamp(accumulator, 0.0, MAX_ACCUMULATED_TIME);

		glfwPollEvents();
		glfwGetCursorPos(window, &xpos, &ypos);
		glfwGetFramebufferSize(window, &fb_width, &fb_height);
		glfwGetWindowSize(window, &width, &height);

		ImGuiIO& io = ImGui::GetIO(); 
		io.DisplaySize = ImVec2((float)width, (float)height);
		io.DisplayFramebufferScale = ImVec2(width > 0 ? ((float)fb_width / width) : 0, height > 0 ? ((float)fb_height / height) : 0);
		io.DeltaTime = (float)frameTime;	

		while (accumulator >= fixedTimeValue) {
			scene_1->UpdateScene((float)fixedTimeValue, (float)totalTime, (float)accumulator);
			totalTime += fixedTimeValue;
			accumulator -= fixedTimeValue;
		}
		
		DrawFrame();	
	}

	vkDeviceWaitIdle(world_device->device);
}

void PuffinEngine::CreateSemaphores() {
	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	ErrorCheck(vkCreateSemaphore(world_device->device, &semaphore_info, nullptr, &imageAvailableSemaphore));
	ErrorCheck(vkCreateSemaphore(world_device->device, &semaphore_info, nullptr, &renderFinishedSemaphore));
	ErrorCheck(vkCreateSemaphore(world_device->device, &semaphore_info, nullptr, &reflectRenderSemaphore));
	ErrorCheck(vkCreateSemaphore(world_device->device, &semaphore_info, nullptr, &refractRenderSemaphore));
}

void PuffinEngine::DrawFrame() {
	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(world_device->device, world_device->swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)	{
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
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.waitSemaphoreCount = 1;
	submit_info.signalSemaphoreCount = 1;
	
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &scene_1->reflectionCmdBuff;
	submit_info.pWaitSemaphores = &imageAvailableSemaphore;
	submit_info.pSignalSemaphores = &reflectRenderSemaphore;
	ErrorCheck(vkQueueSubmit(world_device->queue, 1, &submit_info, VK_NULL_HANDLE));

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &scene_1->refractionCmdBuff;
	submit_info.pWaitSemaphores = &reflectRenderSemaphore;
	submit_info.pSignalSemaphores = &refractRenderSemaphore;
	ErrorCheck(vkQueueSubmit(world_device->queue, 1, &submit_info, VK_NULL_HANDLE));
	
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &scene_1->command_buffers[image_index];
	submit_info.pWaitSemaphores = &refractRenderSemaphore;
	submit_info.pSignalSemaphores = &renderFinishedSemaphore;
	ErrorCheck(vkQueueSubmit(world_device->queue, 1, &submit_info, VK_NULL_HANDLE));

	guiMainHub->Submit(world_device->queue, image_index);
	
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &renderFinishedSemaphore;

	VkSwapchainKHR swap_chains[] = { world_device->swapchain };
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

void PuffinEngine::RecreateSwapChain() {
	glfwGetWindowSize(window, &width, &height);
	if (width == 0 || height == 0) return;

	vkDeviceWaitIdle(world_device->device);

	CleanUpSwapChain();

	world_device->InitSwapChain();
	scene_1->RecreateForSwapchain();
	guiMainHub->RecreateForSwapchain();
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

	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	//io.Fonts->AddFontFromFileTTF("puffinEngine/fonts/exo-2/Exo2-SemiBold.otf", 16.0f);

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
	app->scene_1->currentCamera->MouseMove(xpos, ypos, app->width, app->height, 0.005f);
	app->mousePicker->CalculateNormalisedDeviceCoordinates(xpos, ypos);
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
	app->PressKey(key);
	
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
	PuffinEngine* app = reinterpret_cast<PuffinEngine*>(glfwGetWindowUserPointer(window));
	ImGuiIO& io = ImGui::GetIO();


	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		app->scene_1->DeSelect();
#if BUILD_ENABLE_VULKAN_DEBUG
		std::cout << "You clicked right mouse button" << std::endl;
#endif
	}
	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		app->scene_1->HandleMouseClick();
#if BUILD_ENABLE_VULKAN_DEBUG
		std::cout << "You clicked left mouse button" << std::endl;
#endif
	}
			
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
		std::cout << "You clicked middle mouse button" << std::endl;


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

void PuffinEngine::InitDefaultKeysBindings(std::map<int, FuncPair>& functions ) {	
	FuncPair moveForward = {&Scene::MoveCameraForward, &Scene::StopCameraForwardBackward};
	FuncPair moveBackward = {&Scene::MoveCameraBackward, &Scene::StopCameraForwardBackward};
	FuncPair moveLeft = {&Scene::MoveCameraLeft, &Scene::StopCameraLeftRight};
	FuncPair moveRight = {&Scene::MoveCameraRight, &Scene::StopCameraLeftRight};
	FuncPair moveUp = {&Scene::MoveCameraUp, &Scene::StopCameraUpDown};
	FuncPair moveDown = {&Scene::MoveCameraDown, &Scene::StopCameraUpDown};

	FuncPair makeMainCharacterJump = {&Scene::MakeMainCharacterJump, nullptr};
	FuncPair moveMainCharacterForward = {&Scene::MoveMainCharacterForward, &Scene::StopMainCharacter};
	FuncPair moveMainCharacterBackward = {&Scene::MoveMainCharacterBackward, &Scene::StopMainCharacter};
	FuncPair moveMainCharacterLeft = {&Scene::MoveMainCharacterLeft, &Scene::StopMainCharacter};
	FuncPair moveMainCharacterRight = {&Scene::MoveMainCharacterRight, &Scene::StopMainCharacter};
	
	FuncPair moveSelectedActorForward = {&Scene::MoveSelectedActorForward, &Scene::StopSelectedActorForwardBackward};
	FuncPair moveSelectedActorBackward = {&Scene::MoveSelectedActorBackward, &Scene::StopSelectedActorForwardBackward};
	FuncPair moveSelectedActorLeft = {&Scene::MoveSelectedActorLeft, &Scene::StopSelectedActorLeftRight};
	FuncPair moveSelectedActorRight = {&Scene::MoveSelectedActorRight, &Scene::StopSelectedActorLeftRight};
	FuncPair moveSelectedActorUp = {&Scene::MoveSelectedActorUp, &Scene::StopSelectedActorUpDown};
	FuncPair moveSelectedActorDown = {&Scene::MoveSelectedActorDown, &Scene::StopSelectedActorUpDown};
		
	FuncPair allGuiToggle = {&Scene::AllGuiToggle, nullptr};
	FuncPair SelectionIndicatorToggle = {&Scene::SelectionIndicatorToggle, nullptr};
	FuncPair WireframeToggle = {&Scene::WireframeToggle, nullptr};
	FuncPair AabbToggle = {&Scene::AabbToggle, nullptr};
	FuncPair ConsoleToggle = {&Scene::ConsoleToggle, nullptr};
	FuncPair MainUiToggle = {&Scene::MainUiToggle, nullptr};
	FuncPair TextOverlayToggle = {&Scene::TextOverlayToggle, nullptr};

	functions = {
		{GLFW_KEY_SPACE, makeMainCharacterJump},
		{GLFW_KEY_1, allGuiToggle},
		{GLFW_KEY_2, TextOverlayToggle},
		{GLFW_KEY_3, ConsoleToggle},
		{GLFW_KEY_4, MainUiToggle},
		{GLFW_KEY_A, moveLeft},
		{GLFW_KEY_B, AabbToggle},
		{GLFW_KEY_D, moveRight},
		{GLFW_KEY_E, moveDown},
		{GLFW_KEY_I, moveSelectedActorForward},
		{GLFW_KEY_J, moveSelectedActorLeft},
		{GLFW_KEY_K, moveSelectedActorBackward},
		{GLFW_KEY_L, moveSelectedActorRight},
		{GLFW_KEY_O, moveSelectedActorDown},
		{GLFW_KEY_Q, moveUp},
		{GLFW_KEY_S, moveBackward},
		{GLFW_KEY_U, moveSelectedActorUp},
		{GLFW_KEY_W, moveForward},
		{GLFW_KEY_V, WireframeToggle},
		{GLFW_KEY_X, SelectionIndicatorToggle},
		{GLFW_KEY_RIGHT,moveMainCharacterRight},
		{GLFW_KEY_LEFT, moveMainCharacterLeft},
		{GLFW_KEY_DOWN, moveMainCharacterBackward},
		{GLFW_KEY_UP, moveMainCharacterForward},
	};
}

void PuffinEngine::PressKey(int key) {
	int state = glfwGetKey(window, key);
		
	if (functions.count(key)) {
		if (state == GLFW_PRESS) 
			functions[key].first(scene_1);
		if (state == GLFW_RELEASE && functions[key].second!=nullptr)  
			functions[key].second(scene_1);
	}
}

// void PuffinEngine::ConnectGamepad()
// {
// 	//double seconds = 0.0;
// 	//seconds = glfwGetTime();
// 	//std::cout << "Time passed since init: " << seconds << std::endl;

// 	int present = glfwJoystickPresent(GLFW_JOYSTICK_1);
// 	/*std::cout << "Joystick/Gamepad 1 status: " << present << std::endl;*/

// 	if (1 == present)
// 	{
// 		int axes_count;
// 		const float *axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
// 		//std::cout << "Joystick/Gamepad 1 axes avaliable: " << axes_count << std::endl;
// 		currentCamera->Truck(-axes[0] * 15.0f);
// 		currentCamera->Dolly(axes[1] * 15.0f);

// 		currentCamera->GamepadMove(-axes[2], axes[3], WIDTH, HEIGHT, 0.15f);

// 		/*std::cout << "Left Stick X Axis: " << axes[0] << std::endl;
// 		std::cout << "Left Stick Y Axis: " << axes[1] << std::endl;
// 		std::cout << "Right Stick X Axis: " << axes[2] << std::endl;
// 		std::cout << "Right Stick Y Axis: " << axes[3] << std::endl;
// 		std::cout << "Left Trigger/L2: " << axes[4] << std::endl;
// 		std::cout << "Right Trigger/R2: " << axes[5] << std::endl;*/

// 		int button_count;
// 		const unsigned char *buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &button_count);
// 		//std::cout << "Joystick/Gamepad 1 buttons avaliable: " << button_count << std::endl;


// 		if (GLFW_PRESS == buttons[0])
// 		{
// 			//std::cout << "X button pressed: "<< std::endl;
// 			//break;
// 		}
// 		else if (GLFW_RELEASE == buttons[0])
// 		{
// 			//std::cout << "X button released: "<< std::endl;
// 		}
// 		if (GLFW_PRESS == buttons[4])
// 		{
// 			std::cout << "Left bumper pressed: reset camera" << std::endl;
// 			currentCamera->ResetPosition();
// 			//break;
// 		}

// 		/*std::cout << "A button: " << buttons[0] << std::endl;
// 		std::cout << "B button: " << buttons[1] << std::endl;
// 		std::cout << "X button: " << buttons[2] << std::endl;
// 		std::cout << "Y button: " << buttons[3] << std::endl;
// 		std::cout << "Left bumper " << buttons[4] << std::endl;
// 		std::cout << "Right bumper " << buttons[5] << std::endl;
// 		std::cout << "Back" << buttons[6] << std::endl;
// 		std::cout << "Start" << buttons[7] << std::endl;
// 		std::cout << "Guide?" << buttons[8] << std::endl;
// 		std::cout << "Disconnect?" << buttons[9] << std::endl;
// 		std::cout << "Up" << buttons[10] << std::endl;
// 		std::cout << "Right" << buttons[11] << std::endl;
// 		std::cout << "Down" << buttons[12] << std::endl;
// 		std::cout << "Left" << buttons[13] << std::endl;*/

// 		const char *name = glfwGetJoystickName(GLFW_JOYSTICK_1);
// 		//std::cout << "Your Joystick/Gamepad 1 name: " << name << std::endl;
// 	}
// }

// ---------------- Deinitialisation ---------------- //

void PuffinEngine::CleanUp() {
	CleanUpSwapChain();

	ImGui::DestroyContext();
	DeInitSemaphores();
	DestroyMousePicker();
	DestroyScene();
	DestroyGUI();
	DestroyDevice();
	glfwDestroyWindow(window);
	glfwTerminate();
	
}

void PuffinEngine::CleanUpSwapChain() {
	scene_1->CleanUpForSwapchain();
	guiMainHub->CleanUpForSwapchain();
	world_device->DeInitSwapchainImageViews();
	world_device->DestroySwapchainKHR();
}

void PuffinEngine::DeInitSemaphores() {
	vkDestroySemaphore(world_device->device, renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(world_device->device, imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(world_device->device, reflectRenderSemaphore, nullptr);
	vkDestroySemaphore(world_device->device, refractRenderSemaphore, nullptr);
}

void PuffinEngine::DestroyDevice() {
	world_device->DeInitDevice();
	delete world_device;
	world_device = nullptr;
}

void PuffinEngine::DestroyScene() {// can't see destructor working
	scene_1->DeInitScene();
	delete scene_1;
	scene_1 = nullptr;
}

void PuffinEngine::DestroyGUI() {
	mainUi->DeInit();
	delete mainUi;
	mainUi = nullptr;
	guiStatistics->DeInit();
	delete guiStatistics;
	guiStatistics = nullptr;
	console->DeInitMenu();
	delete console;
	console = nullptr;
	guiMainHub->DeInit();
	delete guiMainHub;
	guiMainHub = nullptr;
}

void PuffinEngine::DestroyMousePicker() {
	mousePicker->DeInit();
	delete mousePicker;
	mousePicker = nullptr;
}