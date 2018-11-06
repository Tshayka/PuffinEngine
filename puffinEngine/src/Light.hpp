#pragma once

#include <glm/glm.hpp>
#include "Actor.hpp"

class Light : public Actor {
public:
	Light(std::string name, std::string description, glm::vec3 position);
	virtual ~Light();

	virtual ActorType GetType() = 0;
	glm::vec3 GetLightColor() const;
	glm::vec4 GetLightPosition() const;
	void SetLightColor(glm::vec3 lightColor);
	void SetLightPosition(glm::vec4 lightPosition);

	float alpha = 0.0f;
	glm::vec3 lightColor;
	glm::vec4 lightPosition;
};

class SphereLight : public Light {
public:
	SphereLight(std::string name, std::string description, glm::vec3 position);
	virtual ~SphereLight();

	ActorType GetType() override;

	ActorType type = ActorType::SphereLight;

};