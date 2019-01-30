#pragma once

#include <glm/glm.hpp>
#include "Actor.hpp"

class Light : public Actor {
public:
	Light(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors);
	virtual ~Light();

	glm::vec3 GetLightColor() const;
	void SetLightColor(glm::vec3 lightColor);

	void UpdatePosition(float) override;
	virtual glm::vec3 CalculateSelectionIndicatorColor() override;

	glm::vec3 lightColor = glm::vec3(255.0f, 255.0f, 255.0f); // 6000K	
};

class SphereLight : public Light {
public:
	SphereLight(std::string name, std::string description, glm::vec3 position, ActorType type, std::vector<std::shared_ptr<Actor>>& actors);
	virtual ~SphereLight();
};

class Skybox : public Light {
public:
	Skybox(std::string name, std::string description, glm::vec3 position, ActorType type, std::vector<std::shared_ptr<Actor>>& actors, float horizon);
	virtual ~Skybox();

	void CreateMesh();

	float horizon;
};