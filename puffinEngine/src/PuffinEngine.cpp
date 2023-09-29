#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "headers/PuffinEngine.hpp"

using namespace puffinengine::tool;

 //---------- Constructors and dectructors ---------- //

PuffinEngine::PuffinEngine() {
#if DEBUG_VERSION
	std::cout << "Engine object created\n";
#endif 
}

PuffinEngine::~PuffinEngine() {
#if DEBUG_VERSION
	std::cout << "Engine object destroyed\n";
#endif
}

template<typename T>
T clamp(const T& what, const T& a, const T& b) {
return std::min(b, std::max(what, a)); 
}

// ---------------- Main functions ------------------ //

void PuffinEngine::run() {
	if (initAllSystems()) {
		mainLoop();
	}

    cleanUp();
}

bool PuffinEngine::initAllSystems() {
	if (!initWindow()) {
		return false;
	}

	if (!initDefaultKeysBindings(functions)) {
		return false;
	}

	initDevice();

	if (!m_ScreenRenderPass.init(&m_Device, RenderPass::Type::SCREEN)) {
		return false;
	}

	if (!m_OffScreenRenderPass.init(&m_Device, RenderPass::Type::OFFSCREEN)) {
		return false;
	}

	createWorldClock();
	GatherThreadInfo();
	CreateImGuiMenu();
	CreateGuiTextOverlay();
	CreateMainUi();
	CreateGuiMainHub();
	CreateMousePicker();
	CreateMaterialLibrary();
	CreateMeshLibrary();
	CreateScene();
	CreateSemaphores();

	return true;
}

void PuffinEngine::initDevice() {
	m_Device.init(window);
}

void PuffinEngine::createWorldClock() {
	//
}

void PuffinEngine::GatherThreadInfo() {
	numThreads = std::thread::hardware_concurrency();
	std::cout << "numThreads = " << numThreads << std::endl;
	m_ThreadPool.SetThreadCount(numThreads);		
}

void PuffinEngine::CreateImGuiMenu() {
	p_Console = new GuiElement();
}

void PuffinEngine::CreateGuiTextOverlay() {
	p_GuiStatistics = new GuiTextOverlay();
}

void PuffinEngine::CreateMainUi() {
	p_MainUi = new GuiMainUi();
}

void PuffinEngine::CreateMousePicker() {
	m_MousePicker.init(&m_Device);
}

void PuffinEngine::CreateMaterialLibrary() {
	m_MaterialLibrary.Init(&m_Device);
}

void PuffinEngine::CreateMeshLibrary() {
	m_MeshLibrary.Init(&m_Device);
}

void PuffinEngine::CreateMainCharacter() {
	//mainCharacter = std::make_unique<MainCharacter>("Temp", "Brave hero", glm::vec3(0.0f, 0.0f, 0.0f), ActorType::MainCharacter);
	//dynamic_cast<MainCharacter*>(mainCharacter.get())->Init(1000, 1000, 100);
}

void PuffinEngine::CreateGuiMainHub() {
	m_GUIMainHub.init(&m_Device, &m_ScreenRenderPass, p_Console, p_GuiStatistics, p_MainUi, &m_MainClock, &m_ThreadPool);
}

void PuffinEngine::CreateScene() {
	scene_1.init(&m_Device, &m_ScreenRenderPass, &m_OffScreenRenderPass, &m_GUIMainHub, &m_MousePicker, &m_MeshLibrary, &m_MaterialLibrary, &m_MainClock, &m_ThreadPool);
}

