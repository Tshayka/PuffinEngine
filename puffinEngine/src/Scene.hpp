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
#include "MainCharacter.hpp"
#include "MaterialLibrary.hpp"
#include "MeshLibrary.hpp"
#include "MousePicker.hpp"
#include "Ui.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;
const float horizon = 9832.0f; //0.5km
static float cloudsPos = 0.0f;
const float cloudsVisibDist = 50.0f;

class Scene {
public:
	Scene();
	~Scene();

	void DeInitScene();
	void DeSelect();
	void HandleMouseClick();
	void InitScene(Device* device, GuiMainHub* statusOverlay, MousePicker* mousePicker, MeshLibrary* meshLibrary, MaterialLibrary* materialLibrary, WorldClock* mainClock, enginetool::ThreadPool& threadPool);
	void RecreateForSwapchain();
	void CleanUpForSwapchain();
	void CreateCommandBuffers();
	void CreateReflectionCommandBuffer();
	void CreateRefractionCommandBuffer();
	void UpdateGUI();
	void UpdateScene();

	// ------------ Scene navigation functions ------------- //

	void TestButton();
	void Test();

	void MoveCameraForward();
	void MoveCameraBackward();
	void StopCameraForwardBackward();
	void MoveCameraLeft();
	void MoveCameraRight();
	void StopCameraLeftRight();
	void MoveCameraUp();
	void MoveCameraDown();
	void StopCameraUpDown();

	void MakeMainCharacterJump();
	void MakeMainCharacterRun();
	void MoveMainCharacterForward();
	void MoveMainCharacterBackward();
	void MoveMainCharacterLeft();
	void MoveMainCharacterRight();
	void StopMainCharacter();

	void MakeSelectedActorJump();
	void MoveSelectedActorForward();
	void MoveSelectedActorBackward();
	void StopSelectedActorForwardBackward();
	void MoveSelectedActorLeft();
	void MoveSelectedActorRight();
	void StopSelectedActorLeftRight();
	void MoveSelectedActorUp();
	void MoveSelectedActorDown();
	void StopSelectedActorUpDown();
	
	void SelectionIndicatorToggle();
	void WireframeToggle();
	void AabbToggle();
	void ConsoleToggle();
	void AllGuiToggle();
	void MainUiToggle();
	void TextOverlayToggle();
	
	std::shared_ptr<Camera> currentCamera;
	std::shared_ptr<Actor> selectedActor;
	std::unique_ptr<Actor> mainCharacter;
	
	std::vector<std::shared_ptr<Actor>> sceneCameras;
	std::vector<std::shared_ptr<Actor>> seas;
	std::vector<std::shared_ptr<Actor>> skyboxes;
	std::vector<std::shared_ptr<Actor>> clouds;

	std::vector<std::shared_ptr<Actor>> actors;
		
	GuiMainHub *guiMainHub = nullptr;
	
	std::vector<VkCommandBuffer> commandBuffers;
	VkCommandBuffer reflectionCmdBuff;
	VkCommandBuffer refractionCmdBuff;

private:
	// ---------------- Main functions ------------------ //

