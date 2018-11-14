#pragma once

#include <glm/glm.hpp>
#include "Actor.hpp"

class Light : public Actor {
public:
	Light(std::string name, std::string description, glm::vec3 position);
	virtual ~Light();

	glm::vec3 GetLightColor() const;
	void SetLightColor(glm::vec3 lightColor);
	
private:
	glm::vec3 lightColor = glm::vec3(255.0f, 210.0f, 160.0f);
};

class SphereLight : public Light {
public:
	SphereLight(std::string name, std::string description, glm::vec3 position);
	virtual ~SphereLight();

	ActorType type = ActorType::SphereLight;
private:
	std::string albedoTexture = "puffinEngine/assets/textures/icons/lightbulbIcon.jpg";
};

class Skybox : public Light {
public:
	Skybox(std::string name, std::string description, glm::vec3 position);
	virtual ~Skybox();

	ActorType type = ActorType::Skybox;
private:
	std::string albedoTexture = "puffinEngine/assets/skybox/carCubemap.ktx";
};