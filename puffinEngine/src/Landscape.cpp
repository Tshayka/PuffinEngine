#include <iostream>

#include "Landscape.hpp"
#include "ErrorCheck.hpp"


// ------- Constructors and dectructors ------------- //

Landscape::Landscape(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors) 
: Actor(name, description, position, type, actors) {
#if BUILD_ENABLE_VULKAN_DEBUG
	// create save file
	std::cout << "Landscape created\n";
#endif 
}

Landscape::~Landscape() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Landscape destroyed\n";
#endif 
}

// ---------------- Main functions ------------------ //

void Landscape::Init(unsigned int maxHealth, int currentHealth) {
    this->maxHealth = maxHealth;
	this->currentHealth = currentHealth;
}

void Landscape::UpdatePosition(float dt) {
	if(movementGoal!=glm::vec3(0.0f,0.0f,0.0f) && !manualControl) Actor::CheckIfInTheDestination();

	movement.x = Approach(movementGoal.x, movement.x, dt * 500);
	movement.y = Approach(movementGoal.y, movement.y, dt * 500);
 	movement.z = Approach(movementGoal.z, movement.z, dt * 500);

	position += movement * dt;

	Actor::UpdateAABB();
}

glm::vec3 Landscape::CalculateSelectionIndicatorColor(){
	float perentOfMax = currentHealth / maxHealth;
	return glm::vec3(2.0f * (1.0f - perentOfMax), 2.0f * perentOfMax, 0.0f);
}

// void Camera::UpdatePosition(float dt)
// {
// 	direction = glm::vec3(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));

// 	// Smooth movement and edge case in approach, without is movement is const
// 	movement.x = Approach(movementGoal.x, movement.x, dt * 80);
// 	movement.y = Approach(movementGoal.y, movement.y, dt * 80);
// 	movement.z = Approach(movementGoal.z, movement.z, dt * 80);

// 	foward = direction;
// 	foward.y = 0.0f;
// 	glm::normalize(foward);

// 	velocity = foward * movement.x + glm::cross(up, foward) * movement.z;
// 	velocity.y = movement.y;
// 	position += velocity * dt;
// 	view = position + direction; 
// }

void Landscape::ResetPosition(){
	Actor::ResetPosition();
}

// ------- Constructors and dectructors ------------- //

Sea::Sea(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors) 
: Landscape(name, description, position, type, actors) {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Sea created\n";
#endif
}

Sea::~Sea() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Sea destroyed\n";
#endif
}

// ---------------- Main functions ------------------ //

void Sea::CreateMesh() {
	int multipler = 200;
	int vSize = multipler;//density of grid
	float offset = multipler / 2.0f; 
	float scale = multipler / (float)vSize;

	vertices.resize(6*(vSize-1)*(vSize-1));

        // create local vertices
        for (uint z = 0; z < vSize; ++z) {
            for (uint x = 0; x < vSize; ++x) {
                uint index = z * vSize + x;

				vertices[index].pos.x = scale*(float)x - offset;
				vertices[index].pos.y =  0.0f;
				vertices[index].pos.z = scale*(float)z - offset;

                vertices[index].color.x = 1.0f;
				vertices[index].color.y = 1.0f;
				vertices[index].color.z = 1.0f;

				vertices[index].text_coord.x = (float)x - offset;
				vertices[index].text_coord.y = (float)z - offset;

				vertices[index].normals.x = 0.0f;
				vertices[index].normals.y = 1.0f;
				vertices[index].normals.z = 0.0f;
            }
        }
     
		std::cout << "Ocean vertices size: " << vertices.size() << std::endl;

		for(int z = 0; z < vSize-1; ++z) {
			for(int x = 0; x < vSize-1; ++x) {
				int topLeft = (z*vSize)+x;
				int topRight = topLeft + 1;
				int bottomLeft = ((z+1)*vSize)+x;
				int bottomRight = bottomLeft + 1;
				indices.emplace_back(topLeft);
				indices.emplace_back(bottomLeft);
				indices.emplace_back(topRight);
				indices.emplace_back(topRight);
				indices.emplace_back(bottomLeft);
				indices.emplace_back(bottomRight);
			}
		}	
}


// ------- Constructors and dectructors ------------- //

Cloud::Cloud(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors) 
: Landscape(name, description, position, type, actors) {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Cloud created\n";
#endif
}

Cloud::~Cloud() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Cloud destroyed\n";
#endif
}