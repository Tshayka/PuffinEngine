#pragma once

#include <glm/glm.hpp>

enum class ActorType {
        Light, Character, Camera
};

class Actor {
public:
	Actor(std::string name, std::string description, glm::vec3 position);
	~Actor();

	// ---------------- Main functions ------------------ //

	std::string GetId() const;
	glm::vec3 GetPosition() const;
	virtual ActorType GetType() = 0;
	
	float Approach(float, float, float);
	std::string CreateId();
	void Dolly(float);
   	void LoadFromFile();
	void SaveToFile();
	void Strafe(float);
	void ResetPosition();
	void UpdatePosition(float);
	

	std::string description;
	std::string id;
	std::string name;
	glm::vec3 position;

	glm::vec3 gravity = glm::vec3(0.0f, -2.0f, 0.0f);
	glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 velocityGoal = glm::vec3(0.0f, 0.0f, 0.0f);
	

private:
	
};