#include <iostream>
#include "Light.hpp"
#include "ErrorCheck.hpp"

// ------- Constructors and dectructors ------------- //

Light::Light(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors) 
: Actor(name, description, position, type, actors) {
#if DEBUG_VERSION
	std::cout << "Light created\n";
#endif
}

Light::~Light() {
#if DEBUG_VERSION
	std::cout << "Light destroyed\n";
#endif
}

SphereLight::SphereLight(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors) 
: Light(name, description, position, type, actors) {
#if DEBUG_VERSION
	std::cout << "Sphere light created\n";
#endif
}

SphereLight::~SphereLight() {
#if DEBUG_VERSION
	std::cout << "Sphere light destroyed\n";
#endif
}

Skybox::Skybox(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors, float horizon) 
: Light(name, description, position, type, actors) {
#if DEBUG_VERSION
	std::cout << "Skybox created\n";
#endif
	this->horizon = horizon;
}

Skybox::~Skybox() {
#if DEBUG_VERSION
	std::cout << "Skybox destroyed\n";
#endif
}


// --------------- Setters and getters -------------- //

glm::vec3 Light::GetLightColor() const {
	return lightColor;
}

void Light::SetLightColor(glm::vec3 lightColor) {
	this->lightColor = lightColor;
}

glm::vec3 Light::CalculateSelectionIndicatorColor() {
	return glm::vec3(1.0f, 1.0f, 1.0f);
}

void Light::UpdatePosition(float dt) {
	if(movementGoal!=glm::vec3(0.0f,0.0f,0.0f) && !manualControl) Actor::CheckIfInTheDestination();
	// Smooth movement and edge case in approach, without is movement is const
	movement.x = Approach(movementGoal.x, movement.x, dt * 80);
	movement.y = Approach(movementGoal.y, movement.y, dt * 80);
 	movement.z = Approach(movementGoal.z, movement.z, dt * 80);

	position += movement * dt;

	Actor::UpdateAABB();
}

void Skybox::CreateMesh() {
	vertices = {
		{{-horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.667f}, {0.0f, 1.0f, 0.0f}},
		{{horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.667f}, {0.0f, 1.0f, 0.0f}},
		{{horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{{horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{{-horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{{-horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.66f}, {0.0f, 1.0f, 0.0f}},
		
		{{-horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.33f}, {0.0f, -1.0f, 0.0f}},
		{{-horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.0f}, {0.0f, -1.0f, 0.0f}},
		{{horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.0f}, {0.0f, -1.0f, 0.0f}},
		{{horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.0f}, {0.0f, -1.0f, 0.0f}},
		{{horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.33f}, {0.0f, -1.0f, 0.0f}},
		{{-horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.33f}, {0.0f, -1.0f, 0.0f}},
		
		{{-horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.667f}, {0.0f, 0.0f, -1.0f}},
		{{-horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.33f}, {0.0f, 0.0f, -1.0f}},
		{{horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.33f}, {0.0f, 0.0f, -1.0f}},
		{{horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.33f}, {0.0f, 0.0f, -1.0f}},
		{{horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.667f}, {0.0f, 0.0f, -1.0f}},
		{{-horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.667f}, {0.0f, 0.0f, -1.0f}},
		
		{{horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.667f}, {-1.0f, 0.0f, 0.0f}},
		{{horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.33f}, {-1.0f, 0.0f, 0.0f}},
		{{horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.33f}, {-1.0f, 0.0f, 0.0f}},
		{{horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.33f}, {-1.0f, 0.0f, 0.0f}},
		{{horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.667f}, {-1.0f, 0.0f, 0.0f}},
		{{horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.66f}, {-1.0f, 0.0f, 0.0f}},
		
		{{horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.66f}, {0.0f, 0.0f, 1.0f}},
		{{horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.33f}, {0.0f, 0.0f, 1.0f}},
		{{-horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.33f}, {0.0f, 0.0f, 1.0f}},
		{{-horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.33f}, {0.0f, 0.0f, 1.0f}},
		{{-horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.667f}, {0.0f, 0.0f, 1.0f}},
		{{horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.667f}, {0.0f, 0.0f, 1.0f}},
		
		{{-horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.66f}, {1.0f, 0.0f, 0.0f}},
		{{-horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.33f}, {1.0f, 0.0f, 0.0f}},
		{{-horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.33f}, {1.0f, 0.0f, 0.0f}},
		{{-horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.33f}, {1.0f, 0.0f, 0.0f}},
		{{-horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.667f}, {1.0f, 0.0f, 0.0f}},
		{{-horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.667f}, {1.0f, 0.0f, 0.0f}}
	};

	indices = {
		0,1,2,3,4,5,
		6,7,8,9,10,11,
		12,13,14,15,16,17,
		18,19,20,21,22,23,
		24,25,26,27,28,29,
		30,31,32,33,34,35
	};
}