#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "PuffinEngine.hpp"

 //---------- Constructors and dectructors ---------- //

PuffinEngine::PuffinEngine() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Puffin Engine created\n";
#endif 
}

PuffinEngine::~PuffinEngine() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Puffin Engine destroyed\n";
#endif
}

// std::string PuffinEngine::isUppercase(char l){
// if(l & 0b00100000)
//     return "This letter is lowercase!";
// else
//     return "This letter is uppercase!";
// }

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
	worldDevice = new Device();
	worldDevice->InitDevice(window);
}

void PuffinEngine::CreateWorldClock() {
	mainClock = new WorldClock();
}

void PuffinEngine::GatherThreadInfo() {
	numThreads = std::thread::hardware_concurrency();
#if BUILD_ENABLE_VULKAN_DEBUG 
	std::cout << "numThreads = " << numThreads << std::endl;
#endif 
	threadPool.SetThreadCount(numThreads);		
}

void PuffinEngine::CreateMaterialLibrary() {
	materialLibrary = new MaterialLibrary();
	materialLibrary->Init(worldDevice);
}

void PuffinEngine::CreateMeshLibrary() {
	meshLibrary = new MeshLibrary();
	meshLibrary->Init(worldDevice);
}

void PuffinEngine::CreateImGuiMenu() {
	console = new GuiElement();
}

void PuffinEngine::CreateGuiTextOverlay() {
	guiStatistics = new GuiTextOverlay();
}

void PuffinEngine::CreateMainUi() {
	mainUi = new GuiMainUi();
}

void PuffinEngine::CreateMousePicker() {
	mousePicker = new MousePicker();
	mousePicker->Init();
}

void PuffinEngine::CreateMainCharacter() {
	//mainCharacter = std::make_unique<MainCharacter>("Temp", "Brave hero", glm::vec3(0.0f, 0.0f, 0.0f), ActorType::MainCharacter);
	//dynamic_cast<MainCharacter*>(mainCharacter.get())->Init(1000, 1000, 100);
}

void PuffinEngine::CreateScene() {
	scene_1 = new Scene();
	scene_1->InitScene(worldDevice, mousePicker, meshLibrary, materialLibrary, mainClock, threadPool);
}

void PuffinEngine::CreateGuiMainHub() {
	guiMainHub = new GuiMainHub();
	guiMainHub->Init(worldDevice, console, guiStatistics, mainUi, mainClock, threadPool, materialLibrary);
}

void PuffinEngine::CreateController() {
	controller.Init(guiMainHub, scene_1);
}

void PuffinEngine::InitVulkan() {
	CreateDevice();
	CreateWorldClock();
	GatherThreadInfo();
	CreateImGuiMenu();
	CreateGuiTextOverlay();
	CreateMainUi();
	CreateMousePicker();
	CreateMaterialLibrary();
	CreateMeshLibrary();
	CreateScene();
	CreateGuiMainHub();
	CreateController();
	CreateSemaphores();
}

void PuffinEngine::MainLoop() {
	mainClock->currentTime = glfwGetTime();
	
	const double MAX_ACCUMULATED_TIME = 1.0;

	while (!glfwWindowShouldClose(window)) {
		double newTime = glfwGetTime();
		double frameTime = newTime - mainClock->currentTime;
		if (frameTime > 0.15)
			frameTime = 0.15;
		mainClock->currentTime = newTime;
		
		mainClock->accumulator += frameTime;
		mainClock->accumulator = clamp(mainClock->accumulator, 0.0, MAX_ACCUMULATED_TIME);

		glfwPollEvents();
		glfwGetCursorPos(window, &xpos, &ypos);
		glfwGetFramebufferSize(window, &fb_width, &fb_height);
		glfwGetWindowSize(window, &width, &height);

		ImGuiIO& io = ImGui::GetIO(); 
		io.DisplaySize = ImVec2((float)width, (float)height);
		io.DisplayFramebufferScale = ImVec2(width > 0 ? ((float)fb_width / width) : 0, height > 0 ? ((float)fb_height / height) : 0);
		io.DeltaTime = (float)frameTime;	

		while (mainClock->accumulator >= mainClock->fixedTimeValue) {
			threadPool.threads.back()->AddJob(std::bind(&PuffinEngine::UpdateGui, this));
			scene_1->UpdateScene();
			mainClock->totalTime += mainClock->fixedTimeValue;
			mainClock->accumulator -= mainClock->fixedTimeValue;
		}
		
		DrawFrame();	
	}

	vkDeviceWaitIdle(worldDevice->device);
}

