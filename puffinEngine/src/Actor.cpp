#include <fstream>
#include <iostream>
#include <cmath>

#include "Actor.hpp"

Actor::Actor(std::string name, std::string description, glm::vec3 position, ActorType type) {
	this->name = name;
	this->description = description;
	this->position = position;
	this->type = type;
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

void Actor::ChangePosition() {
	glm::vec3 direction = destinationPoint - position;
	direction = glm::normalize(direction);
	movementGoal.x = direction.x * velocity.x;
	movementGoal.y = direction.y * velocity.y;
	movementGoal.z = direction.z * velocity.z;
}

void Actor::CheckIfInTheDestination() {
	float distance = glm::distance(position, destinationPoint);
	
	if(distance<0.1) 
		movementGoal=glm::vec3(0.0f, 0.0f, 0.0f);
	else 
		ChangePosition();
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
	destinationPoint = position;
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

void Actor::offManualControl() {
	manualControl = false;
	destinationPoint = position;
}

void Actor::onManualControl() {
	manualControl = true;
	destinationPoint = position;
}