	VkCommandBuffer BeginSingleTimeCommands();
	void CheckActorsVisibility();
	void CheckIfItIsVisible(std::shared_ptr<Actor>& actorToCheck);
	void CleanUpDepthResources();
	void CleanUpOffscreenImage(); 
	void CopyBuffer(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, const VkDeviceSize& size);
	void CreateActorsBuffers();
	void CreateBuffers();	
	void CreateCamera(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart &mesh, enginetool::SceneMaterial &material);
	void CreateCommandPool(); // neccsesary to create command buffer
	void CreateDepthResources();
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void CreateDescriptorSetLayout();
	void CreateFramebuffers();
	void CreateGraphicsPipeline();
	void CreateGUI(float, uint32_t);
	void CreateImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	void CreateLandscape(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart &mesh, enginetool::SceneMaterial &material);
	VkImageView CreateImageView(VkImage, VkFormat, VkImageAspectFlags);
	void CreateSea(std::string name, std::string description, glm::vec3 position);
	VkShaderModule CreateShaderModule(const std::vector<char>&);
	void CreateSkybox(std::string name, std::string description, glm::vec3 position, float horizon);
	void CreateTextureImageView(TextureLayout&);
	void CreateTextureSampler(TextureLayout&);
	void CreateSelectRay(); 
	void CreateCharacter(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart &mesh, enginetool::SceneMaterial &material);
	void CreateCloud(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart &mesh);
	void CreateMappedIndexBuffer(std::vector<uint32_t>& indices, enginetool::Buffer<void>& indexBuffer);
	void CreateMappedVertexBuffer(std::vector<enginetool::VertexLayout>& vertices, enginetool::Buffer<void>& vertexBuffer);
	void CreateSphereLight(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart &mesh);
	void CreateIndexBuffer(std::vector<uint32_t>& indices, enginetool::Buffer<void>& indexBuffer);
	void CreateVertexBuffer(std::vector<enginetool::VertexLayout>& vertices, enginetool::Buffer<void>& vertexBuffer);
	void EndSingleTimeCommands(const VkCommandBuffer& commandBuffer, const VkCommandPool& commandPool);
	bool FindDestinationPosition(glm::vec3& destinationPoint);
	bool HasStencilComponent(VkFormat);
	void InitMaterials();
	void InitSwapchainImageViews(); 
	void LoadAssets();
	void PrepeareMainCharacter(enginetool::ScenePart &mesh);
	void PrepareOffscreenImage();
	void ProcesTasksMultithreaded(enginetool::ThreadPool* threadPool, std::vector<std::function<void()>>& tasks);
	void RandomPositions();
	void SelectActor();
	void UpdateCloudsUniformBuffer(); 
	void UpdateDescriptorSet();
	void UpdateDynamicUniformBuffer();
	void UpdateSelectRayDrawData();
	void UpdateOceanUniformBuffer();
	void UpdatePositions(); 
	void UpdateSelectionIndicatorUniformBuffer();
	void UpdateSkyboxUniformBuffer();
	void UpdateStaticUniformBuffer();
	void UpdateOffscreenUniformBuffer();
	void UpdateUniformBufferParameters();
	
	std::function<void()> task1 = std::bind(&Scene::CheckActorsVisibility, this);
	std::function<void()> task2 = std::bind(&Scene::UpdatePositions, this);
	std::function<void()> task3 = std::bind(&Scene::UpdateSkyboxUniformBuffer, this);
	std::function<void()> task4 = std::bind(&Scene::UpdateUniformBufferParameters, this);
	std::function<void()> task5 = std::bind(&Scene::UpdateStaticUniformBuffer, this);
	std::function<void()> task6 = std::bind(&Scene::UpdateSelectionIndicatorUniformBuffer, this);
	std::function<void()> task7 = std::bind(&Scene::UpdateOffscreenUniformBuffer, this);
	std::function<void()> task8 = std::bind(&Scene::UpdateDynamicUniformBuffer, this);
	std::function<void()> task9 = std::bind(&Scene::UpdateOceanUniformBuffer, this);
	std::function<void()> task10 = std::bind(&Scene::UpdateCloudsUniformBuffer, this); 
	std::function<void()> task11 = std::bind(&Scene::CreateCommandBuffers, this);
	std::function<void()> task12 = std::bind(&Scene::CreateReflectionCommandBuffer, this);
	std::function<void()> task13 = std::bind(&Scene::CreateRefractionCommandBuffer, this);
	
	// ---------------- Deinitialisation ---------------- //