void PuffinEngine::mainLoop() {
	const double maxAccumulatedTime = 1.0;
	m_MainClock.fixedTimeValue = 0.015;
	const double maxFrameTime = 0.15;
	double currentTime = glfwGetTime();
	double accumulator = 0.0;
	int frameCount = 0;
	const int frameHistorySize = 60;
	double frameTimes[frameHistorySize] = { 0.0 };
	int frameIndex = 0;

	while (!glfwWindowShouldClose(window)) {
		double newTime = glfwGetTime();
		double frameTime = newTime - currentTime;
		currentTime = newTime;

		// Limit frame time to avoid spiral of death
		if (frameTime > maxFrameTime)
			frameTime = maxFrameTime;

		accumulator += frameTime;

		glfwPollEvents();
		glfwGetCursorPos(window, &xpos, &ypos);
		glfwGetFramebufferSize(window, &fb_width, &fb_height);
		glfwGetWindowSize(window, &width, &height);

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
		io.DisplayFramebufferScale = ImVec2(width > 0 ? (static_cast<float>(fb_width) / width) : 0,
			height > 0 ? (static_cast<float>(fb_height) / height) : 0);
		io.DeltaTime = static_cast<float>(frameTime);

		while (accumulator >= m_MainClock.fixedTimeValue) {
			m_ThreadPool.threads.back()->AddJob(std::bind(&PuffinEngine::UpdateGui, this));
			scene_1.update();
			m_MainClock.totalElapsedTime += m_MainClock.fixedTimeValue;
			accumulator -= m_MainClock.fixedTimeValue;
		}

		DrawFrame();

		// Calculate average frame time and FPS
		// frameTimes - bumber of frames to consider for FPS calculation
		frameTimes[frameIndex] = frameTime;
		frameIndex = (frameIndex + 1) % frameHistorySize;
		double avgFrameTime = 0.0;
		for (int i = 0; i < frameHistorySize; i++) {
			avgFrameTime += frameTimes[i];
		}

		avgFrameTime /= frameHistorySize;
		m_MainClock.fps = 1.0 / avgFrameTime;
		m_MainClock.frameTime = frameTime;
	}

	vkDeviceWaitIdle(m_Device.get());
}

void PuffinEngine::UpdateGui(){
	m_GUIMainHub.updateGui();
}

void PuffinEngine::CreateSemaphores() {
	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	ErrorCheck(vkCreateSemaphore(m_Device.get(), &semaphore_info, nullptr, &imageAvailableSemaphore));
	ErrorCheck(vkCreateSemaphore(m_Device.get(), &semaphore_info, nullptr, &renderFinishedSemaphore));
	ErrorCheck(vkCreateSemaphore(m_Device.get(), &semaphore_info, nullptr, &reflectRenderSemaphore));
	ErrorCheck(vkCreateSemaphore(m_Device.get(), &semaphore_info, nullptr, &refractRenderSemaphore));
}

void PuffinEngine::DrawFrame() {
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_Device.get(), m_Device.swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

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
	submit_info.pCommandBuffers = &scene_1.reflectionCmdBuff;
	submit_info.pWaitSemaphores = &imageAvailableSemaphore;
	submit_info.pSignalSemaphores = &reflectRenderSemaphore;
	ErrorCheck(vkQueueSubmit(m_Device.queue, 1, &submit_info, VK_NULL_HANDLE));

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &scene_1.refractionCmdBuff;
	submit_info.pWaitSemaphores = &reflectRenderSemaphore;
	submit_info.pSignalSemaphores = &refractRenderSemaphore;
	ErrorCheck(vkQueueSubmit(m_Device.queue, 1, &submit_info, VK_NULL_HANDLE));
	
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &scene_1.commandBuffers[imageIndex];
	submit_info.pWaitSemaphores = &refractRenderSemaphore;
	submit_info.pSignalSemaphores = &renderFinishedSemaphore;
	ErrorCheck(vkQueueSubmit(m_Device.queue, 1, &submit_info, VK_NULL_HANDLE));

	m_GUIMainHub.submit(m_Device.queue, imageIndex);
	
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &renderFinishedSemaphore;

	VkSwapchainKHR swapChains[] = { m_Device.swapchain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapChains;
	present_info.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(m_Device.present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		assert(0 && "Failed to present swap chain image!");
		std::exit(-1);
	}

	vkQueueWaitIdle(m_Device.present_queue);
}

void PuffinEngine::RecreateSwapChain() {
	glfwGetWindowSize(window, &width, &height);
	if (width == 0 || height == 0) { return; }

	vkDeviceWaitIdle(m_Device.get());

	CleanUpSwapChain();

	m_Device.InitSwapChain();
	scene_1.RecreateForSwapchain();
	m_GUIMainHub.recreateForSwapchain();
}


// ----------- Callbacks and GLFW ------------------- //

bool PuffinEngine::initWindow() {
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
		return false;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImFont* font1 = io.Fonts->AddFontDefault();
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

	return true;
}

void PuffinEngine::CharacterCallback(GLFWwindow* window, unsigned int codepoint) {
	ImGuiIO& io = ImGui::GetIO();
	if (codepoint > 0 && codepoint < 0x10000)
		io.AddInputCharacter((unsigned short)codepoint);
}

void PuffinEngine::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
	PuffinEngine* app = reinterpret_cast<PuffinEngine*>(glfwGetWindowUserPointer(window));
	app->scene_1.currentCamera->MouseMove(xpos, ypos, app->width, app->height, 0.005f);
	app->m_MousePicker.CalculateNormalisedDeviceCoordinates(xpos, ypos);
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2((float)xpos, (float)ypos);
}


