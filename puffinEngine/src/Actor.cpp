#include <fstream>
#include <iostream>

#include "Actor.hpp"

Actor::Actor(std::string name, std::string description, glm::vec3 position) {
	this->name = name;
	this->description = description;
	this->position = position;
	initPosition = position;
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

ActorType Actor::GetType() {
    return type;
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

void Actor::LoadFromFile( ){

}

void Actor::LoadModel() {

}

void Actor::SaveToFile() {

}

void Actor::SetPosition(glm::vec3 position) {
	this->position = position;
}

void Actor::ResetPosition() {
	position = initPosition;
	movement = glm::vec3(0.0f, 0.0f, 0.0f);
	movementGoal = glm::vec3(0.0f, 0.0f, 0.0f);
}

void Actor::Dolly(float actorVelocityGoal) {
	movementGoal.x = actorVelocityGoal;
}

void Actor::Pedestal(float actorVelocityGoal) {
	movementGoal.y = actorVelocityGoal;
}

void Actor::Strafe(float actorVelocityGoal) {
	movementGoal.z = actorVelocityGoal;
}

void Actor::Truck(float actorVelocityGoal) {// change to function pointer
	Strafe(actorVelocityGoal);
}

void Actor::UpdateAABB(){
	currentAabb.max = mesh.aabb.min + position;
	currentAabb.min = mesh.aabb.max + position;
}
