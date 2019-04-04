#pragma once

#include <glm/glm.hpp>

#include "MeshLayout.cpp"
#include "MaterialLibrary.hpp"

enum class ActorType {
    Actor, Item, Landscape, SphereLight, RectangularLight, Skybox, DomeLight, Character, Camera, Sea, Cloud, MainCharacter, TriggerArea
};

enum class ActorState {
	Crouch, Fall, Idle, Jump, Lie, Run, WalkForward, WalkBackward, WalkLeft, WalkRight, Reflection
};

enum class CharacterType {
	MainCharacter
};

enum class ItemType {
	Sword, Helmet, Money
};

enum class TriggerAreaType {
	GiveQuest, Info
};

class Actor {
public:
	Actor(std::string name, std::string description, glm::vec3 position, ActorType type, std::vector<std::shared_ptr<Actor>>& actors);
	virtual ~Actor();

	// ---------------- Main functions ------------------ //

	std::string GetId() const;
	virtual ActorType GetType();//const!
	
	float Approach(float, float, float);
	virtual glm::vec3 CalculateSelectionIndicatorColor() = 0;
	void ChangePosition();
	void CheckCollisions();
	void CheckIfInTheDestination();
	float DetectGroundLevel();
	void Dolly(float);
	void offManualControl();
	void onManualControl();
	void SaveToFile();
	void SetPosition(glm::vec3 lightColor);
	void SetState(ActorState state);
	void Pedestal(float);
	void Strafe(float);
	void ResetPosition();
	void Truck(float);
	void UpdateAABB();
	virtual void UpdatePosition(float)=0;

	bool visible = true;
	bool collider = false;
	bool manualControl = false;
	bool inAir = false;
	
	ActorState state;
	
	enginetool::ScenePart* assignedMesh;
	enginetool::SceneMaterial* assignedMaterial;
	
	enginetool::ScenePart::AABB currentAabb;
	std::vector<std::shared_ptr<Actor>>* interactActors;
	
	std::string name;
	
	glm::vec3 position;
	glm::vec3 initPosition;
	glm::vec3 destinationPoint;
	glm::vec3 direction;
	glm::vec3 foward;
	glm::vec3 movement = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 movementGoal = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 velocity = glm::vec3(30.0f, 30.0f, 30.0f);
	glm::vec3 freeFallVelocity = glm::vec3(0.0f, -1340.0f, 0.0f);
	
	float walkVelocity = 150.0f;
	float groundLevel;

private:
	std::string CreateId();
	void StartCrouch();
	void StartFall();
	void StartIdle();
	void StartJump();
	void StartReflect();
	void StartWalkBackward();
	void StartWalkForward();
	void StartWalkLeft();
	void StartWalkRight();

	std::string description;
	std::string id;
	ActorType type = ActorType::Actor;
};