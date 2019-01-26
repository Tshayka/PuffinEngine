#pragma once

#include <glm/glm.hpp>

#include "MeshLayout.cpp"

enum class ActorType {
    Actor, Landscape, SphereLight, RectangularLight, Skybox, DomeLight, Character, Camera, Sea, Cloud
};

class Actor {
public:
	Actor(std::string name, std::string description, glm::vec3 position, ActorType type);
	virtual ~Actor();

	// ---------------- Main functions ------------------ //

	std::string GetId() const;
	virtual ActorType GetType();//const!
	
	float Approach(float, float, float);
	virtual glm::vec3 CalculateSelectionIndicatorColor() = 0;
	void ChangePosition();
	void CheckIfInTheDestination();
	void Dolly(float);
   	static void LoadFromFile(const std::string &filename, enginetool::ScenePart& mesh, std::vector<uint32_t>& indices, std::vector<enginetool::VertexLayout>& vertices);
	void offManualControl();
	void onManualControl();
	void SaveToFile();
	void SetPosition(glm::vec3 lightColor);
	void Pedestal(float);
	void Strafe(float);
	void ResetPosition();
	void Truck(float);
	void UpdateAABB();
	virtual void UpdatePosition(float)=0;
	
	enginetool::ScenePart mesh;
	std::vector<uint32_t> indices;
	std::vector<enginetool::VertexLayout> vertices;
	
	std::string name;
	glm::vec3 position;
	glm::vec3 initPosition;
	glm::vec3 destinationPoint;

	glm::vec3 direction;
	glm::vec3 foward;

	glm::vec3 movement = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 movementGoal = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 velocity = glm::vec3(30.0f, 30.0f, 30.0f);
	glm::vec3 freeFallVelocity = glm::vec3(0.0f, -340.0f, 0.0f);
	float groundLevel = -20.0f;

	enginetool::ScenePart::AABB currentAabb;
	
	bool manualControl = false;

private:
	std::string CreateId();

	std::string description;
	std::string id;
	ActorType type = ActorType::Actor;
};