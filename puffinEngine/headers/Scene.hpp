#pragma once

#define GLM_FORCE_RADIANS // necessary to make sure that functions like glm::rotate use radians as arguments, to avoid any possible confusion
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp> 

#define DYNAMIC_UB_OBJECTS 125 // Clouds

#include "Character.hpp"
#include "Camera.hpp"
#include "Buffer.hpp"
#include "Landscape.hpp"
#include "Light.hpp"
#include "GuiMainHub.hpp"
#include "MainCharacter.hpp"
#include "MaterialLibrary.hpp"
#include "MeshLibrary.hpp"
#include "MousePicker.hpp"
#include "RenderPass.hpp"
#include "SwapChain.hpp"
#include "Texture.hpp"
#include "Ui.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;
const float horizon = 9832.0f; //0.5km
static float cloudsPos = 0.0f;
const float cloudsVisibDist = 50.0f;

namespace puffinengine {
	namespace tool {
		class Scene {
		public:
			Scene();
			~Scene();

			bool m_Initialized = false;

			void init(GLFWwindow* window, Device* device, SwapChain* SwapChain, RenderPass* screenRenderPass, RenderPass* offScreenRenderPass, GuiMainHub* statusOverlay, MousePicker* mousePicker, MeshLibrary* meshLibrary, MaterialLibrary* materialLibrary, WorldClock* mainClock, enginetool::ThreadPool* threadPool);
			void cleanUpForSwapchain();
			void RecreateForSwapchain();
			void deinit();
			
			void DeSelect();
			void HandleMouseClick();
			void CreateCommandBuffers();
			void CreateReflectionCommandBuffer();
			void CreateRefractionCommandBuffer();
			void UpdateGUI();
			void update();

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

			GuiMainHub* m_GUIMainHub = nullptr;

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
			void copyBuffer(enginetool::Buffer* srcBuffer, enginetool::Buffer* dstBuffer, const VkDeviceSize size);
			void CreateActorsBuffers();
			void CreateBuffers();
			void CreateCamera(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart& mesh, enginetool::SceneMaterial& material);
			void CreateCommandPool(); // neccsesary to create command buffer
			void CreateDepthResources();
			void CreateDescriptorPool();
			void CreateDescriptorSet();
			void CreateDescriptorSetLayout();
			void CreateFramebuffers();
			void CreateGraphicsPipeline();
			void CreateGUI(float, uint32_t);
			void CreateImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
			void CreateLandscape(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart& mesh, enginetool::SceneMaterial& material);
			VkImageView CreateImageView(VkImage, VkFormat, VkImageAspectFlags);
			void CreateSea(std::string name, std::string description, glm::vec3 position);
			VkShaderModule CreateShaderModule(const std::vector<char>&);
			void CreateSkybox(std::string name, std::string description, glm::vec3 position, float horizon);
			void CreateTextureImageView(TextureLayout&);
			void CreateTextureSampler(TextureLayout&);
			void CreateSelectRay();
			void CreateCharacter(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart& mesh, enginetool::SceneMaterial& material);
			void CreateCloud(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart& mesh);
			void CreateMappedIndexBuffer(std::vector<uint32_t>& indices, enginetool::Buffer& indexBuffer);
			void CreateMappedVertexBuffer(std::vector<enginetool::VertexLayout>& vertices, enginetool::Buffer& vertexBuffer);
			void CreateSphereLight(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart& mesh);
			void CreateIndexBuffer(std::vector<uint32_t>& indices, enginetool::Buffer& indexBuffer);
			void CreateVertexBuffer(std::vector<enginetool::VertexLayout>& vertices, enginetool::Buffer& vertexBuffer);
			void EndSingleTimeCommands(const VkCommandBuffer& commandBuffer, const VkCommandPool& commandPool);
			bool FindDestinationPosition(glm::vec3& destinationPoint);
			bool HasStencilComponent(VkFormat);
			void InitMaterials();
			void LoadAssets();
			void PrepeareMainCharacter(enginetool::ScenePart& mesh);
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
			struct UboParam {
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
			struct UboDataDynamic {
				glm::mat4* model = nullptr;
			} uboDataDynamic;

			// Struct that holds the models positions
			struct Constants {
				glm::vec4 renderLimitPlane;
				glm::vec3 pos;
				bool glow;
				glm::vec3 color;
				//alignas(16) glm::vec2
			};

			enginetool::Buffer m_UboLine;
			enginetool::Buffer m_UboSkybox;
			enginetool::Buffer m_UboSkyboxReflection;
			enginetool::Buffer m_UboSkyboxRefraction;
			enginetool::Buffer m_UboClouds;
			enginetool::Buffer m_UboCloudsDynamic;
			enginetool::Buffer m_UboOcean;
			enginetool::Buffer m_UboSlectionIndicator;
			enginetool::Buffer m_UboStillObjects;
			enginetool::Buffer m_UboParameters;
			enginetool::Buffer m_UboReflection;
			enginetool::Buffer m_UboReflectionParameters;
			enginetool::Buffer m_UboRefraction;
			enginetool::Buffer m_UboRefractionParameters;

			enginetool::Buffer m_VertexBuffersMeshLibraryObjects;
			enginetool::Buffer m_VertexBuffersSkybox;
			enginetool::Buffer m_VertexBuffersOcean;
			enginetool::Buffer m_VertexBuffersSelectRay;
			enginetool::Buffer m_VertexBuffersAABB;

			enginetool::Buffer m_IndexBuffersMeshLibraryObjects;
			enginetool::Buffer m_IndexBuffersSkybox;
			enginetool::Buffer m_IndexBuffersOcean;
			enginetool::Buffer m_IndexBuffersSelectRay;
			enginetool::Buffer m_IndexBuffersAABB;

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

			enginetool::SceneMaterial* sky = new enginetool::SceneMaterial();

			enginetool::ScenePart element;
			enginetool::ScenePart* selectionIndicatorMesh;

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

			GLFWwindow* p_Window = nullptr;
			Device* m_Device = nullptr;
			SwapChain* p_SwapChain = nullptr;
			RenderPass* p_ScreenRenderPass;
			RenderPass* p_OffScreenRenderPass;
			MaterialLibrary* materialLibrary = nullptr;
			MeshLibrary* m_MeshLibrary = nullptr;
			MousePicker* m_MousePicker = nullptr;
			enginetool::ThreadPool* threadPool = nullptr;
			WorldClock* mainClock = nullptr;
		};
	}
}