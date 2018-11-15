#pragma once

#define GLM_FORCE_RADIANS // necessary to make sure that functions like glm::rotate use radians as arguments, to avoid any possible confusion
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp> 

#define DYNAMIC_UB_OBJECTS 216 // Clouds

#include "Character.hpp"
#include "Camera.hpp"
#include "Device.hpp"
#include "Light.hpp"
#include "StatusOverlay.hpp"
#include "Ui.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;

class Scene
{
public:
	Scene();
	~Scene();

	void ConnectGamepad();
	void DeInitScene();
	void InitScene(Device*, GLFWwindow*, ImGuiMenu*, StatusOverlay*);
	void PressKey(int);
	void RecreateForSwapchain();
	void CleanUpForSwapchain();
	void CreateCommandBuffers();
	void UpdateGUI(float frameTimer, uint32_t elapsedTime);
	void UpdateScene(const float &dt, const float &time, float const &accumulator);

	ImGuiMenu* console = nullptr;
	
	std::vector<std::shared_ptr<Actor>> actors;
		
	StatusOverlay *status_overlay = nullptr;
	
	std::vector<VkCommandBuffer> command_buffers;

private:

	// ---------------- Main functions ------------------ //

	VkCommandBuffer BeginSingleTimeCommands();
	void CopyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
	void CopyBufferToImage(VkBuffer, VkImage, uint32_t, uint32_t);
	void CreateCommandPool(); // neccsesary to create command buffer
	void CreateGUI(float, uint32_t);
	void CreateDepthResources();
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void CreateDescriptorSetLayout();
	void CreateFramebuffers();
	void CreateGraphicsPipeline();
	void CreateImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	VkImageView CreateImageView(VkImage, VkFormat, VkImageAspectFlags);
	VkShaderModule CreateShaderModule(const std::vector<char>&);
	void CreateTextureImageView(enginetool::TextureLayout&);
	void CreateTextureSampler(enginetool::TextureLayout&);
	void CreateUniformBuffer();
	void EndSingleTimeCommands(VkCommandBuffer);
	bool HasStencilComponent(VkFormat);
	void CreateCamera();
	void CreateCharacter();
	void CreateSphereLight();
	void CreateStillObject();
	void InitMaterials();
	void InitSwapchainImageViews();
	void LoadFromFile(const std::string &filename, enginetool::ScenePart& meshes, std::vector<uint32_t>& indices, std::vector<enginetool::VertexLayout>& vertices); 
	void CreateBuffers(std::vector<uint32_t>&, std::vector<enginetool::VertexLayout>&, enginetool::Buffer&, enginetool::Buffer&); //TODO
	void LoadAssets();
	void LoadSkyboxTexture(enginetool::TextureLayout&);
	void LoadTexture(std::string, enginetool::TextureLayout&);
	void TransitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout);
	void UpdateDynamicUniformBuffer(const float& time);
	void UpdateSkyboxUniformBuffer();
	void UpdateUBOParameters();
	void UpdateUniformBuffer(const float& time);

	// ---------------- Deinitialisation ---------------- //

	void DeInitFramebuffer();
	void DeInitImageView();
	void DeInitIndexAndVertexBuffer();
	void DeInitTextureImage();
	void DeInitUniformBuffer();
	void DestroyPipeline();
	void FreeCommandBuffers();
	
	// UBO Static scene objects
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec3 camera_pos; 
	};

	// UBO Static parameters
	// If the member is a three-component vector with components consuming N basic machine units, the base alignment is 4N.
	struct UniformBufferObjectParam	{
		glm::vec3 light_col; 
		float exposure;
		glm::vec3 light_pos[1];
	};

	// UBO for skybox
	struct UniformBufferObjectSky
	{
		glm::mat4 view;
		glm::mat4 proj;
		float exposure;
		float gamma;
	};

	// UBO Dynamic for clouds
	struct UboClouds {
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec3 camera_pos;
		float time;
	};

	// UBO Dynamic that contains all clouds matrices
	struct UboCloudsMatrices
	{
		glm::mat4 *model = nullptr;
	} uboDataDynamic;

	// Struct that holds the models positions
	struct Constants
	{
		glm::vec3 pos;
	} pushConstants;

	struct {
		enginetool::Buffer skybox;
		enginetool::Buffer clouds;
		enginetool::Buffer clouds_dynamic;
		enginetool::Buffer objects;
		enginetool::Buffer still_objects; //TODO
		enginetool::Buffer parameters;
	} uniform_buffers;

	struct {
		enginetool::Buffer skybox;
		enginetool::Buffer clouds;
		enginetool::Buffer objects;
	} vertex_buffers;

	struct {
		enginetool::Buffer skybox;
		enginetool::Buffer clouds;
		enginetool::Buffer objects;
	} index_buffers;

	glm::vec3 rnd_pos[DYNAMIC_UB_OBJECTS];

	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;

	VkPipeline pbr_pipeline;
	VkPipeline clouds_pipeline;
	VkPipeline skybox_pipeline;

	enginetool::SceneMaterial *sky = new enginetool::SceneMaterial();
	enginetool::SceneMaterial *rust = new enginetool::SceneMaterial();
	enginetool::SceneMaterial *chrome = new enginetool::SceneMaterial();
	enginetool::SceneMaterial *plastic = new enginetool::SceneMaterial();
	enginetool::SceneMaterial *lightbulbMat = new enginetool::SceneMaterial();
	enginetool::SceneMaterial *cameraMat = new enginetool::SceneMaterial();
	enginetool::SceneMaterial *characterMat = new enginetool::SceneMaterial();

	std::vector<enginetool::SceneMaterial> scene_material;

	enginetool::ScenePart skybox_mesh;
	enginetool::ScenePart element;
	enginetool::ScenePart cloud_mesh;

	//pog::VertexLayout skybox;

	std::vector<uint32_t> objects_indices;
	std::vector<enginetool::VertexLayout> objects_vertices;
	std::vector<uint32_t> skybox_indices;
	std::vector<enginetool::VertexLayout> skybox_vertices;
	std::vector<uint32_t> clouds_indices;
	std::vector<enginetool::VertexLayout> clouds_vertices;

	VkDescriptorSet skybox_descriptor_set = VK_NULL_HANDLE;
	VkDescriptorSet clouds_descriptor_set = VK_NULL_HANDLE;
	
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout skybox_descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout clouds_descriptor_set_layout = VK_NULL_HANDLE;

	VkCommandPool command_pool;
	size_t dynamicAlignment;
	VkDescriptorPool descriptor_pool;
	VkPipelineLayout pipeline_layout;

	VkRect2D scissor = {};
	VkViewport viewport = {};

	Device* logical_device = nullptr;
	GLFWwindow *window = nullptr;
};