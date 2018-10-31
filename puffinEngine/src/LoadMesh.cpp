#pragma once

#include <array>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "LoadTexture.cpp"

namespace enginetool {
	struct VertexLayout {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 text_coord; //uv
		glm::vec3 normals;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(VertexLayout);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(VertexLayout, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(VertexLayout, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(VertexLayout, text_coord);

			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[3].offset = offsetof(VertexLayout, normals);

			return attributeDescriptions;
		}

		bool operator==(const VertexLayout& other) const {
			return pos == other.pos && color == other.color && text_coord == other.text_coord;
		}
	};
	
	struct ScenePart {
		uint32_t indexBase;
		uint32_t indexCount;
		SceneMaterial* assigned_material;
	};
}

namespace std {
	template<> struct hash<pog::VertexLayout> {
		size_t operator()(pog::VertexLayout const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.text_coord) << 1);
		}
	};
}