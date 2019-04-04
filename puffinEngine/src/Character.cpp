#include <fstream>
#include <iostream>

#include "Character.hpp"

// ------- Constructors and dectructors ------------- //

Character::Character(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors) 
: Actor(name, description, position, type, actors) {
	// create save file
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Character created\n";
#endif
}

Character::~Character() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Character destroyed\n";
#endif
}

// ---------------- Observer methods ---------------- //

void Character::Attach(Observer *observer) {
	obervatorsToUpdate.push_back(observer);
}

void Character::Notify() {
	for (int i = 0; i < obervatorsToUpdate.size(); ++i)
		obervatorsToUpdate[i]->Update(gold);
}

// ---------------------- Init ---------------------- //

void Character::Init(unsigned int maxHealth, int currentHealth, unsigned int gold) {
    this->maxHealth = maxHealth;
	this->currentHealth = currentHealth;
	this->gold = gold;
}

void Character::AddGold(unsigned int amount) {
	gold+=amount;
	std::cout << gold << "\n";
	Notify();
}

void Character::AddToInventory(Item* item){
	inventory.push_back(std::move(*item));
	std::cout << inventory.size() << "\n";
}

bool Character::CheckItem(Item* item){
	// in future add chain of responsibility
	if(item->itemType==ItemType::Money) {
		AddGold(item->GetValue());
		return true;
	}
	
	if(item->itemType==ItemType::Sword) {
		AddToInventory(item);
		return true;
	}

	return false;
}


void Character::FindCollidingItem() {
	std::cout << interactActors->size() << "\n";
	std::vector<std::shared_ptr<Actor>>::iterator it;
	for (it = interactActors->begin(); it!=interactActors->end() ;++it) {
		if(it->get()->GetType()==ActorType::Item){
			if(enginetool::ScenePart::Overlaps(this->currentAabb, it->get()->currentAabb) && it->get()!=this) {
				if(CheckItem(dynamic_cast<Item*>(it->get()))){
					interactActors->erase(it);// remove item from scene
					break;
				}		
			}
		}
	}
	std::cout << interactActors->size() << "\n";
}

glm::vec3 Character::CalculateSelectionIndicatorColor(){
	float perentOfMax = currentHealth / maxHealth;
	return glm::vec3(2.0f * (1.0f - perentOfMax), 2.0f * perentOfMax, 0.0f);
}

void Character::UpdatePosition(float dt) {
	//if(movementGoal!=glm::vec3(0.0f,0.0f,0.0f) && !manualControl) Actor::CheckIfInTheDestination();

	groundLevel = DetectGroundLevel();
	CheckCollisions();
	
	movement.x = Approach(movementGoal.x, movement.x, dt * 1000);
	movement.y = Approach(movementGoal.y, movement.y, dt * 1000);
 	movement.z = Approach(movementGoal.z, movement.z, dt * 1000);

	velocity = movement;
	position += velocity * dt;

	if (position.y <= groundLevel) {
		position.y = groundLevel;
		if (inAir) {
			inAir = false;
			SetState(ActorState::Idle);
		}
	}
	else SetState(ActorState::Fall);
		
	if (inAir) movementGoal.y -= 100.0f;

	UpdateAABB();
}


// void Camera::UpdatePosition(float dt) {
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