void PuffinEngine::ErrorCallback(int error, const char* description) {
#if DEBUG_VERSION
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
		app->scene_1.DeSelect();
#if DEBUG_VERSION
		std::cout << "You clicked right mouse button" << std::endl;
#endif
	}
	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		app->scene_1.HandleMouseClick();
#if DEBUG_VERSION
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
#if DEBUG_VERSION
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

bool PuffinEngine::initDefaultKeysBindings(std::map<int, FuncPair>& functions ) {	
	FuncPair test = {&puffinengine::tool::Scene::Test, nullptr};

	FuncPair moveForward = {&puffinengine::tool::Scene::MoveCameraForward, &puffinengine::tool::Scene::StopCameraForwardBackward};
	FuncPair moveBackward = {&puffinengine::tool::Scene::MoveCameraBackward, &puffinengine::tool::Scene::StopCameraForwardBackward};
	FuncPair moveLeft = {&puffinengine::tool::Scene::MoveCameraLeft, &puffinengine::tool::Scene::StopCameraLeftRight};
	FuncPair moveRight = {&puffinengine::tool::Scene::MoveCameraRight, &puffinengine::tool::Scene::StopCameraLeftRight};
	FuncPair moveUp = {&puffinengine::tool::Scene::MoveCameraUp, &puffinengine::tool::Scene::StopCameraUpDown};
	FuncPair moveDown = {&puffinengine::tool::Scene::MoveCameraDown, &puffinengine::tool::Scene::StopCameraUpDown};

	FuncPair makeMainCharacterJump = {&puffinengine::tool::Scene::MakeMainCharacterJump, nullptr};
	FuncPair moveMainCharacterForward = {&puffinengine::tool::Scene::MoveMainCharacterForward, &puffinengine::tool::Scene::StopMainCharacter};
	FuncPair moveMainCharacterBackward = {&puffinengine::tool::Scene::MoveMainCharacterBackward, &puffinengine::tool::Scene::StopMainCharacter};
	FuncPair moveMainCharacterLeft = {&puffinengine::tool::Scene::MoveMainCharacterLeft, &puffinengine::tool::Scene::StopMainCharacter};
	FuncPair moveMainCharacterRight = {&puffinengine::tool::Scene::MoveMainCharacterRight, &puffinengine::tool::Scene::StopMainCharacter};
	
	FuncPair moveSelectedActorForward = {&puffinengine::tool::Scene::MoveSelectedActorForward, &puffinengine::tool::Scene::StopSelectedActorForwardBackward};
	FuncPair moveSelectedActorBackward = {&puffinengine::tool::Scene::MoveSelectedActorBackward, &puffinengine::tool::Scene::StopSelectedActorForwardBackward};
	FuncPair moveSelectedActorLeft = {&puffinengine::tool::Scene::MoveSelectedActorLeft, &puffinengine::tool::Scene::StopSelectedActorLeftRight};
	FuncPair moveSelectedActorRight = {&puffinengine::tool::Scene::MoveSelectedActorRight, &puffinengine::tool::Scene::StopSelectedActorLeftRight};
	FuncPair moveSelectedActorUp = {&puffinengine::tool::Scene::MoveSelectedActorUp, &puffinengine::tool::Scene::StopSelectedActorUpDown};
	FuncPair moveSelectedActorDown = {&puffinengine::tool::Scene::MoveSelectedActorDown, &puffinengine::tool::Scene::StopSelectedActorUpDown};
		
	FuncPair allGuiToggle = {&puffinengine::tool::Scene::AllGuiToggle, nullptr};
	FuncPair SelectionIndicatorToggle = {&puffinengine::tool::Scene::SelectionIndicatorToggle, nullptr};
	FuncPair WireframeToggle = {&puffinengine::tool::Scene::WireframeToggle, nullptr};
	FuncPair AabbToggle = {&puffinengine::tool::Scene::AabbToggle, nullptr};
	FuncPair ConsoleToggle = {&puffinengine::tool::Scene::ConsoleToggle, nullptr};
	FuncPair MainUiToggle = {&puffinengine::tool::Scene::MainUiToggle, nullptr};
	FuncPair TextOverlayToggle = {&puffinengine::tool::Scene::TextOverlayToggle, nullptr};

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
		{GLFW_KEY_T, test},
		{GLFW_KEY_U, moveSelectedActorUp},
		{GLFW_KEY_W, moveForward},
		{GLFW_KEY_V, WireframeToggle},
		{GLFW_KEY_X, SelectionIndicatorToggle},
		{GLFW_KEY_RIGHT,moveMainCharacterRight},
		{GLFW_KEY_LEFT, moveMainCharacterLeft},
		{GLFW_KEY_DOWN, moveMainCharacterBackward},
		{GLFW_KEY_UP, moveMainCharacterForward},
	};

	return true;
}