void PuffinEngine::UpdateGui(){
	guiMainHub->UpdateGui();
}

void PuffinEngine::CreateSemaphores() {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	ErrorCheck(vkCreateSemaphore(worldDevice->device, &semaphoreInfo, nullptr, &imageAvailableSemaphore));
	ErrorCheck(vkCreateSemaphore(worldDevice->device, &semaphoreInfo, nullptr, &renderFinishedSemaphore));
	ErrorCheck(vkCreateSemaphore(worldDevice->device, &semaphoreInfo, nullptr, &reflectRenderSemaphore));
	ErrorCheck(vkCreateSemaphore(worldDevice->device, &semaphoreInfo, nullptr, &refractRenderSemaphore));
}

void PuffinEngine::DrawFrame() {
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(worldDevice->device, worldDevice->swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)	{
		RecreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		assert(0 && "Failed to acquire swap chain image!");
		std::exit(-1);
	}

	// Submitting the scene command buffer
	VkSubmitInfo sceneSubmitInfo = {}; // queue submission and synchronization is configured through parameters
	sceneSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	sceneSubmitInfo.pWaitDstStageMask = wait_stages;
	sceneSubmitInfo.waitSemaphoreCount = 1;
	sceneSubmitInfo.signalSemaphoreCount = 1;
	
	sceneSubmitInfo.commandBufferCount = 1;
	sceneSubmitInfo.pCommandBuffers = &scene_1->reflectionCmdBuff;
	sceneSubmitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	sceneSubmitInfo.pSignalSemaphores = &reflectRenderSemaphore;
	ErrorCheck(vkQueueSubmit(worldDevice->queue, 1, &sceneSubmitInfo, VK_NULL_HANDLE));

	sceneSubmitInfo.commandBufferCount = 1;
	sceneSubmitInfo.pCommandBuffers = &scene_1->refractionCmdBuff;
	sceneSubmitInfo.pWaitSemaphores = &reflectRenderSemaphore;
	sceneSubmitInfo.pSignalSemaphores = &refractRenderSemaphore;
	ErrorCheck(vkQueueSubmit(worldDevice->queue, 1, &sceneSubmitInfo, VK_NULL_HANDLE));
	
	sceneSubmitInfo.commandBufferCount = 1;
	sceneSubmitInfo.pCommandBuffers = &scene_1->commandBuffers[imageIndex];
	sceneSubmitInfo.pWaitSemaphores = &refractRenderSemaphore;
	sceneSubmitInfo.pSignalSemaphores = &renderFinishedSemaphore;
	ErrorCheck(vkQueueSubmit(worldDevice->queue, 1, &sceneSubmitInfo, VK_NULL_HANDLE));

	ErrorCheck(vkQueueWaitIdle(worldDevice->queue));
	
	// Submitting the gui command buffer
	if (guiMainHub->guiOverlayVisible) {
		VkSubmitInfo guiSubmitInfo = {};
		guiSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		guiSubmitInfo.commandBufferCount = 1;
		guiSubmitInfo.pCommandBuffers = &guiMainHub->command_buffers[imageIndex];
				
		ErrorCheck(vkQueueSubmit(worldDevice->queue, 1, &guiSubmitInfo, VK_NULL_HANDLE));
	}
		
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphore;

	VkSwapchainKHR swapChains[] = { worldDevice->swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(worldDevice->present_queue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		assert(0 && "Failed to present swap chain image!");
		std::exit(-1);
	}

	vkQueueWaitIdle(worldDevice->present_queue);
}

void PuffinEngine::RecreateSwapChain() {
	glfwGetWindowSize(window, &width, &height);
	if (width == 0 || height == 0) return;

	vkDeviceWaitIdle(worldDevice->device);

	CleanUpSwapChain();

	worldDevice->InitSwapChain();
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

	io.Fonts->AddFontFromFileTTF("puffinEngine/fonts/exo-2/Exo2-SemiBold.otf", 16.0f);

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
	FuncPair test = {&Controller::TestButton, nullptr};

	FuncPair moveForward = {&Controller::MoveCameraForward, &Controller::StopCameraForwardBackward};
	FuncPair moveBackward = {&Controller::MoveCameraBackward, &Controller::StopCameraForwardBackward};
	FuncPair moveLeft = {&Controller::MoveCameraLeft, &Controller::StopCameraLeftRight};
	FuncPair moveRight = {&Controller::MoveCameraRight, &Controller::StopCameraLeftRight};
	FuncPair moveUp = {&Controller::MoveCameraUp, &Controller::StopCameraUpDown};
	FuncPair moveDown = {&Controller::MoveCameraDown, &Controller::StopCameraUpDown};

	FuncPair makeMainCharacterJump = {&Controller::MakeMainCharacterJump, nullptr};
	FuncPair moveMainCharacterForward = {&Controller::MoveMainCharacterForward, &Controller::StopMainCharacter};
	FuncPair moveMainCharacterBackward = {&Controller::MoveMainCharacterBackward, &Controller::StopMainCharacter};
	FuncPair moveMainCharacterLeft = {&Controller::MoveMainCharacterLeft, &Controller::StopMainCharacter};
	FuncPair moveMainCharacterRight = {&Controller::MoveMainCharacterRight, &Controller::StopMainCharacter};

	FuncPair takeItem = {&Controller::TakeItem, nullptr};
	
	FuncPair moveSelectedActorForward = {&Controller::MoveSelectedActorForward, &Controller::StopSelectedActorForwardBackward};
	FuncPair moveSelectedActorBackward = {&Controller::MoveSelectedActorBackward, &Controller::StopSelectedActorForwardBackward};
	FuncPair moveSelectedActorLeft = {&Controller::MoveSelectedActorLeft, &Controller::StopSelectedActorLeftRight};
	FuncPair moveSelectedActorRight = {&Controller::MoveSelectedActorRight, &Controller::StopSelectedActorLeftRight};
	FuncPair moveSelectedActorUp = {&Controller::MoveSelectedActorUp, &Controller::StopSelectedActorUpDown};
	FuncPair moveSelectedActorDown = {&Controller::MoveSelectedActorDown, &Controller::StopSelectedActorUpDown};
		
	FuncPair allGuiToggle = {&Controller::AllGuiToggle, nullptr};
	FuncPair SelectionIndicatorToggle = {&Controller::SelectionIndicatorToggle, nullptr};
	FuncPair WireframeToggle = {&Controller::WireframeToggle, nullptr};
	FuncPair AabbToggle = {&Controller::AabbToggle, nullptr};
	FuncPair ConsoleToggle = {&Controller::ConsoleToggle, nullptr};
	FuncPair MainUiToggle = {&Controller::MainUiToggle, nullptr};
	FuncPair TextOverlayToggle = {&Controller::TextOverlayToggle, nullptr};
	FuncPair TriggerAreaToggle = {&Controller::TriggerAreaToggle, nullptr};

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
		{GLFW_KEY_F, takeItem},
		{GLFW_KEY_I, moveSelectedActorForward},
		{GLFW_KEY_J, moveSelectedActorLeft},
		{GLFW_KEY_K, moveSelectedActorBackward},
		{GLFW_KEY_L, moveSelectedActorRight},
		{GLFW_KEY_O, moveSelectedActorDown},
		{GLFW_KEY_Q, moveUp},
		{GLFW_KEY_S, moveBackward},
		{GLFW_KEY_T, test},
		{GLFW_KEY_U, moveSelectedActorUp},
		{GLFW_KEY_W, moveForward},
		{GLFW_KEY_V, WireframeToggle},
		{GLFW_KEY_X, SelectionIndicatorToggle},
		{GLFW_KEY_Z, TriggerAreaToggle},
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
			functions[key].first(&controller);
		if (state == GLFW_RELEASE && functions[key].second!=nullptr)  
			functions[key].second(&controller);
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
	DestroyWorldClock();
	DestroyMeshLibrary();
	DestroyMaterialLibrary();
	DestroyDevice();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void PuffinEngine::CleanUpSwapChain() {
	scene_1->CleanUpForSwapchain();
	guiMainHub->CleanUpForSwapchain();
	worldDevice->DeInitSwapchainImageViews();
	worldDevice->DestroySwapchainKHR();
}

void PuffinEngine::DeInitSemaphores() {
	vkDestroySemaphore(worldDevice->device, renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(worldDevice->device, imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(worldDevice->device, reflectRenderSemaphore, nullptr);
	vkDestroySemaphore(worldDevice->device, refractRenderSemaphore, nullptr);
}

void PuffinEngine::DestroyDevice() {
	worldDevice->DeInitDevice();
	delete worldDevice;
	worldDevice = nullptr;
}

void PuffinEngine::DestroyWorldClock() {
	delete mainClock;
	mainClock = nullptr;
}

void PuffinEngine::DestroyMaterialLibrary() {
	materialLibrary->DeInit();
	delete materialLibrary;
	materialLibrary = nullptr;
}

void PuffinEngine::DestroyMeshLibrary() {
	meshLibrary->DeInit();
	delete meshLibrary;
	meshLibrary = nullptr;
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

	console->DeInit();
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