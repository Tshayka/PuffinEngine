#pragma once

#include <glm/glm.hpp>
#include "Actor.hpp"

class Light : public Actor {
public:
	Light(std::string name, std::string description, glm::vec3 position);
	virtual ~Light();

	void UpdatePosition(float) override;

	glm::vec3 GetLightColor() const;
	void SetLightColor(glm::vec3 lightColor);
	glm::vec3 lightColor = glm::vec3(255.0f, 255.0f, 255.0f); // 6000K	
};

class SphereLight : public Light {
public:
	SphereLight(std::string name, std::string description, glm::vec3 position);
	virtual ~SphereLight();
	ActorType type = ActorType::SphereLight;
};

class Skybox : public Light {
public:
	Skybox(std::string name, std::string description, glm::vec3 position);
	virtual ~Skybox();
	ActorType type = ActorType::Skybox;
};