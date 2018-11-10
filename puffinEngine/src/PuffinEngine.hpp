#pragma once

#include "Device.hpp"
#include "Scene.hpp"
#include "StatusOverlay.hpp"

const int FRAMES_PER_SECOND = 25;
const int SKIP_TICKS = 1000 / FRAMES_PER_SECOND;
const int MAX_FRAMESKIP = 5;

class PuffinEngine {
    public:
    PuffinEngine();
	~PuffinEngine();

    void Run();

    std::string isUppercase(char l);

    int width = 800;
	int height = 600; // relative to the monitor and/or the window and are given in artificial units that do not necessarily correspond to real screen pixels

    private:
    // ----------- Callbacks of ImGui and GLFW ------------------- //
    GLFWwindow* window;
    static void CursorPositionCallback(GLFWwindow*, double, double);
	static void CharacterCallback(GLFWwindow*, unsigned int);
	static void FramebufferSizeCallback(GLFWwindow*, int, int);
	static void KeyCallback(GLFWwindow*, int, int, int, int);
	static void MouseButtonCallback(GLFWwindow*, int, int, int);
	static void onWindowResized(GLFWwindow*, int, int);
	static void CursorEnterCallback(GLFWwindow*, int);
	static void ScrollCallback(GLFWwindow*, double, double);
    static void ErrorCallback(int, const char*);

    // ---------------- Main functions ------------------ //
    void CreateDevice();
    void CreateImGuiMenu();
    void CreateScene();
    void CreateStatusOverlay();
    void CreateSemaphores();
    void DrawFrame();
    void InitVulkan();
    void InitWindow();
    void MainLoop();
    //void PollJoypad();
	void RecreateSwapChain();
    void UpdateCameras(float);

    Device* world_device = nullptr;
    Scene *scene_1 = nullptr;
    ImGuiMenu* console = nullptr;
    StatusOverlay *statusOverlay = nullptr;

	VkSemaphore ImageAvailableSemaphore;
    VkSemaphore RenderFinishedSemaphore;

    double xpos, ypos;
	int fb_width, fb_height; // framebuffer sizes are, in contrast to the window coordinates given in pixels in order to match Vulkans requirements for viewport.

    // ---------------- Deinitialisation ---------------- //
    void CleanUp();
    void CleanUpSwapChain();
    void DeInitSemaphores();
    void DestroyDevice();
    void DestroyScene();
    void DestroyGUI();
};