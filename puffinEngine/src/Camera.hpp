#pragma once

#include <glm/glm.hpp>

#include "Actor.hpp"

class Camera : public Actor {
public:
	Camera(std::string name, std::string description, glm::vec3 position);
	virtual ~Camera();

	ActorType GetType() override;
	void UpdatePosition(float) override;

	void Init(float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float);
	void GamepadMove(double, double, int, int, float);
	void MouseMove(double, double, int, int, float);
	void ResetPosition();
	

	glm::vec3 up;
	glm::vec3 view;

	float FOV;
	float clippingNear;
	float clippingFar;

private:
	std::string albedoTexture = "puffinEngine/assets/textures/icons/cameraIcon.jpg";
	
	float pitch;
	float yaw;

	ActorType type = ActorType::Camera;
};