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

// ---------------- Main functions ------------------ //

float Actor::Approach(float actorGoal, float actorCurrent, float dt) {
	float flDifference = actorGoal - actorCurrent;
	if (flDifference > dt) { return actorCurrent + dt;}
	if (flDifference < -dt) { return actorCurrent - dt;}
	return actorGoal;
}

std::string Actor::CreateId() {
	return "TEMP_ID";
}

void Actor::Dolly(float actorVelocGoal) {
	velocityGoal.x = actorVelocGoal;
}

void Actor::LoadFromFile( ){

}

void Actor::LoadModel() {

}

void Actor::SaveToFile() {

}

void Actor::ResetPosition() {
	position = glm::vec3(4.0f, 4.0f, 4.0f);
	velocityGoal = glm::vec3(0.0f, 0.0f, 0.0f);
	velocity = glm::vec3(0.0f, 0.0f, 0.0f);
}

void Actor::Strafe(float actorVelocGoal) {
	velocityGoal.z = actorVelocGoal;
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