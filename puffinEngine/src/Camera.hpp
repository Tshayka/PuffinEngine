#pragma once

#include <glm/glm.hpp>

#include "Actor.hpp"

class Camera : public Actor {
public:
	Camera(std::string name, std::string description, glm::vec3 position);
	virtual ~Camera();

	ActorType GetType() override;

	void Init(float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float);
	void Dolly(float);
	void GamepadMove(double, double, int, int, float);
	void MouseMove(double, double, int, int, float);
	void Pedestal(float);
	void ResetPosition();
	void Truck(float);
	void UpdatePosition(float);

	glm::vec3 up;
	glm::vec3 view;

	float FOV;
	float clippingNear;
	float clippingFar;

private:
	glm::vec3 movement;
	glm::vec3 movementGoal;
	glm::vec3 velocity;
	glm::vec3 direction;
	glm::vec3 foward;
	
	float pitch;
	float yaw;

	ActorType type = ActorType::Camera;
};