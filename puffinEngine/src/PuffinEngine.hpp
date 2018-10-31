#pragma once

#include "Device.hpp"
#include "StatusOverlay.hpp"

class PuffinEngine{
    public:
    PuffinEngine();
	~PuffinEngine();

    void InitWindow();
    void Run();

    std::string isUppercase(char l);

    private:
    std::shared_ptr<ImGuiMenu> console = nullptr;
    std::shared_ptr<Device> world_device = nullptr;
    GLFWwindow* window;

    StatusOverlay *statusOverlay = nullptr;

    void CreateImGuiMenu();
    void CreateStatusOverlay();
    void CleanUp();
    void CleanUpSwapChain();
    void CreateDevice();
    void DestroyDevice();
    void InitVulkan();
    void MainLoop();
};