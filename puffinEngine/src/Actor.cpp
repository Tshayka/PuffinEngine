#include <fstream>
#include <iostream>

#include "Actor.hpp"

Actor::Actor(std::string name, std::string description, glm::vec3 position) {
	this->name = name;
	this->description = description;
	this->position = position;
	id = CreateId();

	// create save file
	std::cout << "Actor created\n";
}

Actor::~Actor() {
	std::cout << "Actor destroyed\n";
}

// --------------- Setters and getters -------------- //
std::string Actor::GetId() const {
	return id;
}

glm::vec3 Actor::GetPosition() const {
	return position;
}

// ---------------- Main functions ------------------ //

float Actor::Approach(float character_goal, float character_current, float dt) {
	float flDifference = character_goal - character_current;
	if (flDifference > dt) { return character_current + dt;}
	if (flDifference < -dt) { return character_current - dt;}
	return character_goal;
}

std::string Actor::CreateId(){return "TEMP_ID";}

void Actor::Dolly(float charVelocityGoal) {
	velocityGoal.x = charVelocityGoal;
}

void Actor::LoadFromFile(){}

void Actor::SaveToFile(){}

void Actor::ResetPosition() {
	position = glm::vec3(4.0f, 4.0f, 4.0f);
	velocityGoal = glm::vec3(0.0f, 0.0f, 0.0f);
	velocity = glm::vec3(0.0f, 0.0f, 0.0f);
}

void Actor::Strafe(float charVelocityGoal) {
	velocityGoal.z = charVelocityGoal;
}

void Actor::UpdatePosition(float dt) {
	// Smooth movement and edge case in approach, without is movement is const
	velocity.x = Approach(velocityGoal.x, velocity.x, dt * 80);
	velocity.z = Approach(velocityGoal.z, velocity.z, dt * 80);

	position = position + velocity * dt;
	velocity = velocity + gravity * dt;
	
	if (position.y < 0.0f)
		position.y = 0.0f;
}