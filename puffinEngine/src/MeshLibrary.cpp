#include <iostream>

#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h> // loading obj files

#include "MeshLayout.cpp"
#include "MeshLibrary.hpp"
#include "ErrorCheck.hpp"

// ------- Constructors and dectructors ------------- //

MeshLibrary::MeshLibrary() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Mesh library created\n";
#endif
}

MeshLibrary::~MeshLibrary() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Mesh library destroyed\n";
#endif 
}

// ---------------- Main functions ------------------ //

void MeshLibrary::Init(Device* device) {
	logicalDevice = device;

    FillLibrary(); 
}

void MeshLibrary::LoadFromFile(const std::string &filename, enginetool::ScenePart& mesh, std::vector<uint32_t>& indices, std::vector<enginetool::VertexLayout>& vertices){
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

void MeshLibrary::FillLibrary() {
    enginetool::ScenePart box, teapot, human, plane, cloud, sphere, smallCoinB; 

    sphere.meshFilename = "puffinEngine/assets/models/sphere.obj";
	teapot.meshFilename = "puffinEngine/assets/models/teapotR200originMid.obj";
	cloud.meshFilename = "puffinEngine/assets/models/cloud.obj";
    plane.meshFilename = "puffinEngine/assets/models/planeHorizontal1000x1000x1000originMid.obj";
	box.meshFilename = "puffinEngine/assets/models/box100x100x100originMId.obj";
	human.meshFilename = "puffinEngine/assets/models/box180x500x500originMidBot.obj";
    smallCoinB.meshFilename = "puffinEngine/assets/models/selectionCoinSmallB.obj";

    meshes = {
        {"box", box},
        {"teapot", teapot},
        {"human", human},
        {"plane", plane},
        {"cloud", cloud},
        {"sphere", sphere},
        {"SmallCoinB", smallCoinB}
    };

    for (auto& m : meshes) {
        Load(m.second);
        m.second.GetAABB(vertices);
    }
    
}

void MeshLibrary::Load(enginetool::ScenePart& mesh){
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, mesh.meshFilename.c_str())) {
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

void MeshLibrary::DeInit() {
	logicalDevice = nullptr;
}