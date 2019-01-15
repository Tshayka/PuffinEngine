#pragma once

#include <glm/glm.hpp>

#include "LoadMesh.cpp"

enum class ActorType {
    Actor, Landscape, SphereLight, RectangularLight, Skybox, DomeLight, Character, Camera
};

class Actor {
public:
	Actor(std::string name, std::string description, glm::vec3 position, ActorType type);
	virtual ~Actor();

	// ---------------- Main functions ------------------ //

	std::string GetId() const;
	virtual ActorType GetType();
	
	float Approach(float, float, float);
	void Dolly(float);
   	void LoadFromFile();
	void LoadModel();
	void SaveToFile();
	void SetPosition(glm::vec3 lightColor);
	void Pedestal(float);
	void Strafe(float);
	void ResetPosition();
	void Truck(float);
	void UpdateAABB();
	virtual void UpdatePosition(float)=0;
	
	enginetool::ScenePart mesh;
	
	std::string name;
	glm::vec3 position;
	glm::vec3 initPosition;

	glm::vec3 direction;
	glm::vec3 foward;

	glm::vec3 movement = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 movementGoal = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 velocity;

	enginetool::ScenePart::AABB currentAabb;
	
private:
	std::string CreateId();

	std::string description;
	std::string id;
	ActorType type = ActorType::Actor;
};