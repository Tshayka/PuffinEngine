#include <fstream>
#include <iostream>
#include <cmath>

#define BOOST_PENDING_INTEGER_LOG2_HPP
#include <boost/integer/integer_log2.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include "Actor.hpp"

Actor::Actor(std::string name, std::string description, glm::vec3 position, ActorType type, std::vector<std::shared_ptr<Actor>>& actors) : state(ActorState::Idle) {
	this->name = name;
	this->description = description;
	this->position = position;
	this->type = type;
	interactActors = &actors;
	initPosition = position;
	groundLevel = position.y;
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
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	std::string id = boost::uuids::to_string(uuid);
	return id;
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

float Actor::DetectGroundLevel() {
	glm::vec3 rayDirection = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
	glm::vec3 dirFrac;
	dirFrac.x = 1.0f / rayDirection.x;
	dirFrac.y = 1.0f / rayDirection.y;
	dirFrac.z = 1.0f / rayDirection.z;
	glm::vec3 hitPoint;
	float groundLevel = -20.0f;

	std::vector<std::shared_ptr<Actor>>::iterator it;

	for (it = interactActors->begin(); it!=interactActors->end() ;++it) {
		if(enginetool::ScenePart::RayIntersection(hitPoint, dirFrac, position, rayDirection, it->get()->currentAabb) && it->get()!=this) {
			//std::cout << "Hit point: " << hitPoint.x << " " << hitPoint.y << " " << hitPoint.z << "\n";
			if(hitPoint.y > groundLevel) groundLevel = hitPoint.y;
		}
	}
	//std::cout << "Ground is at: " << groundLevel << std::endl;
	return groundLevel;
}

void Actor::CheckCollisions() {
	std::vector<std::shared_ptr<Actor>>::iterator it;
	for (it = interactActors->begin(); it!=interactActors->end() ;++it) {
		if(enginetool::ScenePart::Overlaps(this->currentAabb, it->get()->currentAabb) && it->get()!=this) {
			std::cout << this->name << " collides with: " << it->get()->name << "\n";
			SetState(ActorState::Reflection);		
		}
	}
}

void Actor::SaveToFile() {

}

void Actor::SetPosition(glm::vec3 position) {
	this->position = position;
}

void Actor::SetState(ActorState state) {
	if (this->state==state || inAir) return;

	this->state=state;

	switch(this->state){
		case ActorState::Idle:
			StartIdle();
			break;
		case ActorState::Fall:
			StartFall();
			break;
		case ActorState::Crouch:
			StartCrouch();
			break;
		case ActorState::WalkForward:
			StartWalkForward();
			break;
		case ActorState::WalkBackward:
			StartWalkBackward();
			break;
		case ActorState::WalkLeft:
			StartWalkLeft();
			break;
		case ActorState::WalkRight:
			StartWalkRight();
			break;
		case ActorState::Jump:
			StartJump();
			break;
		case ActorState::Reflection:
			StartReflect();
			break;	
	} 
}

void Actor::UpdateAABB(){
	currentAabb.max = assignedMesh->aabb.max + position;
	currentAabb.min = assignedMesh->aabb.min + position;
}

void Actor::offManualControl() {
	manualControl = false;
	destinationPoint = position;
}

void Actor::onManualControl() {
	manualControl = true;
	destinationPoint = position;
}

void Actor::ResetPosition() {
	position = initPosition;
	movement = glm::vec3(0.0f, 0.0f, 0.0f);
	movementGoal = glm::vec3(0.0f, 0.0f, 0.0f);
	destinationPoint = position;
}

// ------------- Manual control functions --------------- //

void Actor::Dolly(float actorVelocityGoal) {
	movementGoal.x = actorVelocityGoal;
}

void Actor::Pedestal(float actorVelocityGoal) {
	movementGoal.y = actorVelocityGoal;
}

void Actor::Strafe(float actorVelocityGoal) {
	movementGoal.z = actorVelocityGoal;
}

void Actor::Truck(float actorVelocityGoal) {
	Strafe(actorVelocityGoal);
}

// ---------------- State functions ------------------ //

void Actor::StartCrouch() {

}

void Actor::StartFall() {
	inAir = true;
}

void Actor::StartReflect() {
	//R = V - 2 N (N.V) / (N.N)
	
	movementGoal.x *=(-1);
	movementGoal.z *=(-1);
}

void Actor::StartWalkBackward() {
	movementGoal.x = -walkVelocity;
}

void Actor::StartWalkForward() {
	movementGoal.x = walkVelocity;
}

void Actor::StartWalkLeft() {
	movementGoal.z = -walkVelocity;
}

void Actor::StartWalkRight() {
	movementGoal.z = walkVelocity;
}


void Actor::StartIdle() {
	movementGoal = glm::vec3(0.0f, 0.0f, 0.0f);
}

void Actor::StartJump() {
	movementGoal.y = 1300.0f;
}
