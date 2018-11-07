#pragma once

#include <glm/glm.hpp>
#include "Actor.hpp"

class Light : public Actor {
public:
	Light(std::string name, std::string description, glm::vec3 position);
	virtual ~Light();

	ActorType GetType() = 0;
	glm::vec3 GetLightColor() const;
	void SetLightColor(glm::vec3 lightColor);
	
private:
	glm::vec3 lightColor;
};

class SphereLight : public Light {
public:
	SphereLight(std::string name, std::string description, glm::vec3 position);
	virtual ~SphereLight();

	ActorType GetType() override;

	ActorType type = ActorType::SphereLight;

};