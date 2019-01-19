#pragma once

#define GLM_FORCE_RADIANS // necessary to make sure that functions like glm::rotate use radians as arguments, to avoid any possible confusion
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp> 

#define DYNAMIC_UB_OBJECTS 216 // Clouds

#include "Character.hpp"
#include "Camera.hpp"
#include "Device.hpp"
#include "Landscape.hpp"
#include "Light.hpp"
#include "GuiMainHub.hpp"
#include "MousePicker.hpp"
#include "Texture.hpp"
#include "Ui.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;
const float horizon = 9832.0f; //0.5km
static float cloudsPos = 0.0f;
const float cloudsVisibDist = 50.0f;

class Scene
{
public:
	Scene();
	~Scene();

	void ConnectGamepad();
	void DeInitScene();
	void DeSelect();
	void HandleMouseClick();
	void InitScene(Device* device, GLFWwindow* window, GuiMainHub* statusOverlay, MousePicker* mousePicker);
	void PressKey(int);
	void RecreateForSwapchain();
	void CleanUpForSwapchain();
	void CreateCommandBuffers();
	void CreateReflectionCommandBuffer();
	void CreateRefractionCommandBuffer();
	void UpdateGUI(float frameTimer, uint32_t elapsedTime);
	void UpdateScene(const float &dt, const float &time, float const &accumulator);

	bool displayWireframe = false;
	bool displaySceneGeometry = true;
	bool displayAabbBorders = false;
	bool displayClouds = true;
	bool displaySkybox = true;
	bool displayOcean = true;
	bool displaySelectionIndicator = true;
	
	std::shared_ptr<Camera> currentCamera = nullptr;
	std::shared_ptr<Actor> selectedActor = nullptr;
	
	std::vector<std::shared_ptr<Actor>> sceneCameras;
	std::vector<std::shared_ptr<Actor>> actors;
		
	GuiMainHub *guiMainHub = nullptr;
	
	std::vector<VkCommandBuffer> command_buffers;
	VkCommandBuffer reflectionCmdBuff = VK_NULL_HANDLE;
	VkCommandBuffer refractionCmdBuff = VK_NULL_HANDLE;

private:

	// ---------------- Main functions ------------------ //

	VkCommandBuffer BeginSingleTimeCommands();
	void CopyBuffer(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, const VkDeviceSize& size);
	void CreateAABBBuffers();
	void GetAABBDrawData(const enginetool::ScenePart& mesh) noexcept;
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
	void CreateOcean() noexcept;
	void CreateSelectionIndicator();
	VkShaderModule CreateShaderModule(const std::vector<char>&);
	void CreateSkybox() noexcept;
	void CreateTextureImageView(TextureLayout&);
	void CreateTextureSampler(TextureLayout&);
	void CreateUniformBuffer();
	float DetectGround();
	void EndSingleTimeCommands(const VkCommandBuffer& commandBuffer, const VkCommandPool& commandPool);
	bool HasStencilComponent(VkFormat);
	void CreateCamera();
	void CreateLandscape(std::string name, std::string description, glm::vec3 position, std::string meshFilename);
	void CreateSelectRay(); 
	void CreateCharacter();
	void CreateMappedIndexBuffer(std::vector<uint32_t>& indices, enginetool::Buffer& indexBuffer);
	void CreateMappedVertexBuffer(std::vector<enginetool::VertexLayout>& vertices, enginetool::Buffer& vertexBuffer);
	void CreateSphereLight();
	bool FindDestinationPosition(glm::vec3& destinationPoint);
	void InitMaterials();
	void InitSwapchainImageViews();
	void LoadFromFile(const std::string &filename, enginetool::ScenePart& meshes, std::vector<uint32_t>& indices, std::vector<enginetool::VertexLayout>& vertices); 
	void CreateIndexBuffer(std::vector<uint32_t>& indices, enginetool::Buffer& indexBuffer);
	void CreateVertexBuffer(std::vector<enginetool::VertexLayout>& vertices, enginetool::Buffer& vertexBuffer); 
	void LoadAssets();
	void LoadSkyboxTexture(TextureLayout&);
	void LoadTexture(std::string, TextureLayout&);
	void PrepareOffscreen();
	void RandomPositions();
	void SelectActor(); 
	void UpdateAABBDrawData();
	void UpdateDynamicUniformBuffer(const float& time);
	void UpdateSelectRayDrawData();
	void UpdateOceanUniformBuffer(const float& time); 
	void UpdateSelectionIndicatorUniformBuffer(const float& time);
	void UpdateSkyboxUniformBuffer();
	void UpdateStaticUniformBuffer(const float& time);
	void UpdateUBOOffscreen(const float& time);
	void UpdateUBOParameters();
	

	// ---------------- Deinitialisation ---------------- //

	void DeInitFramebuffer();
	void DeInitDepthResources();
	void DeInitIndexAndVertexBuffer();
	void DeInitTextureImage();
	void DeInitUniformBuffer();
	void DestroyPipeline();
	void FreeCommandBuffers();
	
	struct UboStaticGeometry {
		glm::mat4 model;
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec3 cameraPos; 
	} UBOSG;

	struct UboSelectionIndicator {
		glm::mat4 model;
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec3 cameraPos;
		float time; 
	} UBOSI;

	struct UboOffscreen {
		glm::mat4 model;
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec3 cameraPos; 
	} UBOO;

	// UBO Static parameters
	// If the member is a three-component vector with components consuming N basic machine units, the base alignment is 4N.
	struct UboParam	{
		glm::vec3 light_col; 
		float exposure;
		glm::vec3 light_pos[1];
	} UBOP;

	struct UboSkybox {
		glm::mat4 view;
		glm::mat4 proj;
		float exposure;
		float gamma;
	} UBOSB;