	void DeInitFramebuffer();
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
		float time; 
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
		float time; 
	} UBOO;

	// UBO Static parameters
	// If the member is a three-component vector with components consuming N basic machine units, the base alignment is 4N.
	struct UboParam	{
		glm::vec3 light_col; 
		float exposure;
		glm::vec3 light_pos[1];
	} UBOP;

	struct UboSkybox {
		glm::mat4 model;
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec3 cameraPos;
		float time;
	} UBOSB;

	struct UboSea {
		glm::mat4 model;
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec3 cameraPos;
		float time;
	} UBOSE;

	struct UboClouds {
		glm::mat4 model;
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec3 cameraPos;
		float time;
	} UBOC;

	// UBO Dynamic that contains all clouds matrices
	struct UboCloudsMatrices {
		glm::mat4 *model = nullptr;
	} uboDataDynamic;

	// Struct that holds the models positions
	struct Constants {
		glm::vec4 renderLimitPlane;
		glm::vec3 pos;
		bool glow;
		glm::vec3 color;
		//alignas(16) glm::vec2
	};

	struct {
		enginetool::Buffer<void> line;
		enginetool::Buffer<void> skybox;
		enginetool::Buffer<void> skyboxReflection;
		enginetool::Buffer<void> skyboxRefraction;
		enginetool::Buffer<void> clouds;
		enginetool::Buffer<void> clouds_dynamic;
		enginetool::Buffer<void> ocean;
		enginetool::Buffer<void> selectionIndicator;
		enginetool::Buffer<void> stillObjects;
		enginetool::Buffer<void> parameters;
		enginetool::Buffer<void> reflection;
		enginetool::Buffer<void> reflectionParameters;
		enginetool::Buffer<void> refraction;
		enginetool::Buffer<void> refractionParameters;
	} uniform_buffers;

	struct {
		enginetool::Buffer<void> meshLibraryObjects;
		enginetool::Buffer<void> skybox;
		enginetool::Buffer<void> ocean;
		enginetool::Buffer<void> selectRay;
		enginetool::Buffer<void> aabb;
	} vertex_buffers;

	struct {
		enginetool::Buffer<void> meshLibraryObjects;
		enginetool::Buffer<void> skybox;
		enginetool::Buffer<void> ocean;
		enginetool::Buffer<void> selectRay;
		enginetool::Buffer<void> aabb;
	} index_buffers;

	std::unique_ptr<TextureLayout> reflectionImage = std::make_unique<TextureLayout>();
	std::unique_ptr<TextureLayout> refractionImage = std::make_unique<TextureLayout>();

	std::unique_ptr<TextureLayout> screenDepthImage = std::make_unique<TextureLayout>();
	std::unique_ptr<TextureLayout> reflectionDepthImage = std::make_unique<TextureLayout>();
	std::unique_ptr<TextureLayout> refractionDepthImage = std::make_unique<TextureLayout>();

	bool displayWireframe = false;
	bool displaySceneGeometry = true;
	bool displayAabb = false;
	bool displayClouds = true;
	bool displaySkybox = true;
	bool displayOcean = true;
	bool displaySelectionIndicator = true;
	bool displayMainCharacter = true;

	glm::vec3 rnd_pos[DYNAMIC_UB_OBJECTS];
	
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
	
	enginetool::SceneMaterial sky;
	
	enginetool::ScenePart element;
	enginetool::ScenePart* selectionIndicatorMesh = nullptr;

	std::vector<uint32_t> rayIndices;
	std::vector<enginetool::VertexLayout> rayVertices;
	
	VkDescriptorSet aabbDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet lineDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet oceanDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet skybox_descriptor_set = VK_NULL_HANDLE;
	VkDescriptorSet cloudDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet skyboxReflectionDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet skyboxRefractionDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet selectionIndicatorDescriptorSet = VK_NULL_HANDLE;
	
	VkDescriptorSetLayout aabbDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout lineDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout oceanDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout skybox_descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout cloudDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout selectionIndicatorDescriptorSetLayout;

	VkCommandPool commandPool;
	VkCommandPool reflectionCommandPool;
	VkCommandPool refractionCommandPool;

	std::array<Constants, 3> pushConstants;

	size_t dynamicAlignment;
	VkDescriptorPool descriptorPool;
	VkPipelineLayout pipelineLayout;
	
	VkRect2D scissor = {};
	VkViewport viewport = {};

	Device* logicalDevice = nullptr;
	MaterialLibrary* materialLibrary = nullptr;
	MeshLibrary* meshLibrary = nullptr;
	MousePicker* mousePicker = nullptr;
	enginetool::ThreadPool* threadPool = nullptr;
	WorldClock* mainClock = nullptr;	
};