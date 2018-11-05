#pragma once

#include <glm/glm.hpp>

class Actor
{
public:

	Actor();
	~Actor();

	// ---------------- Main functions ------------------ //

	std::string CreateId();
	void LoadFromFile();
	void SaveToFile();

	std::string actor_id;

	// -------------- Moving across scene --------------- //

	float ActorApproach(float, float, float);
	void DollyCharacter(float);
	void InitCharacter(float, float, float, float, float, float, float, float, float, float, float, float);
	void ActorResetPosition();
	void StrafeCharacter(float);
	void UpdateCharacter(float);
	
	glm::vec3 GetActorPosition() const;

	glm::vec3 actor_position;
	glm::vec3 actor_velocty_goal;
	glm::vec3 actor_veloc;
	glm::vec3 actor_gravity;

	std::string description;
	std::string id;
	std::string name;
	unsigned int max_health;
	int current_health;
	unsigned int gold;

};