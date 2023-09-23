#pragma once

#include <glm/glm.hpp>
// UI params are set via push constants
struct PushConstBlock {
	glm::vec2 scale;
	glm::vec2 translate;
};