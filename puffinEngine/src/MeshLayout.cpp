#include <array>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
#include <vulkan/vulkan.h>

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

	struct LineVertexLayout { //still not used
		glm::vec3 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(LineVertexLayout);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(LineVertexLayout, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(LineVertexLayout, color);

			return attributeDescriptions;
		}
	};
	
	struct ScenePart {
		
		struct AABB {
			glm::vec3 min;
			glm::vec3 max;
		} aabb; 

		uint32_t indexBase = 0;
		uint32_t indexCount = 0;
		std::string meshFilename;

		void GetAABB(const std::vector<enginetool::VertexLayout>& vertices) {
			glm::vec3 min; 
			glm::vec3 max;

			min.x = min.y = min.z = std::numeric_limits<float>::max();
			max.x = max.y = max.z = std::numeric_limits<float>::lowest();

			for(uint32_t i = indexBase; i < (indexBase + indexCount) ; ++i) {
				if (vertices[i].pos.x < min.x) min.x = vertices[i].pos.x;
				if (vertices[i].pos.x > max.x) max.x = vertices[i].pos.x;

				if (vertices[i].pos.y < min.y) min.y = vertices[i].pos.y;
				if (vertices[i].pos.y > max.y) max.y = vertices[i].pos.y;

				if (vertices[i].pos.z < min.z) min.z = vertices[i].pos.z;
				if (vertices[i].pos.z > max.z) max.z = vertices[i].pos.z;
			}

			aabb.min = min;
    		aabb.max = max;
		}

		static bool Overlaps(const AABB& a, const AABB& b) {
			return  a.max.x >= b.min.x && a.min.x <= b.max.x &&
					a.max.y >= b.min.y && a.min.y <= b.max.y &&
					a.max.z >= b.min.z && a.min.z <= b.max.z;
		}

		static bool Contains(const AABB& a, const AABB& b) {
			return  a.min.x <= b.min.x && b.max.x <= a.max.x &&
					a.min.y <= b.min.y && b.max.y <= a.max.y &&
					a.min.z <= b.min.z && b.max.z <= a.max.z;
		}

		static bool RayIntersection(glm::vec3& hitPoint, const glm::vec3& dirFrac, const glm::vec3& rayOrg, const glm::vec3& rayDir, const AABB& bb) {
			float t;

			float t1 = (bb.min.x - rayOrg.x)*dirFrac.x;
			float t2 = (bb.max.x - rayOrg.x)*dirFrac.x;
			float t3 = (bb.min.y - rayOrg.y)*dirFrac.y;
			float t4 = (bb.max.y - rayOrg.y)*dirFrac.y;
			float t5 = (bb.min.z - rayOrg.z)*dirFrac.z;
			float t6 = (bb.max.z - rayOrg.z)*dirFrac.z;

			float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
			float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

			// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
			if (tmax < 0) {
				t = tmax;
				return false;
			}

			if (tmin > tmax) {
				t = tmax;
				return false;
			}

			t = tmin; // Store length of ray until intersection in t
			hitPoint = rayOrg + rayDir * t; // intersection point
			return true;
		}
	};
}

namespace std {
	template<> struct hash<enginetool::VertexLayout> {
		size_t operator()(enginetool::VertexLayout const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.text_coord) << 1);
		}
	};
}