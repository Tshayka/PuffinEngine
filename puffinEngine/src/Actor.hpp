#pragma once

#include <glm/glm.hpp>

enum class ActorType {
        SphereLight, RectangularLight, DomeLight, Character, Camera
};

class Actor {
public:
	Actor(std::string name, std::string description, glm::vec3 position);
	virtual ~Actor();

	// ---------------- Main functions ------------------ //

	std::string GetId() const;
	virtual ActorType GetType() = 0;
	
	float Approach(float, float, float);
	void Dolly(float);
   	void LoadFromFile();
	void LoadModel();
	void SaveToFile();
	void Pedestal(float);
	void Strafe(float);
	void ResetPosition();
	void UpdatePosition(float);
	
	std::string description;
	std::string name;
	glm::vec3 position;
	glm::vec3 initPosition;

	glm::vec3 gravity = glm::vec3(0.0f, -2.0f, 0.0f);
	glm::vec3 movement = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 movementGoal = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 velocity;
	
private:
	std::string id;
	std::string CreateId();
};