#include <fstream>
#include <iostream>

#include "Actor.hpp"

// ------- Constructors and dectructors ------------- //

enum class BodySlots
{
	LeftHand, RightHand, Head, Neck, Chest, Belt1, Belt2, Finger1, Finger2, Cloak, Shoes
};

Actor::Actor()
{
	CreateId();
	// create save file
	std::cout << "Actor created\n";
}

Actor::~Actor()
{
	std::cout << "Actor removed\n";
}

// --------------- Setters and getters -------------- //

glm::vec3 Actor::GetActorPosition() const
{
	return actor_position;
}

// ---------------- Main functions ------------------ //

std::string Actor::CreateId(){return "TEMP_ID";}


// -------------- Moving across scene --------------- //

void Actor::InitCharacter(float pos_x, float pos_y, float pos_z,
	float veloc_x, float veloc_y, float veloc_z,
	float gravity_x, float gravity_y, float gravity_z,
	float veloc_goal_x, float veloc_goal_y, float veloc_goal_z)
{
	actor_position = glm::vec3(pos_x, pos_y, pos_z); // set position
	actor_veloc = glm::vec3(veloc_x, veloc_y, veloc_z); // set position
	actor_gravity = glm::vec3(gravity_x, gravity_y, gravity_z);
	actor_velocty_goal = glm::vec3(veloc_goal_x, veloc_goal_y, veloc_goal_z);
}

void Actor::LoadFromFile()
{}

void Actor::SaveToFile()
{}

void Actor::DollyCharacter(float char_velocity_goal)
{
	actor_velocty_goal.x = char_velocity_goal;
}

void Actor::StrafeCharacter(float char_velocity_goal)
{
	actor_velocty_goal.z = char_velocity_goal;
}

void Actor::UpdateCharacter(float dt)
{
	// Smooth movement and edge case in approach, without is movement is const
	actor_veloc.x = ActorApproach(actor_velocty_goal.x, actor_veloc.x, dt * 80);
	actor_veloc.z = ActorApproach(actor_velocty_goal.z, actor_veloc.z, dt * 80);

	actor_position = actor_position + actor_veloc * dt;
	actor_veloc = actor_veloc + actor_gravity * dt;
	
	if (actor_position.y < 0.0f)
		actor_position.y = 0.0f;
	
}

float Actor::ActorApproach(float character_goal, float character_current, float dt)
{
	float flDifference = character_goal - character_current;

	if (flDifference > dt) { return character_current + dt;
}
	if (flDifference < -dt) { return character_current - dt;
}

	return character_goal;
}

void Actor::ActorResetPosition()
{
	actor_position = glm::vec3(4.0f, 4.0f, 4.0f);
	actor_velocty_goal = glm::vec3(0.0f, 0.0f, 0.0f);
	actor_veloc = glm::vec3(0.0f, 0.0f, 0.0f);
}