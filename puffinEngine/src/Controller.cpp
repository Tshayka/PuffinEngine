#include <iostream>

#include "Controller.hpp"
#include "ErrorCheck.hpp"


// ------- Constructors and dectructors ------------- //

Controller::Controller() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Controller created\n";
#endif 
}

Controller::~Controller() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Controller destroyed\n";
#endif
    DeInit(); 
}


// ---------------- Main functions ------------------ //

void Controller::Init(GuiMainHub* guiMainHub, Scene* scene) {
	this->scene = scene;
    this->guiMainHub = guiMainHub;
}

void Controller::DeInit() {
    scene = nullptr;
    guiMainHub = nullptr;
}

// -------------------- Bindings -------------------- //

void Controller::TestButton() {scene->Test();}

void Controller::MoveCameraForward() {scene->currentCamera->Dolly(150.0f);}
void Controller::MoveCameraBackward() {scene->currentCamera->Dolly(-150.0f);}
void Controller::StopCameraForwardBackward() {scene->currentCamera->Dolly(0.0f);}
void Controller::MoveCameraLeft() {scene->currentCamera->Truck(150.0f);}
void Controller::MoveCameraRight() {scene->currentCamera->Truck(-150.0f);}
void Controller::StopCameraLeftRight() {scene->currentCamera->Truck(0.0f);}
void Controller::MoveCameraUp() {scene->currentCamera->Pedestal(150.0f);}
void Controller::MoveCameraDown() {scene->currentCamera->Pedestal(-150.0f);}
void Controller::StopCameraUpDown() {scene->currentCamera->Pedestal(0.0f);}

void Controller::MakeMainCharacterJump() {scene->mainCharacter->SetState(ActorState::Jump);}
void Controller::MakeMainCharacterRun() {scene->mainCharacter->SetState(ActorState::Run);}
void Controller::MoveMainCharacterForward() {scene->mainCharacter->SetState(ActorState::WalkForward);}
void Controller::MoveMainCharacterBackward() {scene->mainCharacter->SetState(ActorState::WalkBackward);}
void Controller::MoveMainCharacterLeft() {scene->mainCharacter->SetState(ActorState::WalkLeft);}
void Controller::MoveMainCharacterRight() {scene->mainCharacter->SetState(ActorState::WalkRight);}
void Controller::StopMainCharacter() {scene->mainCharacter->SetState(ActorState::Idle);}

void Controller::TakeItem() {dynamic_cast<MainCharacter*>(scene->mainCharacter.get())->FindCollidingItem();}

void Controller::MoveSelectedActorForward() {if (scene->selectedActor!=nullptr) {scene->selectedActor->onManualControl(); scene->selectedActor->Dolly(150.0f);}}
void Controller::MoveSelectedActorBackward() {if (scene->selectedActor!=nullptr) {scene->selectedActor->onManualControl(); scene->selectedActor->Dolly(-150.0f);}}
void Controller::StopSelectedActorForwardBackward() {if (scene->selectedActor!=nullptr) {scene->selectedActor->offManualControl(); scene->selectedActor->Dolly(0.0f);}}
void Controller::MoveSelectedActorLeft() {if (scene->selectedActor!=nullptr) {scene->selectedActor->onManualControl(); scene->selectedActor->Strafe(-150.0f);}}
void Controller::MoveSelectedActorRight() {if (scene->selectedActor!=nullptr) {scene->selectedActor->onManualControl(); scene->selectedActor->Strafe(150.0f); }}
void Controller::StopSelectedActorLeftRight() {if (scene->selectedActor!=nullptr) {scene->selectedActor->offManualControl(); scene->selectedActor->Strafe(0.0f);}}
void Controller::MoveSelectedActorUp() {if (scene->selectedActor!=nullptr) {scene->selectedActor->onManualControl(); scene->selectedActor->Pedestal(150.0f);}}
void Controller::MoveSelectedActorDown() {if (scene->selectedActor!=nullptr) {scene->selectedActor->onManualControl(); scene->selectedActor->Pedestal(-150.0f);}}
void Controller::StopSelectedActorUpDown() {if (scene->selectedActor!=nullptr) {scene->selectedActor->offManualControl(); scene->selectedActor->Strafe(0.0f);}}

void Controller::WireframeToggle() {scene->displayWireframe = !scene->displayWireframe;}
void Controller::AabbToggle() {scene->displayAabb = !scene->displayAabb;}
void Controller::SelectionIndicatorToggle() {scene->displaySelectionIndicator = !scene->displaySelectionIndicator;}
void Controller::TriggerAreaToggle() {scene->displayTriggerArea = !scene->displayTriggerArea;}

void Controller::ConsoleToggle() {guiMainHub->ui_settings.display_imgui = !guiMainHub->ui_settings.display_imgui;}
void Controller::AllGuiToggle() {guiMainHub->guiOverlayVisible = !guiMainHub->guiOverlayVisible;}
void Controller::MainUiToggle() {guiMainHub->ui_settings.display_main_ui = !guiMainHub->ui_settings.display_main_ui;}
void Controller::TextOverlayToggle() {guiMainHub->ui_settings.display_stats_overlay = !guiMainHub->ui_settings.display_stats_overlay;}