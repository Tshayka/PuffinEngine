#pragma once

#include "GuiMainHub.hpp"
#include "Scene.hpp"

class Controller {
public:
	Controller();
	~Controller();
    
    void DeInit();
	void Init(GuiMainHub* guiMainHub, Scene* scene);

    void TestButton();

	void MoveCameraForward();
	void MoveCameraBackward();
	void StopCameraForwardBackward();
	void MoveCameraLeft();
	void MoveCameraRight();
	void StopCameraLeftRight();
	void MoveCameraUp();
	void MoveCameraDown();
	void StopCameraUpDown();

	void MakeMainCharacterJump();
	void MakeMainCharacterRun();
	void MoveMainCharacterForward();
	void MoveMainCharacterBackward();
	void MoveMainCharacterLeft();
	void MoveMainCharacterRight();
	void StopMainCharacter();

	void TakeItem();

	void MakeSelectedActorJump();
	void MoveSelectedActorForward();
	void MoveSelectedActorBackward();
	void StopSelectedActorForwardBackward();
	void MoveSelectedActorLeft();
	void MoveSelectedActorRight();
	void StopSelectedActorLeftRight();
	void MoveSelectedActorUp();
	void MoveSelectedActorDown();
	void StopSelectedActorUpDown();
	
	void SelectionIndicatorToggle();
	void WireframeToggle();
	void AabbToggle();
	void ConsoleToggle();
	void AllGuiToggle();
	void MainUiToggle();
	void TextOverlayToggle();
	void TriggerAreaToggle();

    Scene* scene = nullptr;
    GuiMainHub* guiMainHub = nullptr;
};