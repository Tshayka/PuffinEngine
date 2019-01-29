#include <fstream>
#include <iostream>
#include <cmath>

#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h> // loading obj files

#include "Actor.hpp"

Actor::Actor(std::string name, std::string description, glm::vec3 position, ActorType type) : state(ActorState::Idle) {
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

// Use proxy design pattern!
void Actor::LoadFromFile(const std::string &filename, enginetool::ScenePart& mesh, std::vector<uint32_t>& indices, std::vector<enginetool::VertexLayout>& vertices){
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
		throw std::runtime_error(warn + err);
	}

	mesh.indexBase = static_cast<uint32_t>(indices.size());
	std::unordered_map<enginetool::VertexLayout, uint32_t> uniqueVertices = {};
	
	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		uint32_t index_offset = 0;		
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {

				tinyobj::index_t index = shapes[s].mesh.indices[index_offset + v];

				enginetool::VertexLayout vertex = {};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.text_coord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.normals = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				vertex.color = { 1.0f, 1.0f, 1.0f };

				// if (uniqueVertices.count(vertex) == 0) {
				// 	uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				// 	vertices.emplace_back(vertex);
				// }
				// indices.emplace_back(uniqueVertices[vertex]);

				vertices.emplace_back(vertex);
				indices.emplace_back(indices.size());
			}
			index_offset += fv;
		}	
		mesh.indexCount = index_offset;
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
	} 
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

void Actor::StartWalkBackward() {
	movementGoal.x = -walkVelocity;
}

void Actor::StartWalkForward() {
	movementGoal.x = walkVelocity;
}

void Actor::StartWalkLeft() {
	movementGoal.z = walkVelocity;
}

void Actor::StartWalkRight() {
	movementGoal.z = -walkVelocity;
}


void Actor::StartIdle() {
	movementGoal = glm::vec3(0.0f, 0.0f, 0.0f);
}

void Actor::StartJump() {
	movementGoal.y = 1300.0f;
}