	struct UboSea {
		glm::mat4 model;
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec3 cameraPos;
		float time;
	} UBOSE;

	struct UboClouds {
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec3 cameraPos;
		float time;
	} UBOC;

	// struct UboLine {
	// 	glm::mat4 proj;
	// 	glm::mat4 view;
	// 	glm::mat4 model;
	// };

	// UBO Dynamic that contains all clouds matrices
	struct UboCloudsMatrices {
		glm::mat4 *model = nullptr;
	} uboDataDynamic;

	// Struct that holds the models positions
	struct Constants {
		glm::vec4 color;
		glm::vec4 renderLimitPlane;
		glm::vec3 pos;
		bool selected;
	} pushConstants;

	struct {
		enginetool::Buffer line;
		enginetool::Buffer skybox;
		enginetool::Buffer skyboxReflection;
		enginetool::Buffer skyboxRefraction;
		enginetool::Buffer clouds;
		enginetool::Buffer clouds_dynamic;
		enginetool::Buffer ocean;
		enginetool::Buffer selectionIndicator;
		enginetool::Buffer stillObjects;
		enginetool::Buffer parameters;
		enginetool::Buffer reflection;
		enginetool::Buffer reflectionParameters;
		enginetool::Buffer refraction;
		enginetool::Buffer refractionParameters;
	} uniform_buffers;

	struct {
		enginetool::Buffer skybox;
		enginetool::Buffer ocean;
		enginetool::Buffer clouds;
		enginetool::Buffer objects;
		enginetool::Buffer selectRay;
		enginetool::Buffer aabb;
		enginetool::Buffer selectionIndicator;
	} vertex_buffers;

	struct {
		enginetool::Buffer skybox;
		enginetool::Buffer ocean;
		enginetool::Buffer clouds;
		enginetool::Buffer objects;
		enginetool::Buffer selectRay;
		enginetool::Buffer aabb;
		enginetool::Buffer selectionIndicator;
	} index_buffers;

	struct OffscreenPass {
		int32_t width, height;
		TextureLayout reflectionImage;
		TextureLayout reflectionDepthImage;
		TextureLayout refractionImage;
		TextureLayout refractionDepthImage;
	} offscreenPass;

	glm::vec3 rnd_pos[DYNAMIC_UB_OBJECTS];

	TextureLayout depthImage;

	VkPipeline aabbPipeline;
	VkPipeline selectRayPipeline;
	VkPipeline pbrWireframePipeline;
	VkPipeline pbrPipeline;
	VkPipeline pbrRefractionPipeline;
	VkPipeline pbrReflectionPipeline;
	VkPipeline oceanPipeline;
	VkPipeline oceanWireframePipeline;
	VkPipeline cloudsPipeline;
	VkPipeline cloudsWireframePipeline;
	VkPipeline skyboxPipeline;
	VkPipeline skyboxWireframePipeline;
	VkPipeline skyboxRefractionPipeline;
	VkPipeline skyboxReflectionPipeline;
	VkPipeline selectionIndicatorPipeline;
	
	enginetool::SceneMaterial *sky = new enginetool::SceneMaterial(logicalDevice);
	enginetool::SceneMaterial *rust = new enginetool::SceneMaterial(logicalDevice);
	enginetool::SceneMaterial *chrome = new enginetool::SceneMaterial(logicalDevice);
	enginetool::SceneMaterial *plastic = new enginetool::SceneMaterial(logicalDevice);
	enginetool::SceneMaterial *lightbulbMat = new enginetool::SceneMaterial(logicalDevice);
	enginetool::SceneMaterial *cameraMat = new enginetool::SceneMaterial(logicalDevice);
	enginetool::SceneMaterial *characterMat = new enginetool::SceneMaterial(logicalDevice);

	std::vector<enginetool::SceneMaterial> scene_material;

	enginetool::ScenePart element;
	enginetool::ScenePart cloud_mesh;
	enginetool::ScenePart selectionIndicatorMesh;

	std::vector<uint32_t> objects_indices;
	std::vector<enginetool::VertexLayout> objectsVertices;
	std::vector<uint32_t> skybox_indices;
	std::vector<enginetool::VertexLayout> skybox_vertices;
	std::vector<uint32_t> oceanIndices;
	std::vector<enginetool::VertexLayout> oceanVertices;
	std::vector<uint32_t> clouds_indices;
	std::vector<enginetool::VertexLayout> clouds_vertices;
	std::vector<uint32_t> rayIndices;
	std::vector<enginetool::VertexLayout> rayVertices;
	std::vector<uint32_t> aabbIndices;
	std::vector<enginetool::VertexLayout> aabbVertices;
	std::vector<uint32_t> selectIndicatorIndices;
	std::vector<enginetool::VertexLayout> selectIndicatorVertices;

	VkDescriptorSet lineDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet oceanDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet skybox_descriptor_set = VK_NULL_HANDLE;
	VkDescriptorSet clouds_descriptor_set = VK_NULL_HANDLE;
	VkDescriptorSet skyboxReflectionDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet skyboxRefractionDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet selectionIndicatorDescriptorSet = VK_NULL_HANDLE;
	
	VkDescriptorSetLayout lineDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout oceanDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout skybox_descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout clouds_descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout selectionIndicatorDescriptorSetLayout;

	VkCommandPool commandPool;
	size_t dynamicAlignment;
	VkDescriptorPool descriptorPool;
	VkPipelineLayout pipelineLayout;
	
	VkRect2D scissor = {};
	VkViewport viewport = {};

	Device* logicalDevice = nullptr;
	MousePicker* mousePicker = nullptr;	
	GLFWwindow* window = nullptr;
};