#pragma once

#include <utility>
#include <map>

#include "Device.hpp"
#include "RenderPass.hpp"
#include "Scene.hpp"
#include "GuiMainHub.hpp"

//const int FRAMES_PER_SECOND = 25;
//const int SKIP_TICKS = 1000 / FRAMES_PER_SECOND;
//const int MAX_FRAMESKIP = 5;

typedef std::pair<std::function<void(puffinengine::tool::Scene*)>, std::function<void(puffinengine::tool::Scene*)>> FuncPair;

class PuffinEngine {
public:
    PuffinEngine();
	~PuffinEngine();

    void run();

    int width = 800;
	int height = 600; // relative to the monitor and/or the window and are given in artificial units that do not necessarily correspond to real screen pixels

private:
     bool initAllSystems();
     bool initWindow();
     bool initDefaultKeysBindings(std::map<int, FuncPair>& functions);
     void createWorldClock();

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
    
    

    //void ConnectGamepad();
    void initDevice();
    void CreateGuiTextOverlay();
    void CreateImGuiMenu();
    void CreateMainCharacter();
    void CreateMainUi();
    void CreateMousePicker();
    void CreateScene();
    void CreateGuiMainHub();
    void CreateMaterialLibrary();
    void CreateMeshLibrary();
    void CreateSemaphores();

    void DrawFrame();
    void GatherThreadInfo();

    void mainLoop();
    void PressKey(int key);
	void RecreateSwapChain();
    void UpdateGui();
   
    std::map<int, FuncPair> functions;
    std::unique_ptr<Actor> mainCharacter;
    
    puffinengine::tool::Scene scene_1;
    Device m_Device;
    puffinengine::tool::RenderPass m_ScreenRenderPass;
    puffinengine::tool::RenderPass m_OffScreenRenderPass;
    puffinengine::tool::WorldClock m_MainClock;

    MaterialLibrary m_MaterialLibrary;
    MeshLibrary m_MeshLibrary;
    MousePicker m_MousePicker;

    GuiMainHub m_GUIMainHub;
    GuiElement* console = nullptr;
    GuiMainUi* mainUi = nullptr;
    GuiTextOverlay* guiStatistics = nullptr;

	VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkSemaphore reflectRenderSemaphore;
    VkSemaphore refractRenderSemaphore;

    uint32_t numThreads;
    enginetool::ThreadPool m_ThreadPool;

    double xpos, ypos;
	int fb_width, fb_height; // framebuffer sizes are, in contrast to the window coordinates given in pixels in order to match Vulkans requirements for viewport.

    // ---------------- Deinitialisation ---------------- //

    void cleanUp();
    void CleanUpSwapChain();
    void DestroyGUI();
    void DeInitSemaphores();
    void DestroyMaterialLibrary();
    void destroyMeshLibrary();
    void deinitMousePicker();
    void DestroyScene();
    void deinitWorldClock();
};