void PuffinEngine::PressKey(int key) {
	int state = glfwGetKey(window, key);
		
	if (functions.count(key)) {
		if (state == GLFW_PRESS) 
			functions[key].first(&scene_1);
		if (state == GLFW_RELEASE && functions[key].second!=nullptr)  
			functions[key].second(&scene_1);
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

void PuffinEngine::cleanUp() {
	CleanUpSwapChain();

	ImGui::DestroyContext();
	DeInitSemaphores();
	deinitMousePicker();
	DestroyScene();
	destroyMeshLibrary();
	DestroyMaterialLibrary();
	DestroyGUI();
	deinitWorldClock();

	if (m_ScreenRenderPass.m_Initialized) {
		m_ScreenRenderPass.deInit();
	}

	if (m_OffScreenRenderPass.m_Initialized) {
		m_OffScreenRenderPass.deInit();
	}

	m_Device.deInit();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void PuffinEngine::CleanUpSwapChain() {
	scene_1.CleanUpForSwapchain();
	m_GUIMainHub.cleanUpForSwapchain();
	m_Device.DeInitSwapchainImageViews();
	m_Device.DestroySwapchainKHR();
}

void PuffinEngine::DeInitSemaphores() {
	vkDestroySemaphore(m_Device.get(), renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(m_Device.get(), imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(m_Device.get(), reflectRenderSemaphore, nullptr);
	vkDestroySemaphore(m_Device.get(), refractRenderSemaphore, nullptr);
}

void PuffinEngine::deinitWorldClock() {
	//
}

void PuffinEngine::DestroyMaterialLibrary() {
	m_MaterialLibrary.DeInit();
}

void PuffinEngine::destroyMeshLibrary() {
	m_MeshLibrary.DeInit();
}

void PuffinEngine::DestroyScene() {// can't see destructor working
	scene_1.deinit();
}

void PuffinEngine::DestroyGUI() {
	p_MainUi->deInit();
	delete p_MainUi;
	p_MainUi = nullptr;

	p_GuiStatistics->deInit();
	delete p_GuiStatistics;
	p_GuiStatistics = nullptr;

	p_Console->deInit();
	delete p_Console;
	p_Console = nullptr;

	m_GUIMainHub.deinit();
}

void PuffinEngine::deinitMousePicker() {
	m_MousePicker.deinit();
}