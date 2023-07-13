#include <algorithm> // "max" and "min" in VkExtent2D
#include <fstream>
#include <iostream>
#include <random>
#include <filesystem>

#include "LoadFile.cpp"
#include "ErrorCheck.hpp"
#include "Scene.hpp"

void* alignedAlloc(size_t size, size_t alignment)
{
	void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else 
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void alignedFree(void* data)
{
#if	defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else 
	free(data);
#endif
}

//---------- Constructors and dectructors ---------- //

Scene::Scene() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Scene object created\n";
#endif 
}

Scene::~Scene() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Scene object destroyed\n";
#endif
}

// --------------- Setters and getters -------------- //



// ---------------- Main functions ------------------ //

void Scene::InitScene(Device* device, GuiMainHub* guiMainHub, MousePicker* mousePicker, MeshLibrary* meshLibrary, MaterialLibrary* materialLibrary, WorldClock* mainClock, enginetool::ThreadPool& threadPool) {
	logicalDevice = device;
	this->guiMainHub = guiMainHub;
	this->mousePicker = mousePicker;
	this->meshLibrary = meshLibrary;
	this->materialLibrary = materialLibrary;
	this->threadPool = &threadPool;
	this->mainClock = mainClock;
	    
	selectionIndicatorMesh = &meshLibrary->meshes["SmallCoinB"];

	m_UboLine.setDevice(device);
	m_UboSkybox.setDevice(device);
	m_UboSkyboxReflection.setDevice(device);
	m_UboSkyboxRefraction.setDevice(device);
	m_UboClouds.setDevice(device);
	m_UboCloudsDynamic.setDevice(device);
	m_UboOcean.setDevice(device);
	m_UboSlectionIndicator.setDevice(device);
	m_UboStillObjects.setDevice(device);
	m_UboParameters.setDevice(device);
	m_UboReflection.setDevice(device);
	m_UboReflectionParameters.setDevice(device);
	m_UboRefraction.setDevice(device);
	m_UboRefractionParameters.setDevice(device);

	m_VertexBuffersMeshLibraryObjects.setDevice(device);
	m_VertexBuffersSkybox.setDevice(device);
	m_VertexBuffersOcean.setDevice(device);
	m_VertexBuffersSelectRay.setDevice(device);
	m_VertexBuffersAABB.setDevice(device);

	m_IndexBuffersMeshLibraryObjects.setDevice(device);
	m_IndexBuffersSkybox.setDevice(device);
	m_IndexBuffersOcean.setDevice(device);
	m_IndexBuffersSelectRay.setDevice(device);
    m_IndexBuffersAABB.setDevice(device);

	InitSwapchainImageViews();
	CreateCommandPool();
	CreateDepthResources();
	PrepareOffscreenImage();
	CreateFramebuffers();
	LoadAssets();
	CreateBuffers();
	CreateDescriptorPool();
	CreateDescriptorSetLayout();
	CreateDescriptorSet();
	CreateGraphicsPipeline();
	UpdateGUI();
	CreateCommandBuffers();
	CreateReflectionCommandBuffer();
	CreateRefractionCommandBuffer();
}

void Scene::UpdateScene() {
	UpdatePositions();

	std::vector<std::function<void()>> stageOne = {task1, task3, task4, task5, task6, task7, task8, task9, task10};
	ProcesTasksMultithreaded(threadPool, stageOne);

#if BUILD_ENABLE_VULKAN_DEBUG
	CreateCommandBuffers();
	CreateReflectionCommandBuffer();
	CreateRefractionCommandBuffer();
#else
	std::vector<std::function<void()>> stageTwo = {task11, task12, task13};
	ProcesTasksMultithreaded(threadPool, stageTwo); 
#endif 
}

void Scene::CleanUpForSwapchain() {
	CleanUpDepthResources();
	CleanUpOffscreenImage();
	DeInitFramebuffer();
	FreeCommandBuffers();
	DestroyPipeline();
}

void Scene::RecreateForSwapchain() {
	InitSwapchainImageViews();
	CreateDepthResources();
	PrepareOffscreenImage();
	CreateFramebuffers();
	UpdateDescriptorSet();
	CreateGraphicsPipeline();
	CreateCommandBuffers();
	CreateReflectionCommandBuffer();
	CreateRefractionCommandBuffer();

	mousePicker->width = (float)logicalDevice->swapchain_extent.width;
	mousePicker->height = (float)logicalDevice->swapchain_extent.height;
}

// ------------------ Swapchain --------------------- //

void Scene::InitSwapchainImageViews() {
	logicalDevice->swapchainImageViews.resize(logicalDevice->swapchainImages.size());

	for (size_t i = 0; i < logicalDevice->swapchainImages.size(); i++) {
		logicalDevice->swapchainImageViews[i] = CreateImageView(logicalDevice->swapchainImages[i], logicalDevice->swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

VkImageView Scene::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags) {//TODO use in swapchain textureLayout class
	VkImageViewCreateInfo ViewInfo = {};
	ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ViewInfo.image = image;
	ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	ViewInfo.format = format;
	ViewInfo.subresourceRange.aspectMask = aspect_flags;
	ViewInfo.subresourceRange.baseMipLevel = 0;
	ViewInfo.subresourceRange.levelCount = 1;
	ViewInfo.subresourceRange.baseArrayLayer = 0;
	ViewInfo.subresourceRange.layerCount = 1;

	VkImageView image_view;
	if (vkCreateImageView(logicalDevice->get(), &ViewInfo, nullptr, &image_view) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to create texture image view!");
		std::exit(-1);
	}

	return image_view;
}

// ----------------- Framebuffer -------------------- //

void Scene::CreateFramebuffers() {
	// Reflection frambuffer
	std::array<VkImageView, 2>  reflectionAttachments = {reflectionImage->view, reflectionDepthImage->view};
			
	VkFramebufferCreateInfo reflectionFramebufferInfo = {};
	reflectionFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	reflectionFramebufferInfo.renderPass = logicalDevice->offscreenRenderPass;
	reflectionFramebufferInfo.attachmentCount = static_cast<uint32_t>(reflectionAttachments.size());
	reflectionFramebufferInfo.pAttachments = reflectionAttachments.data();
	reflectionFramebufferInfo.width = logicalDevice->swapchain_extent.width;
	reflectionFramebufferInfo.height = logicalDevice->swapchain_extent.height;
	reflectionFramebufferInfo.layers = 1;

	ErrorCheck(vkCreateFramebuffer(logicalDevice->get(), &reflectionFramebufferInfo, nullptr, &logicalDevice->reflectionFramebuffer));

	// Refraction frambuffer
	std::array<VkImageView, 2>  refractionAttachments = {refractionImage->view, refractionDepthImage->view};

	VkFramebufferCreateInfo refractionFramebufferInfo = {};
	refractionFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	refractionFramebufferInfo.renderPass = logicalDevice->offscreenRenderPass;
	refractionFramebufferInfo.attachmentCount = static_cast<uint32_t>(refractionAttachments.size());
	refractionFramebufferInfo.pAttachments = refractionAttachments.data();
	refractionFramebufferInfo.width = logicalDevice->swapchain_extent.width;
	refractionFramebufferInfo.height = logicalDevice->swapchain_extent.height;
	refractionFramebufferInfo.layers = 1;

	ErrorCheck(vkCreateFramebuffer(logicalDevice->get(), &refractionFramebufferInfo, nullptr, &logicalDevice->refractionFramebuffer));

	// Screen frambuffer
	logicalDevice->swap_chain_framebuffers.resize(logicalDevice->swapchainImageViews.size());

	for (size_t i = 0; i < logicalDevice->swapchainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = { logicalDevice->swapchainImageViews[i], screenDepthImage->view };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = logicalDevice->renderPass; // specify with which render pass the framebuffer needs to be compatible
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = logicalDevice->swapchain_extent.width;
		framebufferInfo.height = logicalDevice->swapchain_extent.height;
		framebufferInfo.layers = 1; // swap chain images are single images,

		ErrorCheck(vkCreateFramebuffer(logicalDevice->get(), &framebufferInfo, nullptr, &logicalDevice->swap_chain_framebuffers[i]));
	}
}

// --------------- Command buffers ------------------ //

/* Operations in Vulkan that we want to execute, like drawing operations, need to be submitted to a queue.
These operations first need to be recorded into a VkCommandBuffer before they can be submitted.
These command buffers are allocated from a VkCommandPool that is associated with a specific queue family.
Because the image in the framebuffer depends on which specific image the swap chain will give us,
we need to record a command buffer for each possible image and select the right one at draw time. */

void Scene::CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = logicalDevice->FindQueueFamilies(logicalDevice->gpu);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow command buffers to be rerecorded individually, optional

	ErrorCheck(vkCreateCommandPool(logicalDevice->get(), &poolInfo, nullptr, &commandPool));
	ErrorCheck(vkCreateCommandPool(logicalDevice->get(), &poolInfo, nullptr, &refractionCommandPool));
	ErrorCheck(vkCreateCommandPool(logicalDevice->get(), &poolInfo, nullptr, &reflectionCommandPool));
}

VkCommandBuffer Scene::BeginSingleTimeCommands() {//TODO
	VkCommandBufferAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocInfo.commandPool = commandPool;
	AllocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDevice->get(), &AllocInfo, &commandBuffer);

	VkCommandBufferBeginInfo BeginInfo = {};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &BeginInfo);

	return commandBuffer;
}

void Scene::EndSingleTimeCommands(const VkCommandBuffer& commandBuffer, const VkCommandPool& commandPool) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(logicalDevice->queue, 1, &SubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(logicalDevice->queue);

	vkFreeCommandBuffers(logicalDevice->get(), commandPool, 1, &commandBuffer);
}

void Scene::copyBuffer(enginetool::Buffer* srcBuffer, enginetool::Buffer* dstBuffer, const VkDeviceSize size) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy bufferCopy = {};
	bufferCopy.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer->getBuffer(), dstBuffer->getBuffer(), 1, &bufferCopy);

	EndSingleTimeCommands(commandBuffer, commandPool);
}

// -------------- Graphics pipeline ----------------- //

void Scene::CreateGraphicsPipeline() {
	VkPushConstantRange PushConstantRange = {};
	PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	PushConstantRange.offset = 0;
	PushConstantRange.size = sizeof(Constants);
	assert(sizeof(Constants) <= logicalDevice->gpu_properties.limits.maxPushConstantsSize);

	VkPipelineInputAssemblyStateCreateInfo InputAssembly = {}; // describes what kind of geometry will be drawn from the vertices and if primitive restart should be enabled
	InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // triangle from every 3 vertices without reuse
	InputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo Rasterization = {}; // takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader
	Rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	Rasterization.depthClampEnable = VK_FALSE;
	Rasterization.rasterizerDiscardEnable = VK_FALSE; // geometry never passes through the rasterizer stage
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL; // determines how fragments are generated for geometry
	Rasterization.lineWidth = 1.0f;
	Rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
	Rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	Rasterization.depthBiasEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
	ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo ColorBlending = {};
	ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	ColorBlending.logicOpEnable = VK_FALSE;
	ColorBlending.logicOp = VK_LOGIC_OP_COPY;
	ColorBlending.attachmentCount = 1;
	ColorBlending.pAttachments = &ColorBlendAttachment;

	VkPipelineViewportStateCreateInfo ViewportState = {};
	ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	ViewportState.viewportCount = 1;
	ViewportState.pViewports = &viewport;
	ViewportState.scissorCount = 1;
	ViewportState.pScissors = &scissor;

	VkPipelineMultisampleStateCreateInfo Multisample = {}; // configures multisampling, is one of the ways to perform anti-aliasing
	Multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	Multisample.sampleShadingEnable = VK_FALSE;
	Multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo DepthStencil = {};
	DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	DepthStencil.depthTestEnable = VK_TRUE;
	DepthStencil.depthWriteEnable = VK_TRUE;
	DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // if you change this, materials will have the same shader
	
	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo ViewportDynamic = {};
	ViewportDynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	ViewportDynamic.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	ViewportDynamic.pDynamicStates = dynamicStateEnables.data();

	auto bindingDescription = enginetool::VertexLayout::getBindingDescription();
	auto attributeDescriptions = enginetool::VertexLayout::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
	VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputInfo.vertexBindingDescriptionCount = 1; //  is the number of vertex binding descriptions provided in pVertexBindingDescriptions
	VertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	VertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	std::array<VkDescriptorSetLayout, 7> layouts = { descriptor_set_layout, 
													skybox_descriptor_set_layout, 
													cloudDescriptorSetLayout, 
													oceanDescriptorSetLayout, 
													lineDescriptorSetLayout,
													selectionIndicatorDescriptorSetLayout,
													aabbDescriptorSetLayout 
													};

	VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
	PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutInfo.pushConstantRangeCount = 1;
	PipelineLayoutInfo.pPushConstantRanges = &PushConstantRange;
	PipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	PipelineLayoutInfo.pSetLayouts = layouts.data();
	
	ErrorCheck(vkCreatePipelineLayout(logicalDevice->get(), &PipelineLayoutInfo, nullptr, &pipelineLayout));

	std::array <VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo PipelineInfo = {};
	PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	PipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	PipelineInfo.pStages = shaderStages.data();
	PipelineInfo.pVertexInputState = &VertexInputInfo;
	PipelineInfo.pInputAssemblyState = &InputAssembly;
	PipelineInfo.pViewportState = &ViewportState;
	PipelineInfo.pRasterizationState = &Rasterization;
	PipelineInfo.pMultisampleState = &Multisample;
	PipelineInfo.pDepthStencilState = &DepthStencil;
	PipelineInfo.pColorBlendState = &ColorBlending;
	PipelineInfo.pDynamicState = &ViewportDynamic;
	PipelineInfo.layout = pipelineLayout;
	PipelineInfo.renderPass = logicalDevice->renderPass;
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	// I. Skybox reflect pipeline
	std::filesystem::path p = std::filesystem::current_path().parent_path();
	std::filesystem::path vertCubeMapShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "skymap_shader.vert.spv";
	std::filesystem::path fragCubeMapShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "skymap_shader.frag.spv";

	auto vertCubeMapShaderCode = enginetool::readFile(vertCubeMapShaderCodePath.string());
	auto fragCubeMapShaderCode = enginetool::readFile(fragCubeMapShaderCodePath.string()); 

	VkShaderModule vertCubeMapShaderModule = logicalDevice->CreateShaderModule(vertCubeMapShaderCode);
	VkShaderModule fragCubeMapShaderModule = logicalDevice->CreateShaderModule(fragCubeMapShaderCode);

	VkPipelineShaderStageCreateInfo vertCubeMapShaderStageInfo = {};
	vertCubeMapShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertCubeMapShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertCubeMapShaderStageInfo.module = vertCubeMapShaderModule;
	vertCubeMapShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragCubeMapShaderStageInfo = {};
	fragCubeMapShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragCubeMapShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragCubeMapShaderStageInfo.module = fragCubeMapShaderModule;
	fragCubeMapShaderStageInfo.pName = "main";

	shaderStages[0] = vertCubeMapShaderStageInfo;
	shaderStages[1] = fragCubeMapShaderStageInfo;

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &skyboxPipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_LINE; 
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &skyboxWireframePipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL; 

	// Skybox refraction pipeline
	PipelineInfo.renderPass = logicalDevice->offscreenRenderPass;
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &skyboxRefractionPipeline));

	// Skybox reflection pipeline
	Rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	PipelineInfo.renderPass = logicalDevice->offscreenRenderPass;
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &skyboxReflectionPipeline));
	
	Rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
	PipelineInfo.renderPass = logicalDevice->renderPass;

	vkDestroyShaderModule(logicalDevice->get(), fragCubeMapShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->get(), vertCubeMapShaderModule, nullptr);
	
	// II. Models pipeline
	std::filesystem::path vertModelsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "pbr_shader.vert.spv";
	std::filesystem::path fragModelsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "pbr_shader.frag.spv";

	auto vertModelsShaderCode = enginetool::readFile(vertModelsShaderCodePath.string());
	auto fragModelsShaderCode = enginetool::readFile(fragModelsShaderCodePath.string());

	VkShaderModule vertModelsShaderModule = logicalDevice->CreateShaderModule(vertModelsShaderCode);
	VkShaderModule fragModelsShaderModule = logicalDevice->CreateShaderModule(fragModelsShaderCode);

	VkPipelineShaderStageCreateInfo vertModelsShaderStageInfo = {};
	vertModelsShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertModelsShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertModelsShaderStageInfo.module = vertModelsShaderModule;
	vertModelsShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragModelsShaderStageInfo = {};
	fragModelsShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragModelsShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragModelsShaderStageInfo.module = fragModelsShaderModule;
	fragModelsShaderStageInfo.pName = "main";

	shaderStages[0] = vertModelsShaderStageInfo;
	shaderStages[1] = fragModelsShaderStageInfo;

	DepthStencil.depthTestEnable = VK_TRUE;
	DepthStencil.depthWriteEnable = VK_TRUE;

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &pbrPipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_LINE; 
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &pbrWireframePipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	
	// Models refraction pipeline
	PipelineInfo.renderPass = logicalDevice->offscreenRenderPass;
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &pbrRefractionPipeline));

	// Models reflection pipeline
	Rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	PipelineInfo.renderPass = logicalDevice->offscreenRenderPass;
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &pbrReflectionPipeline));

	vkDestroyShaderModule(logicalDevice->get(), fragModelsShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->get(), vertModelsShaderModule, nullptr);
	
	Rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
	PipelineInfo.renderPass = logicalDevice->renderPass;

	// III. Selection crystal pipeline
	std::filesystem::path vertSelectionCrystalShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "selectionCrystalShader.vert.spv";
	std::filesystem::path fragSelectionCrystalShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "selectionCrystalShader.frag.spv";

	auto vertSelectionCrystalShaderCode = enginetool::readFile(vertSelectionCrystalShaderCodePath.string());
	auto fragSelectionCrystalShaderCode = enginetool::readFile(fragSelectionCrystalShaderCodePath.string());

	VkShaderModule vertSelectionCrystalShaderModule = logicalDevice->CreateShaderModule(vertSelectionCrystalShaderCode);
	VkShaderModule fragSelectionCrystalShaderModule = logicalDevice->CreateShaderModule(fragSelectionCrystalShaderCode);

	VkPipelineShaderStageCreateInfo vertSelectionCrystalShaderStageInfo = {};
	vertSelectionCrystalShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertSelectionCrystalShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertSelectionCrystalShaderStageInfo.module = vertSelectionCrystalShaderModule;
	vertSelectionCrystalShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragSelectionCrystalShaderStageInfo = {};
	fragSelectionCrystalShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragSelectionCrystalShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragSelectionCrystalShaderStageInfo.module = fragSelectionCrystalShaderModule;
	fragSelectionCrystalShaderStageInfo.pName = "main";

	shaderStages[0] = vertSelectionCrystalShaderStageInfo;
	shaderStages[1] = fragSelectionCrystalShaderStageInfo;

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &selectionIndicatorPipeline));

	vkDestroyShaderModule(logicalDevice->get(), fragSelectionCrystalShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->get(), vertSelectionCrystalShaderModule, nullptr);

	// IV. Ocean pipeline
	std::filesystem::path vertOceanShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "ocean_shader.vert.spv";
	std::filesystem::path fragOceanShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "ocean_shader.frag.spv";

	auto vertOceanShaderCode = enginetool::readFile(vertOceanShaderCodePath.string());
	auto fragOceanShaderCode = enginetool::readFile(fragOceanShaderCodePath.string());

	VkShaderModule vertOceanShaderModule = logicalDevice->CreateShaderModule(vertOceanShaderCode);
	VkShaderModule fragOceanShaderModule = logicalDevice->CreateShaderModule(fragOceanShaderCode);

	VkPipelineShaderStageCreateInfo vertOceanShaderStageInfo = {};
	vertOceanShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertOceanShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertOceanShaderStageInfo.module = vertOceanShaderModule;
	vertOceanShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragOceanShaderStageInfo = {};
	fragOceanShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragOceanShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragOceanShaderStageInfo.module = fragOceanShaderModule;
	fragOceanShaderStageInfo.pName = "main";

	shaderStages[0] = vertOceanShaderStageInfo;
	shaderStages[1] = fragOceanShaderStageInfo;

	Rasterization.cullMode = VK_CULL_MODE_NONE;

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &oceanPipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_LINE; 
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &oceanWireframePipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL; 

	vkDestroyShaderModule(logicalDevice->get(), fragOceanShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->get(), vertOceanShaderModule, nullptr);
	
	// V. Line pipeline
	std::filesystem::path vertLineShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "lineShader.vert.spv";
	std::filesystem::path fragLineShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "lineShader.frag.spv";

	auto vertLineShaderCode = enginetool::readFile(vertLineShaderCodePath.string());
	auto fragLineShaderCode = enginetool::readFile(fragLineShaderCodePath.string());

	VkShaderModule vertLineShaderModule = logicalDevice->CreateShaderModule(vertLineShaderCode);
	VkShaderModule fragLineShaderModule = logicalDevice->CreateShaderModule(fragLineShaderCode);

	VkPipelineShaderStageCreateInfo vertLineShaderStageInfo = {};
	vertLineShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertLineShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertLineShaderStageInfo.module = vertLineShaderModule;
	vertLineShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragLineShaderStageInfo = {};
	fragLineShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragLineShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragLineShaderStageInfo.module = fragLineShaderModule;
	fragLineShaderStageInfo.pName = "main";

	shaderStages[0] = vertLineShaderStageInfo;
	shaderStages[1] = fragLineShaderStageInfo;

	Rasterization.polygonMode = VK_POLYGON_MODE_LINE; 
	Rasterization.lineWidth = 2.0f;
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &selectRayPipeline));
	Rasterization.lineWidth = 1.0f;
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL;

	vkDestroyShaderModule(logicalDevice->get(), fragLineShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->get(), vertLineShaderModule, nullptr); 

	// VI. Aabb pipeline
	std::filesystem::path vertAabbShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "aabbShader.vert.spv";
	std::filesystem::path fragAabbShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "aabbShader.frag.spv";

	auto vertAabbShaderCode = enginetool::readFile(vertAabbShaderCodePath.string());
	auto fragAabbShaderCode = enginetool::readFile(fragAabbShaderCodePath.string());

	VkShaderModule vertAabbShaderModule = logicalDevice->CreateShaderModule(vertAabbShaderCode);
	VkShaderModule fragAabbShaderModule = logicalDevice->CreateShaderModule(fragAabbShaderCode);

	VkPipelineShaderStageCreateInfo vertAabbShaderStageInfo = {};
	vertAabbShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertAabbShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertAabbShaderStageInfo.module = vertAabbShaderModule;
	vertAabbShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragAabbShaderStageInfo = {};
	fragAabbShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragAabbShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragAabbShaderStageInfo.module = fragAabbShaderModule;
	fragAabbShaderStageInfo.pName = "main";

	shaderStages[0] = vertAabbShaderStageInfo;
	shaderStages[1] = fragAabbShaderStageInfo;

	Rasterization.polygonMode = VK_POLYGON_MODE_LINE; 
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &aabbPipeline));
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL;

	vkDestroyShaderModule(logicalDevice->get(), fragAabbShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->get(), vertAabbShaderModule, nullptr); 

	// VI. Clouds pipeline
	std::filesystem::path vertCloudsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "clouds_shader.vert.spv";
	std::filesystem::path fragCloudsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "clouds_shader.frag.spv";

	auto vertCloudsShaderCode = enginetool::readFile(vertCloudsShaderCodePath.string());
	auto fragCloudsShaderCode = enginetool::readFile(fragCloudsShaderCodePath.string());

	VkShaderModule vertCloudsShaderModule = logicalDevice->CreateShaderModule(vertCloudsShaderCode);
	VkShaderModule fragCloudsShaderModule = logicalDevice->CreateShaderModule(fragCloudsShaderCode);

	VkPipelineShaderStageCreateInfo vertCloudsShaderStageInfo = {};
	vertCloudsShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertCloudsShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertCloudsShaderStageInfo.module = vertCloudsShaderModule;
	vertCloudsShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragCloudsShaderStageInfo = {};
	fragCloudsShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragCloudsShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragCloudsShaderStageInfo.module = fragCloudsShaderModule;
	fragCloudsShaderStageInfo.pName = "main";

	shaderStages[0] = vertCloudsShaderStageInfo;
	shaderStages[1] = fragCloudsShaderStageInfo;

	ColorBlendAttachment.blendEnable = VK_TRUE;
	ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &cloudsPipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_LINE; 
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->get(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &cloudsWireframePipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL; 

	vkDestroyShaderModule(logicalDevice->get(), fragCloudsShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->get(), vertCloudsShaderModule, nullptr);
}

void Scene::ProcesTasksMultithreaded(enginetool::ThreadPool* threadPool, std::vector<std::function<void()>>& tasks){
	while(!tasks.empty()){
		size_t i = 0;		
		while (i < threadPool->threads.size()-1 && !tasks.empty()) {
			threadPool->threads[i]->AddJob(tasks.back());
			tasks.pop_back();
			++i;
		}		
	}

	threadPool->Hold();
}

void Scene::CreateCommandBuffers() {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = {  0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = logicalDevice->renderPass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = logicalDevice->swapchain_extent.width;
	renderPassInfo.renderArea.extent.height = logicalDevice->swapchain_extent.height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	
	commandBuffers.resize(logicalDevice->swap_chain_framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if the allocated command buffers are primary or secondary, here "primary" can be submitted to a queue for execution, but cannot be called from other command buffers
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	ErrorCheck(vkAllocateCommandBuffers(logicalDevice->get(), &allocInfo, commandBuffers.data()));

	// starting command buffer recording
	for (size_t i = 0; i < commandBuffers.size(); i++)	{
		// Set target frame buffer
		renderPassInfo.framebuffer = logicalDevice->swap_chain_framebuffers[i];
		vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)logicalDevice->swapchain_extent.width;
		viewport.height = (float)logicalDevice->swapchain_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);

		scissor.offset = { 0, 0 }; // scissor rectangle covers framebuffer entirely
		scissor.extent = logicalDevice->swapchain_extent;
		vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

		VkDeviceSize offsets[1] = { 0 };

		if(displaySelectionIndicator && selectedActor!=nullptr) {
			float pointerOffset = selectedActor->position.y + abs(selectedActor->assignedMesh->aabb.max.y)+abs(selectionIndicatorMesh->aabb.max.y)+0.25f;
			pushConstants[0].renderLimitPlane = glm::vec4(0.0f, 0.0f, 0.0f, horizon );
			pushConstants[0].color = selectedActor->CalculateSelectionIndicatorColor();
			pushConstants[0].pos = glm::vec3(selectedActor->position.x, pointerOffset, selectedActor->position.z);
			vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Constants), &pushConstants[0]);
			
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 5, 1, &selectionIndicatorDescriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &m_VertexBuffersMeshLibraryObjects.getBuffer(), offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], m_IndexBuffersMeshLibraryObjects.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, selectionIndicatorPipeline);
			vkCmdDrawIndexed(commandBuffers[i], selectionIndicatorMesh->indexCount, 1, 0,  selectionIndicatorMesh->indexBase, 0);
		}

		if (displaySkybox) {
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &skybox_descriptor_set, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &m_VertexBuffersSkybox.getBuffer(), offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], m_IndexBuffersSkybox.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (displayWireframe) ? (skyboxWireframePipeline) : (skyboxPipeline));
			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(std::dynamic_pointer_cast<Skybox>(skyboxes[0])->indices.size()), 1, 0, 0, 0);
		}

		if (displayMainCharacter) {
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &m_VertexBuffersMeshLibraryObjects.getBuffer(), offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], m_IndexBuffersMeshLibraryObjects.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
			std::array<VkDescriptorSet, 1> descriptorSets;
			descriptorSets[0] = mainCharacter->assignedMaterial->descriptorSet;
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (displayWireframe) ? (pbrWireframePipeline) : (*mainCharacter->assignedMaterial->assignedPipeline));
			pushConstants[0].pos = mainCharacter->position;
			pushConstants[0].renderLimitPlane = glm::vec4(0.0f, 0.0f, 0.0f, horizon);
			vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Constants), &pushConstants[0]);
			vkCmdDrawIndexed(commandBuffers[i], mainCharacter->assignedMesh->indexCount, 1, 0, mainCharacter->assignedMesh->indexBase, 0);
		}

		if (displayOcean) {
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1, &oceanDescriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &m_VertexBuffersOcean.getBuffer(), offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], m_IndexBuffersOcean.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (displayWireframe) ? (oceanWireframePipeline) : (oceanPipeline));
			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(std::dynamic_pointer_cast<Sea>(seas[0])->indices.size()), 1, 0, 0, 0);
		}

		if (displaySceneGeometry) {
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &m_VertexBuffersMeshLibraryObjects.getBuffer(), offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], m_IndexBuffersMeshLibraryObjects.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

			for (const auto& a : actors) {
				if(a->visible) {
					std::array<VkDescriptorSet, 1> descriptorSets;
					descriptorSets[0] = a->assignedMaterial->descriptorSet;
					vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
					vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (displayWireframe) ? (pbrWireframePipeline) : (*a->assignedMaterial->assignedPipeline));
					pushConstants[0].pos = a->position;
					pushConstants[0].renderLimitPlane = glm::vec4(0.0f, 0.0f, 0.0f, horizon );
					vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Constants), &pushConstants[0]);
					vkCmdDrawIndexed(commandBuffers[i], a->assignedMesh->indexCount, 1, 0, a->assignedMesh->indexBase, 0);
				}
			}
		}

		if (displayClouds)	{
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &m_VertexBuffersMeshLibraryObjects.getBuffer(), offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], m_IndexBuffersMeshLibraryObjects.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (displayWireframe) ? (cloudsWireframePipeline) : (cloudsPipeline));
			
			for (uint32_t k = 0; k < DYNAMIC_UB_OBJECTS; k++) {
				uint32_t dynamic_offset = k * static_cast<uint32_t>(dynamicAlignment);
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &cloudDescriptorSet, 1, &dynamic_offset);
				vkCmdDrawIndexed(commandBuffers[i], clouds[0]->assignedMesh->indexCount, 1, 0, clouds[0]->assignedMesh->indexBase, 0);			
			}
		}

		if(displayWireframe) {
			UpdateSelectRayDrawData();
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 4, 1, &lineDescriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &m_VertexBuffersSelectRay.getBuffer(), offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], m_IndexBuffersSelectRay.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, selectRayPipeline);
			vkCmdDrawIndexed(commandBuffers[i], 2, 1, 0, 0, 0);
		}

		if(displayAabb) {
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &m_VertexBuffersAABB.getBuffer(), offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], m_IndexBuffersAABB.getBuffer() , 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 6, 1, &aabbDescriptorSet, 0, nullptr);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, aabbPipeline);
			for (const auto& a : actors) {
				if(a->visible) {
					pushConstants[0].pos = a->position;
					vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Constants), &pushConstants[0]);
					vkCmdDrawIndexed(commandBuffers[i], 24, 1, 0, a->assignedMesh->indexBaseAabb, 0);
				}
			}
		}

		vkCmdEndRenderPass(commandBuffers[i]);
		ErrorCheck(vkEndCommandBuffer(commandBuffers[i]));
	}
}

void Scene::CreateReflectionCommandBuffer() {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = logicalDevice->offscreenRenderPass;
	renderPassInfo.framebuffer = logicalDevice->reflectionFramebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = logicalDevice->swapchain_extent.width;
	renderPassInfo.renderArea.extent.height = logicalDevice->swapchain_extent.height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = reflectionCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if the allocated command buffers are primary or secondary, here "primary" can be submitted to a queue for execution, but cannot be called from other command buffers
	allocInfo.commandBufferCount = 1;

	ErrorCheck(vkAllocateCommandBuffers(logicalDevice->get(), &allocInfo, &reflectionCmdBuff));
	
	ErrorCheck(vkBeginCommandBuffer(reflectionCmdBuff, &beginInfo));
	vkCmdBeginRenderPass(reflectionCmdBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)logicalDevice->swapchain_extent.width;
	viewport.height = (float)logicalDevice->swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(reflectionCmdBuff, 0, 1, &viewport);

	scissor.offset = { 0, 0 }; // scissor rectangle covers framebuffer entirely
	scissor.extent.height = logicalDevice->swapchain_extent.height;
	scissor.extent.width = logicalDevice->swapchain_extent.width;
	vkCmdSetScissor(reflectionCmdBuff, 0, 1, &scissor);

	VkDeviceSize offsets[1] = { 0 };

	// Skybox
	if (displaySkybox)	{
		vkCmdBindDescriptorSets(reflectionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &skyboxReflectionDescriptorSet, 0, nullptr);
		vkCmdBindVertexBuffers(reflectionCmdBuff, 0, 1, &m_VertexBuffersSkybox.getBuffer(), offsets);
		vkCmdBindIndexBuffer(reflectionCmdBuff, m_IndexBuffersSkybox.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(reflectionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxReflectionPipeline);
		vkCmdDrawIndexed(reflectionCmdBuff, static_cast<uint32_t>(std::dynamic_pointer_cast<Skybox>(skyboxes[0])->indices.size()), 1, 0, 0, 0);
	}

	// 3d object
	vkCmdBindVertexBuffers(reflectionCmdBuff, 0, 1, &m_VertexBuffersMeshLibraryObjects.getBuffer(), offsets);
	vkCmdBindIndexBuffer(reflectionCmdBuff, m_IndexBuffersMeshLibraryObjects.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	for (size_t j = 0; j < actors.size(); j++) {
		// reflection
		std::array<VkDescriptorSet, 1> descriptorSets;
		descriptorSets[0] = actors[j]->assignedMaterial->reflectDescriptorSet;
		vkCmdBindDescriptorSets(reflectionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
		vkCmdBindPipeline(reflectionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pbrReflectionPipeline);
		
		pushConstants[1].pos = actors[j]->position;
		pushConstants[1].renderLimitPlane = (currentCamera->position.y<0) ? (glm::vec4(0.0f, -1.0f, 0.0f, -0.0f)) : (glm::vec4(0.0f, 1.0f, 0.0f, -0.0f));
		vkCmdPushConstants(reflectionCmdBuff, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Constants), &pushConstants[1]);
		vkCmdDrawIndexed(reflectionCmdBuff, actors[j]->assignedMesh->indexCount, 1, 0, actors[j]->assignedMesh->indexBase, 0);
	}

	vkCmdEndRenderPass(reflectionCmdBuff);
	ErrorCheck(vkEndCommandBuffer(reflectionCmdBuff));
}

void Scene::CreateRefractionCommandBuffer() { 
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = logicalDevice->offscreenRenderPass;
	renderPassInfo.framebuffer = logicalDevice->refractionFramebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = logicalDevice->swapchain_extent.width;
	renderPassInfo.renderArea.extent.height = logicalDevice->swapchain_extent.height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = refractionCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if the allocated command buffers are primary or secondary, here "primary" can be submitted to a queue for execution, but cannot be called from other command buffers
	allocInfo.commandBufferCount = 1;

	ErrorCheck(vkAllocateCommandBuffers(logicalDevice->get(), &allocInfo, &refractionCmdBuff));
	
	ErrorCheck(vkBeginCommandBuffer(refractionCmdBuff, &beginInfo));
	vkCmdBeginRenderPass(refractionCmdBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)logicalDevice->swapchain_extent.width;
	viewport.height = (float)logicalDevice->swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(refractionCmdBuff, 0, 1, &viewport);

	scissor.offset = { 0, 0 }; // scissor rectangle covers framebuffer entirely
	scissor.extent.height = logicalDevice->swapchain_extent.height;
	scissor.extent.width = logicalDevice->swapchain_extent.width;
	vkCmdSetScissor(refractionCmdBuff, 0, 1, &scissor);

	VkDeviceSize offsets[1] = { 0 };

	// Skybox
	if (displaySkybox)	{
		vkCmdBindDescriptorSets(refractionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &skyboxRefractionDescriptorSet, 0, nullptr);
		vkCmdBindVertexBuffers(refractionCmdBuff, 0, 1, &m_VertexBuffersSkybox.getBuffer(), offsets);
		vkCmdBindIndexBuffer(refractionCmdBuff, m_IndexBuffersSkybox.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(refractionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxRefractionPipeline);
		vkCmdDrawIndexed(refractionCmdBuff, static_cast<uint32_t>(std::dynamic_pointer_cast<Skybox>(skyboxes[0])->indices.size()), 1, 0, 0, 0);
	}

	// 3d object
	vkCmdBindVertexBuffers(refractionCmdBuff, 0, 1, &m_VertexBuffersMeshLibraryObjects.getBuffer(), offsets);
	vkCmdBindIndexBuffer(refractionCmdBuff, m_IndexBuffersMeshLibraryObjects.getBuffer() , 0, VK_INDEX_TYPE_UINT32);

	for (size_t j = 0; j < actors.size(); j++) {
		std::array<VkDescriptorSet, 1> descriptorSets;
		descriptorSets[0] = actors[j]->assignedMaterial->refractDescriptorSet;
		vkCmdBindDescriptorSets(refractionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
		vkCmdBindPipeline(refractionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pbrRefractionPipeline);
		
		pushConstants[2].pos = actors[j]->position;
		pushConstants[2].renderLimitPlane = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f );
		vkCmdPushConstants(refractionCmdBuff, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Constants), &pushConstants[2]);
		vkCmdDrawIndexed(refractionCmdBuff, actors[j]->assignedMesh->indexCount, 1, 0, actors[j]->assignedMesh->indexBase, 0);
	}

	vkCmdEndRenderPass(refractionCmdBuff);
	ErrorCheck(vkEndCommandBuffer(refractionCmdBuff));
}

void Scene::CreateBuffers() {
	CreateVertexBuffer(meshLibrary->vertices, m_VertexBuffersMeshLibraryObjects);
	CreateIndexBuffer(meshLibrary->indices, m_IndexBuffersMeshLibraryObjects);
	
	CreateVertexBuffer(meshLibrary->aabbVertices, m_VertexBuffersAABB);
	CreateIndexBuffer(meshLibrary->aabbIndices, m_IndexBuffersAABB);

	// Ocean Uniform buffers memory -> static
	m_UboOcean.createUnstagedBuffer(sizeof(UboSea), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboOcean.map(sizeof(UboSea));
	
	// Clouds Uniform buffers memory -> dynamic
	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = logicalDevice->gpu_properties.limits.minUniformBufferOffsetAlignment;

	dynamicAlignment = sizeof(glm::mat4);

	if (minUboAlignment > 0) {
		dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}

	size_t bufferSize = DYNAMIC_UB_OBJECTS * dynamicAlignment;
	uboDataDynamic.model = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment);

	if (uboDataDynamic.model == VK_NULL_HANDLE)	{
		assert(0 && "Vulkan ERROR: Can't create host memory for the dynamic uniform buffer");
		std::exit(-1);
	}

	std::cout << "minUniformBufferOffsetAlignment = " << minUboAlignment << std::endl;
	std::cout << "dynamicAlignment = " << dynamicAlignment << std::endl;
		
    // Uniform buffer object with per-object matrices
	m_UboCloudsDynamic.createUnstagedBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	m_UboCloudsDynamic.map(bufferSize);

	// Static shared uniform buffer object with projection and view matrix
	m_UboClouds.createUnstagedBuffer(sizeof(UboClouds), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboClouds.map(sizeof(UboClouds));

	// Rotating Objects Uniform buffers memory -> static
	m_UboSlectionIndicator.createUnstagedBuffer(sizeof(UboSelectionIndicator), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboSlectionIndicator.map(sizeof(UboSelectionIndicator));

	// Objects Uniform buffers memory -> static
	m_UboStillObjects.createUnstagedBuffer(sizeof(UboStaticGeometry), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboStillObjects.map(sizeof(UboStaticGeometry));

	// Objects Uniform buffers for reflection rendering -> static
	m_UboReflection.createUnstagedBuffer(sizeof(UboStaticGeometry), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboReflection.map(sizeof(UboStaticGeometry));

	// Objects Uniform buffers for refraction rendering -> static
	m_UboRefraction.createUnstagedBuffer(sizeof(UboStaticGeometry), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboRefraction.map(sizeof(UboStaticGeometry));

	// Additional uniform bufer for parameters -> static
	m_UboParameters.createUnstagedBuffer(sizeof(UboParam), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboParameters.map(sizeof(UboParam));

	// Additional uniform bufer parameters for reflection scene -> static
	m_UboReflectionParameters.createUnstagedBuffer(sizeof(UboParam), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboReflectionParameters.map(sizeof(UboParam));

	// Additional uniform bufer parameters for refraction scene -> static
	m_UboRefractionParameters.createUnstagedBuffer(sizeof(UboParam), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboRefractionParameters.map(sizeof(UboParam));

	// Skybox Uniform buffers memory -> static
	m_UboSkybox.createUnstagedBuffer(sizeof(UboSkybox), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboSkybox.map(sizeof(UboSkybox));

	m_UboSkyboxReflection.createUnstagedBuffer(sizeof(UboSkybox), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboSkyboxReflection.map(sizeof(UboSkybox));

	m_UboSkyboxRefraction.createUnstagedBuffer(sizeof(UboSkybox), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboSkyboxRefraction.map(sizeof(UboSkybox));

	// Line uniform buffer -> static
	m_UboLine.createUnstagedBuffer(sizeof(UboStaticGeometry), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_UboLine.map(sizeof(UboStaticGeometry));

	// Create random positions for dynamic uniform buffer
	RandomPositions();
}

void Scene::RandomPositions() {
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<double> dist(-cloudsVisibDist/2, cloudsVisibDist/2);
	for (uint32_t i = 0; i < DYNAMIC_UB_OBJECTS; i++) {
		rnd_pos[i] = glm::vec3(dist(mt));
	}
}

void Scene::UpdatePositions() {
	for(const auto& a : actors) a->UpdatePosition((float)mainClock->fixedTimeValue);
	for(const auto& c : sceneCameras) c->UpdatePosition((float)mainClock->fixedTimeValue);
	mainCharacter->UpdatePosition((float)mainClock->fixedTimeValue);
}

void Scene::HandleMouseClick() {
	if(selectedActor == nullptr) {
		SelectActor();
		std::cout << "Selected"<< std::endl;
	}
	else {
		glm::vec3 indicatedTarget = selectedActor->position; 
		if(FindDestinationPosition(indicatedTarget)) {
			selectedActor->destinationPoint = indicatedTarget;
			selectedActor->ChangePosition();
			std::cout << "Position changed"<< std::endl;
		}
	}
}

void Scene::CheckActorsVisibility(){
	for(auto& a : actors) CheckIfItIsVisible(a);
}

void Scene::CheckIfItIsVisible(std::shared_ptr<Actor>& actorToCheck) {
	float distance = glm::distance(mainCharacter->position, actorToCheck->position);
	if(distance>3000.0f) 
		actorToCheck->visible=false;
	else actorToCheck->visible=true;
}

void Scene::SelectActor() {
	glm::vec3 dirFrac;
	dirFrac.x = 1.0f / mousePicker->GetRayDirection().x;
	dirFrac.y = 1.0f / mousePicker->GetRayDirection().y;
	dirFrac.z = 1.0f / mousePicker->GetRayDirection().z;
	for (auto const& a : actors) {
		if(enginetool::ScenePart::RayIntersection(mousePicker->hitPoint, dirFrac, mousePicker->GetRayOrigin(), mousePicker->GetRayDirection(), a->currentAabb)) {
			std::cout << "Hit point: " << mousePicker->hitPoint.x << " " << mousePicker->hitPoint.y << " " << mousePicker->hitPoint.z << "\n";
			selectedActor=a;
			std::cout << "Selected object: " << selectedActor->name << std::endl;
			break;
		}
	}
}

bool Scene::FindDestinationPosition(glm::vec3& destinationPoint) {
	glm::vec3 dirFrac;
	dirFrac.x = 1.0f / mousePicker->GetRayDirection().x;
	dirFrac.y = 1.0f / mousePicker->GetRayDirection().y;
	dirFrac.z = 1.0f / mousePicker->GetRayDirection().z;

	glm::vec3 rayOrigin = mousePicker->GetRayOrigin();

	for (auto const& a : actors) {
		if(enginetool::ScenePart::RayIntersection(mousePicker->hitPoint, dirFrac, rayOrigin, mousePicker->GetRayDirection(), a->currentAabb) && selectedActor!=a) {
			destinationPoint = mousePicker->hitPoint;
			std::cout << "Position found"<< std::endl;
			return true;
		}
	}
	return false;	
}

void Scene::DeSelect() {
	//selectedActor->movementGoal = glm::vec3(0.0f, 0.0f, 0.0f);
	selectedActor = nullptr;
	std::cout << "Deselected" << std::endl;
}

void Scene::UpdateStaticUniformBuffer() {
	UBOSG.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOSG.proj[1][1] *= -1; //since the Y axis of Vulkan NDC points down
	UBOSG.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOSG.model = glm::mat4(1.0f);
	UBOSG.cameraPos = glm::vec3(currentCamera->position);
	memcpy(m_UboStillObjects.getMapped(), &UBOSG, sizeof(UBOSG));
	memcpy(m_UboLine.getMapped(), &UBOSG, sizeof(UBOSG));
	mousePicker->UpdateMousePicker(UBOSG.view, UBOSG.proj, currentCamera);

}

void Scene::UpdateCloudsUniformBuffer() {
	UBOC.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOC.proj[1][1] *= -1; 
	UBOC.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOC.time = (float)mainClock->totalTime;
	UBOC.view[3][0] *= 0;
	UBOC.view[3][1] *= 0;
	UBOC.view[3][2] *= 0;
	UBOC.model = glm::mat4(1.0f);
	UBOC.cameraPos = currentCamera->position;
	memcpy(m_UboClouds.getMapped(), &UBOC, sizeof(UBOC));	
} 

void Scene::UpdateSelectionIndicatorUniformBuffer() {
	UBOSI.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOSI.proj[1][1] *= -1; 
	UBOSI.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOSI.model = glm::rotate(glm::mat4(1.0f), (float)mainClock->totalTime * glm::radians(90.0f), currentCamera->up);
	UBOSI.cameraPos = glm::vec3(currentCamera->position);
	UBOSI.time = (float)mainClock->totalTime;
	memcpy(m_UboSlectionIndicator.getMapped(), &UBOSI, sizeof(UBOSI));
}

void Scene::UpdateOffscreenUniformBuffer() {
	UBOO.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOO.proj[1][1] *= -1; 
	UBOO.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOO.model = glm::mat4(1.0f);
	UBOO.cameraPos = glm::vec3(currentCamera->position);
	memcpy(m_UboRefraction.getMapped(), &UBOO, sizeof(UBOO));
	UBOO.view[1][0] *= -1;
	UBOO.view[1][1] *= -1;
	UBOO.view[1][2] *= -1;
	UBOO.cameraPos *= glm::vec3(1.0f, -1.0f, 1.0f);
	memcpy(m_UboReflection.getMapped(), &UBOO, sizeof(UBOO));
}

void Scene::UpdateUniformBufferParameters() {
	UBOP.light_col = std::dynamic_pointer_cast<SphereLight>(actors[2])->GetLightColor();
	UBOP.exposure = 2.5f;
	UBOP.light_pos[0] = actors[2]->position;
	memcpy(m_UboParameters.getMapped(), &UBOP, sizeof(UBOP));
	memcpy(m_UboRefractionParameters.getMapped(), &UBOP, sizeof(UBOP));

	UBOP.light_pos[0]*= glm::vec3(1.0f, -1.0f, 1.0f);
	memcpy(m_UboReflectionParameters.getMapped(), &UBOP, sizeof(UBOP));
}

void Scene::UpdateDynamicUniformBuffer() {
	uint32_t dim = static_cast<uint32_t>(pow(DYNAMIC_UB_OBJECTS, (1.0f / 3.0f)));
	glm::vec3 offset(cloudsVisibDist, cloudsVisibDist/10, cloudsVisibDist);
	cloudsPos+=0.01f;
	
	for (uint32_t x = 0; x < dim; x++) {
		for (uint32_t y = 0; y < dim; y++) {
			for (uint32_t z = 0; z < dim; z++) {
				uint32_t index = x * dim * dim + y * dim + z;

				// Aligned offset
				glm::mat4* modelMat = (glm::mat4*)(((uint64_t)uboDataDynamic.model + (index * dynamicAlignment)));
					
				// Update matrices
				glm::vec3 pos = glm::vec3(-((dim * offset.x) / 2.0f) + offset.x / 2.0f + x * offset.x, -((dim * offset.y) / 2.0f) + offset.y / 2.0f + y * offset.y, -((dim * offset.z) / 2.0f) + offset.z / 2.0f + z * offset.z);
					
				pos += rnd_pos[index];
				
				if(cloudsPos>cloudsVisibDist*2){
					cloudsPos=-cloudsVisibDist*2;
				} 

				*modelMat = glm::translate(glm::mat4(1.0f), pos);
				*modelMat = glm::translate(*modelMat, glm::vec3(cloudsPos, cloudsVisibDist*2, 0.0f));
				//*modelMat = glm::scale(*modelMat,  glm::vec3(55.0f, 55.0f, 55.0f));

				//*modelMat = glm::rotate(*modelMat, time * glm::radians(90.0f), glm::vec3(1.0f, 1.0f, 0.0f));
				//*modelMat = glm::rotate(*modelMat, time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				//*modelMat = glm::rotate(*modelMat, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			}
		}
	}

	memcpy(m_UboCloudsDynamic.getMapped(), uboDataDynamic.model, sizeof(uboDataDynamic));

	// Flush to make changes visible to the host 
	VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.memory = m_UboCloudsDynamic.getMemory();
	memoryRange.size = sizeof(uboDataDynamic);	
	vkFlushMappedMemoryRanges(logicalDevice->get(), 1, &memoryRange);
}

void Scene::UpdateSkyboxUniformBuffer() {
	UBOSB.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOSB.proj[1][1] *= -1;
	UBOSB.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOSB.view[3][0] *= 0;
	UBOSB.view[3][1] *= 0;
	UBOSB.view[3][2] *= 0;
	UBOSB.time = (float)mainClock->totalTime;
	memcpy(m_UboSkybox.getMapped(), &UBOSB, sizeof(UBOSB));
	memcpy(m_UboSkyboxRefraction.getMapped(), &UBOSB, sizeof(UBOSB));

	UBOSB.view[1][0] *= -1;
	UBOSB.view[1][1] *= -1;
	UBOSB.view[1][2] *= -1;
	memcpy(m_UboSkyboxReflection.getMapped(), &UBOSB, sizeof(UBOSB));
}

void Scene::UpdateOceanUniformBuffer() {
	UBOSE.model = glm::mat4(1.0f);
	UBOSE.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOSE.proj[1][1] *= -1;
	UBOSE.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOSE.cameraPos = currentCamera->position;
	UBOSE.time = (float)mainClock->totalTime;
	memcpy(m_UboOcean.getMapped(), &UBOSE, sizeof(UBOSE));
}

// ------ Text overlay - performance statistics ----- //

void Scene::UpdateGUI() {
	guiMainHub->UpdateGui();
}

// ------------------ Descriptors ------------------- //

void Scene::CreateDescriptorSetLayout() {
	//Secene models 
	VkDescriptorSetLayoutBinding SceneModelUBOLayoutBinding = {};
	SceneModelUBOLayoutBinding.binding = 0;
	SceneModelUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	SceneModelUBOLayoutBinding.descriptorCount = 1;
	SceneModelUBOLayoutBinding.pImmutableSamplers = nullptr;
	SceneModelUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding UBOParamLayoutBinding = {};
	UBOParamLayoutBinding.binding = 1;
	UBOParamLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	UBOParamLayoutBinding.descriptorCount = 1;
	UBOParamLayoutBinding.pImmutableSamplers = nullptr;
	UBOParamLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 2;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding albedoLayoutBinding = {};
	albedoLayoutBinding.binding = 3;
	albedoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedoLayoutBinding.descriptorCount = 1;
	albedoLayoutBinding.pImmutableSamplers = nullptr;
	albedoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding metallicLayoutBinding = {};
	metallicLayoutBinding.binding = 4;
	metallicLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	metallicLayoutBinding.descriptorCount = 1;
	metallicLayoutBinding.pImmutableSamplers = nullptr;
	metallicLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding roughnessLayoutBinding = {};
	roughnessLayoutBinding.binding = 5;
	roughnessLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	roughnessLayoutBinding.descriptorCount = 1;
	roughnessLayoutBinding.pImmutableSamplers = nullptr;
	roughnessLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding normalLayoutBinding = {};
	normalLayoutBinding.binding = 6;
	normalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalLayoutBinding.descriptorCount = 1;
	normalLayoutBinding.pImmutableSamplers = nullptr;
	normalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding aoLayoutBinding = {};
	aoLayoutBinding.binding = 7;
	aoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	aoLayoutBinding.descriptorCount = 1;
	aoLayoutBinding.pImmutableSamplers = nullptr;
	aoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 8> scene_objects_bindings = { SceneModelUBOLayoutBinding, UBOParamLayoutBinding, samplerLayoutBinding,
		albedoLayoutBinding, metallicLayoutBinding, roughnessLayoutBinding, normalLayoutBinding, aoLayoutBinding };

	VkDescriptorSetLayoutCreateInfo SceneObjectsLayoutInfo = {};
	SceneObjectsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	SceneObjectsLayoutInfo.bindingCount = static_cast<uint32_t>(scene_objects_bindings.size());
	SceneObjectsLayoutInfo.pBindings = scene_objects_bindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->get(), &SceneObjectsLayoutInfo, nullptr, &descriptor_set_layout));

	// Skybox
	VkDescriptorSetLayoutBinding SkyboxUBOLayoutBinding = {};
	SkyboxUBOLayoutBinding.binding = 0;
	SkyboxUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	SkyboxUBOLayoutBinding.descriptorCount = 1;
	SkyboxUBOLayoutBinding.pImmutableSamplers = nullptr;
	SkyboxUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding SkyboxParamLayoutBinding = {};
	SkyboxParamLayoutBinding.binding = 1;
	SkyboxParamLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	SkyboxParamLayoutBinding.descriptorCount = 1;
	SkyboxParamLayoutBinding.pImmutableSamplers = nullptr;
	SkyboxParamLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding SkyboxImageLayoutBinding = {};
	SkyboxImageLayoutBinding.binding = 2;
	SkyboxImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SkyboxImageLayoutBinding.descriptorCount = 1;
	SkyboxImageLayoutBinding.pImmutableSamplers = nullptr;
	SkyboxImageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 3> skybox_bindings = { SkyboxUBOLayoutBinding, SkyboxParamLayoutBinding, SkyboxImageLayoutBinding };

	VkDescriptorSetLayoutCreateInfo SkyboxLayoutInfo = {};
	SkyboxLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	SkyboxLayoutInfo.bindingCount = static_cast<uint32_t>(skybox_bindings.size());
	SkyboxLayoutInfo.pBindings = skybox_bindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->get(), &SkyboxLayoutInfo, nullptr, &skybox_descriptor_set_layout));
	
	// Dynamic buffer
	VkDescriptorSetLayoutBinding CloudsUBOLayoutBinding = {};
	CloudsUBOLayoutBinding.binding = 0;
	CloudsUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	CloudsUBOLayoutBinding.descriptorCount = 1;
	CloudsUBOLayoutBinding.pImmutableSamplers = nullptr;
	CloudsUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding DynamicUBOCloudsBinding = {};
	DynamicUBOCloudsBinding.binding = 1;
	DynamicUBOCloudsBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	DynamicUBOCloudsBinding.descriptorCount = 1;
	DynamicUBOCloudsBinding.pImmutableSamplers = nullptr;
	DynamicUBOCloudsBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> clouds_bindings = { CloudsUBOLayoutBinding, DynamicUBOCloudsBinding };

	VkDescriptorSetLayoutCreateInfo CloudsLayoutInfo = {};
	CloudsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	CloudsLayoutInfo.bindingCount = static_cast<uint32_t>(clouds_bindings.size());
	CloudsLayoutInfo.pBindings = clouds_bindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->get(), &CloudsLayoutInfo, nullptr, &cloudDescriptorSetLayout));

	// Ocean
	VkDescriptorSetLayoutBinding oceanUBOLayoutBinding = {};
	oceanUBOLayoutBinding.binding = 0;
	oceanUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	oceanUBOLayoutBinding.descriptorCount = 1;
	oceanUBOLayoutBinding.pImmutableSamplers = nullptr;
	oceanUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding skyboxReflectionLayoutBinding = {};
	skyboxReflectionLayoutBinding.binding = 1;
	skyboxReflectionLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	skyboxReflectionLayoutBinding.descriptorCount = 1;
	skyboxReflectionLayoutBinding.pImmutableSamplers = nullptr;
	skyboxReflectionLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding reflectionLayoutBinding = {};
	reflectionLayoutBinding.binding = 2;
	reflectionLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	reflectionLayoutBinding.descriptorCount = 1;
	reflectionLayoutBinding.pImmutableSamplers = nullptr;
	reflectionLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding refractionLayoutBinding = {};
	refractionLayoutBinding.binding = 3;
	refractionLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	refractionLayoutBinding.descriptorCount = 1;
	refractionLayoutBinding.pImmutableSamplers = nullptr;
	refractionLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 4> oceanBindings = { oceanUBOLayoutBinding, skyboxReflectionLayoutBinding, reflectionLayoutBinding, refractionLayoutBinding };

	VkDescriptorSetLayoutCreateInfo OceanLayoutInfo = {};
	OceanLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	OceanLayoutInfo.bindingCount = static_cast<uint32_t>(oceanBindings.size());
	OceanLayoutInfo.pBindings = oceanBindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->get(), &OceanLayoutInfo, nullptr, &oceanDescriptorSetLayout));

	// Line
	VkDescriptorSetLayoutBinding lineUBOLayoutBinding = {};
	lineUBOLayoutBinding.binding = 0;
	lineUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lineUBOLayoutBinding.descriptorCount = 1;
	lineUBOLayoutBinding.pImmutableSamplers = nullptr;
	lineUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> lineBindings = { lineUBOLayoutBinding };

	VkDescriptorSetLayoutCreateInfo LineLayoutInfo = {};
	LineLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	LineLayoutInfo.bindingCount = static_cast<uint32_t>(lineBindings.size());
	LineLayoutInfo.pBindings = lineBindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->get(), &LineLayoutInfo, nullptr, &lineDescriptorSetLayout));

	// Selection indicator
	VkDescriptorSetLayoutBinding SelectionIndicatorUboLayoutBinding = {};
	SelectionIndicatorUboLayoutBinding.binding = 0;
	SelectionIndicatorUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	SelectionIndicatorUboLayoutBinding.descriptorCount = 1;
	SelectionIndicatorUboLayoutBinding.pImmutableSamplers = nullptr;
	SelectionIndicatorUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding SelectionIndicatorImageLayoutBinding = {};
	SelectionIndicatorImageLayoutBinding.binding = 1;
	SelectionIndicatorImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SelectionIndicatorImageLayoutBinding.descriptorCount = 1;
	SelectionIndicatorImageLayoutBinding.pImmutableSamplers = nullptr;
	SelectionIndicatorImageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> selectionIndicatorBindings = { SelectionIndicatorUboLayoutBinding, SelectionIndicatorImageLayoutBinding };

	VkDescriptorSetLayoutCreateInfo SelectionIndicatorLayoutInfo = {};
	SelectionIndicatorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	SelectionIndicatorLayoutInfo.bindingCount = static_cast<uint32_t>(selectionIndicatorBindings.size());
	SelectionIndicatorLayoutInfo.pBindings = selectionIndicatorBindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->get(), &SelectionIndicatorLayoutInfo, nullptr, &selectionIndicatorDescriptorSetLayout));

	// Aabb
	VkDescriptorSetLayoutBinding aabbUBOLayoutBinding = {};
	aabbUBOLayoutBinding.binding = 0;
	aabbUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	aabbUBOLayoutBinding.descriptorCount = 1;
	aabbUBOLayoutBinding.pImmutableSamplers = nullptr;
	aabbUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> aabbBindings = { aabbUBOLayoutBinding };

	VkDescriptorSetLayoutCreateInfo AabbLayoutInfo = {};
	AabbLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	AabbLayoutInfo.bindingCount = static_cast<uint32_t>(aabbBindings.size());
	AabbLayoutInfo.pBindings = aabbBindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->get(), &AabbLayoutInfo, nullptr, &aabbDescriptorSetLayout));
}

void Scene::CreateDescriptorPool() {
	// Don't forget to rise this numbers when you add bindings
	std::array<VkDescriptorPoolSize, 3> PoolSizes = {};
	PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	PoolSizes[0].descriptorCount = static_cast<uint32_t>(1);
	PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	PoolSizes[1].descriptorCount = static_cast<uint32_t>(materialLibrary->materials.size() * 6 * 3 + 11);
	PoolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	PoolSizes[2].descriptorCount = static_cast<uint32_t>(materialLibrary->materials.size() * 6 + 11);

	VkDescriptorPoolCreateInfo PoolInfo = {};
	PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.maxSets = static_cast<uint32_t>(materialLibrary->materials.size()*3 + 8); // maximum number of descriptor sets that will be allocated

	ErrorCheck(vkCreateDescriptorPool(logicalDevice->get(), &PoolInfo, nullptr, &descriptorPool));
}

void Scene::CreateDescriptorSet() {
	// 3D object descriptor set
	for (auto& m : materialLibrary->materials) {
		VkDescriptorImageInfo IrradianceMapImageInfo = {};
		IrradianceMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		IrradianceMapImageInfo.imageView = sky->skybox.view;
		IrradianceMapImageInfo.sampler = sky->skybox.sampler;

		VkDescriptorImageInfo AlbedoImageInfo = {};
		AlbedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		AlbedoImageInfo.imageView = m.second.albedo.view;
		AlbedoImageInfo.sampler = m.second.albedo.sampler;

		VkDescriptorImageInfo MettalicImageInfo = {};
		MettalicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		MettalicImageInfo.imageView = m.second.metallic.view;
		MettalicImageInfo.sampler = m.second.metallic.sampler;

		VkDescriptorImageInfo RoughnessImageInfo = {};
		RoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		RoughnessImageInfo.imageView = m.second.roughness.view;
		RoughnessImageInfo.sampler = m.second.roughness.sampler;

		VkDescriptorImageInfo NormalImageInfo = {};
		NormalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		NormalImageInfo.imageView = m.second.normal.view;
		NormalImageInfo.sampler = m.second.normal.sampler;

		VkDescriptorImageInfo AmbientOcclusionImageInfo = {};
		AmbientOcclusionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		AmbientOcclusionImageInfo.imageView = m.second.ambientOcclucion.view;
		AmbientOcclusionImageInfo.sampler = m.second.ambientOcclucion.sampler;

		VkDescriptorSetAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfo.descriptorPool = descriptorPool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &descriptor_set_layout;

		ErrorCheck(vkAllocateDescriptorSets(logicalDevice->get(), &AllocInfo, &m.second.descriptorSet));
		ErrorCheck(vkAllocateDescriptorSets(logicalDevice->get(), &AllocInfo, &m.second.reflectDescriptorSet));
		ErrorCheck(vkAllocateDescriptorSets(logicalDevice->get(), &AllocInfo, &m.second.refractDescriptorSet));

		VkDescriptorBufferInfo BufferInfo = {};
		BufferInfo.buffer = m_UboStillObjects.getBuffer();
		BufferInfo.offset = 0;
		BufferInfo.range = sizeof(UboStaticGeometry);
		
		VkDescriptorBufferInfo ObjectBufferParametersInfo = {};
		ObjectBufferParametersInfo.buffer = m_UboParameters.getBuffer();
		ObjectBufferParametersInfo.offset = 0;
		ObjectBufferParametersInfo.range = sizeof(UboParam);

		std::array<VkWriteDescriptorSet, 8> objectDescriptorWrites = {};

		objectDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[0].dstSet = m.second.descriptorSet;
		objectDescriptorWrites[0].dstBinding = 0;
		objectDescriptorWrites[0].dstArrayElement = 0;
		objectDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		objectDescriptorWrites[0].descriptorCount = 1;
		objectDescriptorWrites[0].pBufferInfo = &BufferInfo;

		objectDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[1].dstSet = m.second.descriptorSet;
		objectDescriptorWrites[1].dstBinding = 1;
		objectDescriptorWrites[1].dstArrayElement = 0;
		objectDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		objectDescriptorWrites[1].descriptorCount = 1;
		objectDescriptorWrites[1].pBufferInfo = &ObjectBufferParametersInfo;

		objectDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[2].dstSet = m.second.descriptorSet;
		objectDescriptorWrites[2].dstBinding = 2;
		objectDescriptorWrites[2].dstArrayElement = 0;
		objectDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[2].descriptorCount = 1;
		objectDescriptorWrites[2].pImageInfo = &IrradianceMapImageInfo;

		objectDescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[3].dstSet = m.second.descriptorSet;
		objectDescriptorWrites[3].dstBinding = 3;
		objectDescriptorWrites[3].dstArrayElement = 0;
		objectDescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[3].descriptorCount = 1;
		objectDescriptorWrites[3].pImageInfo = &AlbedoImageInfo;

		objectDescriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[4].dstSet = m.second.descriptorSet;
		objectDescriptorWrites[4].dstBinding = 4;
		objectDescriptorWrites[4].dstArrayElement = 0;
		objectDescriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[4].descriptorCount = 1;
		objectDescriptorWrites[4].pImageInfo = &MettalicImageInfo;

		objectDescriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[5].dstSet = m.second.descriptorSet;
		objectDescriptorWrites[5].dstBinding = 5;
		objectDescriptorWrites[5].dstArrayElement = 0;
		objectDescriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[5].descriptorCount = 1;
		objectDescriptorWrites[5].pImageInfo = &RoughnessImageInfo;

		objectDescriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[6].dstSet = m.second.descriptorSet;
		objectDescriptorWrites[6].dstBinding = 6;
		objectDescriptorWrites[6].dstArrayElement = 0;
		objectDescriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[6].descriptorCount = 1;
		objectDescriptorWrites[6].pImageInfo = &NormalImageInfo;

		objectDescriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[7].dstSet = m.second.descriptorSet;
		objectDescriptorWrites[7].dstBinding = 7;
		objectDescriptorWrites[7].dstArrayElement = 0;
		objectDescriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[7].descriptorCount = 1;
		objectDescriptorWrites[7].pImageInfo = &AmbientOcclusionImageInfo;

		vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(objectDescriptorWrites.size()), objectDescriptorWrites.data(), 0, nullptr);

		// Copy above descriptor set values to reflection ad refracion

		VkDescriptorBufferInfo reflectBufferInfo = {};
		reflectBufferInfo.buffer = m_UboReflection.getBuffer();
		reflectBufferInfo.offset = 0;
		reflectBufferInfo.range = sizeof(UboOffscreen);

		VkDescriptorBufferInfo reflectParametersInfo = {};
		reflectParametersInfo.buffer = m_UboReflectionParameters.getBuffer();
		reflectParametersInfo.offset = 0;
		reflectParametersInfo.range = sizeof(UboParam);

		std::array<VkWriteDescriptorSet, 8> reflectDescriptorWrites = objectDescriptorWrites;

		reflectDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		reflectDescriptorWrites[0].dstSet = m.second.reflectDescriptorSet;
		reflectDescriptorWrites[0].pBufferInfo = &reflectBufferInfo;
		reflectDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		reflectDescriptorWrites[1].dstSet = m.second.reflectDescriptorSet;
		reflectDescriptorWrites[1].pBufferInfo = &reflectParametersInfo;
		reflectDescriptorWrites[2].dstSet = m.second.reflectDescriptorSet;
		reflectDescriptorWrites[3].dstSet = m.second.reflectDescriptorSet;
		reflectDescriptorWrites[4].dstSet = m.second.reflectDescriptorSet;
		reflectDescriptorWrites[5].dstSet = m.second.reflectDescriptorSet;
		reflectDescriptorWrites[6].dstSet = m.second.reflectDescriptorSet;
		reflectDescriptorWrites[7].dstSet = m.second.reflectDescriptorSet;

		vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(reflectDescriptorWrites.size()), reflectDescriptorWrites.data(), 0, nullptr);

		VkDescriptorBufferInfo refractBufferInfo = {};
		refractBufferInfo.buffer = m_UboRefraction.getBuffer();
		refractBufferInfo.offset = 0;
		refractBufferInfo.range = sizeof(UboOffscreen);

		VkDescriptorBufferInfo refractParametersInfo = {};
		refractParametersInfo.buffer = m_UboRefractionParameters.getBuffer();
		refractParametersInfo.offset = 0;
		refractParametersInfo.range = sizeof(UboParam);

		std::array<VkWriteDescriptorSet, 8> refractDescriptorWrites = objectDescriptorWrites;

		refractDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		refractDescriptorWrites[0].dstSet = m.second.refractDescriptorSet;
		refractDescriptorWrites[0].pBufferInfo = &refractBufferInfo;
		refractDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		refractDescriptorWrites[1].dstSet = m.second.refractDescriptorSet;
		refractDescriptorWrites[1].pBufferInfo = &refractParametersInfo;
		refractDescriptorWrites[2].dstSet = m.second.refractDescriptorSet;
		refractDescriptorWrites[3].dstSet = m.second.refractDescriptorSet;
		refractDescriptorWrites[4].dstSet = m.second.refractDescriptorSet;
		refractDescriptorWrites[5].dstSet = m.second.refractDescriptorSet;
		refractDescriptorWrites[6].dstSet = m.second.refractDescriptorSet;
		refractDescriptorWrites[7].dstSet = m.second.refractDescriptorSet;

		vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(refractDescriptorWrites.size()), refractDescriptorWrites.data(), 0, nullptr);
	}

	// SkyBox descriptor set
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &skybox_descriptor_set_layout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->get(), &allocInfo, &skybox_descriptor_set));
	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->get(), &allocInfo, &skyboxReflectionDescriptorSet));
	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->get(), &allocInfo, &skyboxRefractionDescriptorSet));

	VkDescriptorBufferInfo SkyboxBufferInfo = {};
	SkyboxBufferInfo.buffer = m_UboSkybox.getBuffer();
	SkyboxBufferInfo.offset = 0;
	SkyboxBufferInfo.range = sizeof(UboSkybox);

	VkDescriptorBufferInfo SkyboxBufferParametersInfo = {};
	SkyboxBufferParametersInfo.buffer = m_UboParameters.getBuffer();
	SkyboxBufferParametersInfo.offset = 0;
	SkyboxBufferParametersInfo.range = sizeof(UboParam);

	VkDescriptorImageInfo ImageInfo = {};
	ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ImageInfo.imageView = sky->skybox.view;
	ImageInfo.sampler = sky->skybox.sampler;

	std::array<VkWriteDescriptorSet, 3> skyboxDescriptorWrites = {};

	skyboxDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	skyboxDescriptorWrites[0].dstSet = skybox_descriptor_set;
	skyboxDescriptorWrites[0].dstBinding = 0;
	skyboxDescriptorWrites[0].dstArrayElement = 0;
	skyboxDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	skyboxDescriptorWrites[0].descriptorCount = 1;
	skyboxDescriptorWrites[0].pBufferInfo = &SkyboxBufferInfo;

	skyboxDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	skyboxDescriptorWrites[1].dstSet = skybox_descriptor_set;
	skyboxDescriptorWrites[1].dstBinding = 1;
	skyboxDescriptorWrites[1].dstArrayElement = 0;
	skyboxDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	skyboxDescriptorWrites[1].descriptorCount = 1;
	skyboxDescriptorWrites[1].pBufferInfo = &SkyboxBufferParametersInfo;

	skyboxDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	skyboxDescriptorWrites[2].dstSet = skybox_descriptor_set;
	skyboxDescriptorWrites[2].dstBinding = 2;
	skyboxDescriptorWrites[2].dstArrayElement = 0;
	skyboxDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	skyboxDescriptorWrites[2].descriptorCount = 1;
	skyboxDescriptorWrites[2].pImageInfo = &ImageInfo;

	vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(skyboxDescriptorWrites.size()), skyboxDescriptorWrites.data(), 0, nullptr);

	VkDescriptorBufferInfo skyboxReflectBufferInfo = {};
	skyboxReflectBufferInfo.buffer = m_UboSkyboxReflection.getBuffer();
	skyboxReflectBufferInfo.offset = 0;
	skyboxReflectBufferInfo.range = sizeof(UboSkybox);

	VkDescriptorBufferInfo skyboxReflectBufferParametersInfo = {};
	skyboxReflectBufferParametersInfo.buffer = m_UboParameters.getBuffer();
	skyboxReflectBufferParametersInfo.offset = 0;
	skyboxReflectBufferParametersInfo.range = sizeof(UboParam);

	std::array<VkWriteDescriptorSet, 3> skyboxReflectionDescriptorWrites = skyboxDescriptorWrites;

	skyboxReflectionDescriptorWrites[0].dstSet = skyboxReflectionDescriptorSet;
	skyboxReflectionDescriptorWrites[0].pBufferInfo = &skyboxReflectBufferInfo;
	skyboxReflectionDescriptorWrites[1].dstSet = skyboxReflectionDescriptorSet;
	skyboxReflectionDescriptorWrites[1].pBufferInfo = &skyboxReflectBufferParametersInfo;
	skyboxReflectionDescriptorWrites[2].dstSet = skyboxReflectionDescriptorSet;
	
	vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(skyboxReflectionDescriptorWrites.size()), skyboxReflectionDescriptorWrites.data(), 0, nullptr);

	VkDescriptorBufferInfo skyboxRefractionBufferInfo = {};
	skyboxRefractionBufferInfo.buffer = m_UboSkyboxRefraction.getBuffer();
	skyboxRefractionBufferInfo.offset = 0;
	skyboxRefractionBufferInfo.range = sizeof(UboSkybox);

	VkDescriptorBufferInfo skyboxRefractionBufferParametersInfo = {};
	skyboxRefractionBufferParametersInfo.buffer = m_UboParameters.getBuffer();
	skyboxRefractionBufferParametersInfo.offset = 0;
	skyboxRefractionBufferParametersInfo.range = sizeof(UboParam);

	std::array<VkWriteDescriptorSet, 3> skyboxRefractionDescriptorWrites = skyboxDescriptorWrites;

	skyboxRefractionDescriptorWrites[0].dstSet = skyboxRefractionDescriptorSet;
	skyboxRefractionDescriptorWrites[0].pBufferInfo = &skyboxRefractionBufferInfo;
	skyboxRefractionDescriptorWrites[1].dstSet = skyboxReflectionDescriptorSet;
	skyboxRefractionDescriptorWrites[1].pBufferInfo = &skyboxRefractionBufferParametersInfo;
	skyboxRefractionDescriptorWrites[2].dstSet = skyboxRefractionDescriptorSet;
	
	vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(skyboxRefractionDescriptorWrites.size()), skyboxRefractionDescriptorWrites.data(), 0, nullptr);
	
	// Clouds descriptor set
	VkDescriptorSetAllocateInfo CloudsAllocInfo = {};
	CloudsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	CloudsAllocInfo.descriptorPool = descriptorPool;
	CloudsAllocInfo.descriptorSetCount = 1;
	CloudsAllocInfo.pSetLayouts = &cloudDescriptorSetLayout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->get(), &CloudsAllocInfo, &cloudDescriptorSet));

	VkDescriptorBufferInfo CloudsBufferInfo = {};
	CloudsBufferInfo.buffer = m_UboClouds.getBuffer();
	CloudsBufferInfo.offset = 0;
	CloudsBufferInfo.range = sizeof(UboClouds);

	VkDescriptorBufferInfo CloudsDynamicBufferInfo = {};
	CloudsDynamicBufferInfo.buffer = m_UboCloudsDynamic.getBuffer();
	CloudsDynamicBufferInfo.offset = 0;
	CloudsDynamicBufferInfo.range = sizeof(UboDataDynamic);

	std::array<VkWriteDescriptorSet, 2> cloudsDescriptorWrites = {};

	cloudsDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	cloudsDescriptorWrites[0].dstSet = cloudDescriptorSet;
	cloudsDescriptorWrites[0].dstBinding = 0;
	cloudsDescriptorWrites[0].dstArrayElement = 0;
	cloudsDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cloudsDescriptorWrites[0].descriptorCount = 1;
	cloudsDescriptorWrites[0].pBufferInfo = &CloudsBufferInfo;

	cloudsDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	cloudsDescriptorWrites[1].dstSet = cloudDescriptorSet;
	cloudsDescriptorWrites[1].dstBinding = 1;
	cloudsDescriptorWrites[1].dstArrayElement = 0;
	cloudsDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	cloudsDescriptorWrites[1].descriptorCount = 1;
	cloudsDescriptorWrites[1].pBufferInfo = &CloudsDynamicBufferInfo;

	vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(cloudsDescriptorWrites.size()), cloudsDescriptorWrites.data(), 0, nullptr);

	// Ocean descriptor set
	VkDescriptorSetAllocateInfo oceanAllocInfo = {};
	oceanAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	oceanAllocInfo.descriptorPool = descriptorPool;
	oceanAllocInfo.descriptorSetCount = 1;
	oceanAllocInfo.pSetLayouts = &oceanDescriptorSetLayout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->get(), &oceanAllocInfo, &oceanDescriptorSet));

	VkDescriptorBufferInfo oceanBufferInfo = {};
	oceanBufferInfo.buffer = m_UboOcean.getBuffer();
	oceanBufferInfo.offset = 0;
	oceanBufferInfo.range = sizeof(UboSea);

	VkDescriptorImageInfo IrradianceMapImageInfo = {};
	IrradianceMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	IrradianceMapImageInfo.imageView = sky->skybox.view;
	IrradianceMapImageInfo.sampler = sky->skybox.sampler;

	VkDescriptorImageInfo ReflectionImageInfo = {};
	ReflectionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ReflectionImageInfo.imageView = reflectionImage->view;
	ReflectionImageInfo.sampler = reflectionImage->sampler;

	VkDescriptorImageInfo RefractionImageInfo = {};
	RefractionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	RefractionImageInfo.imageView = refractionImage->view;
	RefractionImageInfo.sampler = refractionImage->sampler;

	std::array<VkWriteDescriptorSet, 4> oceanDescriptorWrites = {};

	oceanDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	oceanDescriptorWrites[0].dstSet = oceanDescriptorSet;
	oceanDescriptorWrites[0].dstBinding = 0;
	oceanDescriptorWrites[0].dstArrayElement = 0;
	oceanDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	oceanDescriptorWrites[0].descriptorCount = 1;
	oceanDescriptorWrites[0].pBufferInfo = &oceanBufferInfo;

	oceanDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	oceanDescriptorWrites[1].dstSet = oceanDescriptorSet;
	oceanDescriptorWrites[1].dstBinding = 1;
	oceanDescriptorWrites[1].dstArrayElement = 0;
	oceanDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	oceanDescriptorWrites[1].descriptorCount = 1;
	oceanDescriptorWrites[1].pImageInfo = &IrradianceMapImageInfo;

	oceanDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	oceanDescriptorWrites[2].dstSet = oceanDescriptorSet;
	oceanDescriptorWrites[2].dstBinding = 2;
	oceanDescriptorWrites[2].dstArrayElement = 0;
	oceanDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	oceanDescriptorWrites[2].descriptorCount = 1;
	oceanDescriptorWrites[2].pImageInfo = &ReflectionImageInfo;

	oceanDescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	oceanDescriptorWrites[3].dstSet = oceanDescriptorSet;
	oceanDescriptorWrites[3].dstBinding = 3;
	oceanDescriptorWrites[3].dstArrayElement = 0;
	oceanDescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	oceanDescriptorWrites[3].descriptorCount = 1;
	oceanDescriptorWrites[3].pImageInfo = &RefractionImageInfo;

	vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(oceanDescriptorWrites.size()), oceanDescriptorWrites.data(), 0, nullptr);

	// Line descriptor set
	VkDescriptorSetAllocateInfo LineAllocInfo = {};
	LineAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	LineAllocInfo.descriptorPool = descriptorPool;
	LineAllocInfo.descriptorSetCount = 1;
	LineAllocInfo.pSetLayouts = &lineDescriptorSetLayout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->get(), &LineAllocInfo, &lineDescriptorSet));

	VkDescriptorBufferInfo LineBufferInfo = {};
	LineBufferInfo.buffer = m_UboLine.getBuffer();
	LineBufferInfo.offset = 0;
	LineBufferInfo.range = sizeof(UboStaticGeometry);

	std::array<VkWriteDescriptorSet, 1> lineDescriptorWrites = {};

	lineDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	lineDescriptorWrites[0].dstSet = lineDescriptorSet;
	lineDescriptorWrites[0].dstBinding = 0;
	lineDescriptorWrites[0].dstArrayElement = 0;
	lineDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lineDescriptorWrites[0].descriptorCount = 1;
	lineDescriptorWrites[0].pBufferInfo = &LineBufferInfo;

	vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(lineDescriptorWrites.size()), lineDescriptorWrites.data(), 0, nullptr);

	// Selection indicator descriptor set
	VkDescriptorSetAllocateInfo SelectionIndicatorAllocInfo = {};
	SelectionIndicatorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	SelectionIndicatorAllocInfo.descriptorPool = descriptorPool;
	SelectionIndicatorAllocInfo.descriptorSetCount = 1;
	SelectionIndicatorAllocInfo.pSetLayouts = &selectionIndicatorDescriptorSetLayout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->get(), &SelectionIndicatorAllocInfo, &selectionIndicatorDescriptorSet));

	VkDescriptorBufferInfo SelectionIndicatorBufferInfo = {};
	SelectionIndicatorBufferInfo.buffer = m_UboSlectionIndicator.getBuffer();
	SelectionIndicatorBufferInfo.offset = 0;
	SelectionIndicatorBufferInfo.range = sizeof(UboSelectionIndicator);

	VkDescriptorImageInfo SelectionIndicatorImageInfo = {};
	SelectionIndicatorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	SelectionIndicatorImageInfo.imageView = sky->skybox.view;
	SelectionIndicatorImageInfo.sampler = sky->skybox.sampler;
	
	std::array<VkWriteDescriptorSet, 2> selectionIndicatorDescriptorWrites = {};

	selectionIndicatorDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	selectionIndicatorDescriptorWrites[0].dstSet = selectionIndicatorDescriptorSet;
	selectionIndicatorDescriptorWrites[0].dstBinding = 0;
	selectionIndicatorDescriptorWrites[0].dstArrayElement = 0;
	selectionIndicatorDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	selectionIndicatorDescriptorWrites[0].descriptorCount = 1;
	selectionIndicatorDescriptorWrites[0].pBufferInfo = &SelectionIndicatorBufferInfo;

	selectionIndicatorDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	selectionIndicatorDescriptorWrites[1].dstSet = selectionIndicatorDescriptorSet;
	selectionIndicatorDescriptorWrites[1].dstBinding = 1;
	selectionIndicatorDescriptorWrites[1].dstArrayElement = 0;
	selectionIndicatorDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	selectionIndicatorDescriptorWrites[1].descriptorCount = 1;
	selectionIndicatorDescriptorWrites[1].pImageInfo = &SelectionIndicatorImageInfo;

	vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(selectionIndicatorDescriptorWrites.size()), selectionIndicatorDescriptorWrites.data(), 0, nullptr);

	// Aabb descriptor set
	VkDescriptorSetAllocateInfo AabbAllocInfo = {};
	AabbAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AabbAllocInfo.descriptorPool = descriptorPool;
	AabbAllocInfo.descriptorSetCount = 1;
	AabbAllocInfo.pSetLayouts = &aabbDescriptorSetLayout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->get(), &AabbAllocInfo, &aabbDescriptorSet));

	VkDescriptorBufferInfo AabbBufferInfo = {};
	AabbBufferInfo.buffer = m_UboLine.getBuffer();
	AabbBufferInfo.offset = 0;
	AabbBufferInfo.range = sizeof(UboStaticGeometry);

	std::array<VkWriteDescriptorSet, 1> aabbDescriptorWrites = {};

	aabbDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	aabbDescriptorWrites[0].dstSet = aabbDescriptorSet;
	aabbDescriptorWrites[0].dstBinding = 0;
	aabbDescriptorWrites[0].dstArrayElement = 0;
	aabbDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	aabbDescriptorWrites[0].descriptorCount = 1;
	aabbDescriptorWrites[0].pBufferInfo = &AabbBufferInfo;

	vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(aabbDescriptorWrites.size()), aabbDescriptorWrites.data(), 0, nullptr);
}

void Scene::UpdateDescriptorSet() {
	VkDescriptorBufferInfo oceanBufferInfo = {};
	oceanBufferInfo.buffer = m_UboOcean.getBuffer();
	oceanBufferInfo.offset = 0;
	oceanBufferInfo.range = sizeof(UboSea);

	VkDescriptorImageInfo IrradianceMapImageInfo = {};
	IrradianceMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	IrradianceMapImageInfo.imageView = sky->skybox.view;
	IrradianceMapImageInfo.sampler = sky->skybox.sampler;

	VkDescriptorImageInfo ReflectionImageInfo = {};
	ReflectionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ReflectionImageInfo.imageView = reflectionImage->view;
	ReflectionImageInfo.sampler = reflectionImage->sampler;

	VkDescriptorImageInfo RefractionImageInfo = {};
	RefractionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	RefractionImageInfo.imageView = refractionImage->view;
	RefractionImageInfo.sampler = refractionImage->sampler;

	std::array<VkWriteDescriptorSet, 4> oceanDescriptorWrites = {};

	oceanDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	oceanDescriptorWrites[0].dstSet = oceanDescriptorSet;
	oceanDescriptorWrites[0].dstBinding = 0;
	oceanDescriptorWrites[0].dstArrayElement = 0;
	oceanDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	oceanDescriptorWrites[0].descriptorCount = 1;
	oceanDescriptorWrites[0].pBufferInfo = &oceanBufferInfo;

	oceanDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	oceanDescriptorWrites[1].dstSet = oceanDescriptorSet;
	oceanDescriptorWrites[1].dstBinding = 1;
	oceanDescriptorWrites[1].dstArrayElement = 0;
	oceanDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	oceanDescriptorWrites[1].descriptorCount = 1;
	oceanDescriptorWrites[1].pImageInfo = &IrradianceMapImageInfo;

	oceanDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	oceanDescriptorWrites[2].dstSet = oceanDescriptorSet;
	oceanDescriptorWrites[2].dstBinding = 2;
	oceanDescriptorWrites[2].dstArrayElement = 0;
	oceanDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	oceanDescriptorWrites[2].descriptorCount = 1;
	oceanDescriptorWrites[2].pImageInfo = &ReflectionImageInfo;

	oceanDescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	oceanDescriptorWrites[3].dstSet = oceanDescriptorSet;
	oceanDescriptorWrites[3].dstBinding = 3;
	oceanDescriptorWrites[3].dstArrayElement = 0;
	oceanDescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	oceanDescriptorWrites[3].descriptorCount = 1;
	oceanDescriptorWrites[3].pImageInfo = &RefractionImageInfo;

	vkUpdateDescriptorSets(logicalDevice->get(), static_cast<uint32_t>(oceanDescriptorWrites.size()), oceanDescriptorWrites.data(), 0, nullptr);

}

void Scene::Test() {
	std::cout << "TEST" << std::endl;
	CreateLandscape("Runtime creation test ", "Do you see box?", glm::vec3(500.0f, 0.0f, 500.0f), meshLibrary->meshes["box"], materialLibrary->materials["plastic"]);
}

// ------------- Populate scene --------------------- //

void Scene::UpdateSelectRayDrawData() {
	rayVertices = {
		{ {mousePicker->GetRayOrigin().x, mousePicker->GetRayOrigin().y-1.0f, mousePicker->GetRayOrigin().z}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{ mousePicker->hitPoint, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}
	};

	enginetool::VertexLayout* vtxDst = (enginetool::VertexLayout*)m_VertexBuffersSelectRay.getMapped();
	memcpy(vtxDst, rayVertices.data(), 2 * sizeof(enginetool::VertexLayout));
	m_VertexBuffersSelectRay.flush(VK_WHOLE_SIZE);
	
	uint32_t* idxDst = (uint32_t*)m_IndexBuffersSelectRay.getMapped();
	memcpy(idxDst, rayIndices.data(), 2 * sizeof(uint32_t));
	m_IndexBuffersSelectRay.flush(VK_WHOLE_SIZE);
}

void Scene::CreateSelectRay() {
	rayVertices = {
		{ mousePicker->hitPoint, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{ {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}
	};

	rayIndices = {0,1};

	CreateMappedVertexBuffer(rayVertices, m_VertexBuffersSelectRay);
	CreateMappedIndexBuffer(rayIndices, m_IndexBuffersSelectRay);
}

void Scene::LoadAssets() {
	InitMaterials();
	CreateSelectRay();
	PrepeareMainCharacter(meshLibrary->meshes["human"]);
	
	// Scene objects/actors
	CreateCloud("Test cloud", "Look, I am flying", glm::vec3(0.0f, 0.0f, 0.0f), meshLibrary->meshes["sphere"]);
	CreateSkybox("Test skybox", "Here must be green car, hello! Lorem Ipsum ;)", glm::vec3(0.0f, 0.0f, 0.0f), horizon);
	CreateSea("Test sea", "I am part of terrain, hello!", glm::vec3(0.0f, 0.0f, 0.0f));
	CreateLandscape("Test object amelinium teapot", "You can't paint this!", glm::vec3(-7.0f, 0.0f, 20.0f), meshLibrary->meshes["teapot"], materialLibrary->materials["chrome"]);
	CreateCamera("Test Camera", "Temporary object created for testing purpose", glm::vec3(30.0f, 40.0f, 3.0f), meshLibrary->meshes["box"], materialLibrary->materials["default"]);
	CreateCharacter("Test Character", "Temporary object created for testing purpose", glm::vec3(20.0f, 20.0f,/*1968.5f*/ 10.0f), meshLibrary->meshes["human"], materialLibrary->materials["character"]);
	CreateSphereLight("Test Light", "Lorem ipsum light", glm::vec3(0.0f, 6.0f, 17.0f), meshLibrary->meshes["sphere"]);
	
	CreateLandscape("Test object plane", "I am simple plane, boring", glm::vec3(10.0f, -16.0f, -20.0f), meshLibrary->meshes["plane"], materialLibrary->materials["rust"]);
	CreateLandscape("Test object small green box ", "I am simple 10cm box, watch me", glm::vec3(15.0f, 7.0f, 2.0f), meshLibrary->meshes["box"], materialLibrary->materials["plastic"]);
	CreateLandscape("Test object plane2", "I am simple plane, boring", glm::vec3(10.0f, -14.0f, 0.0f), meshLibrary->meshes["plane"], materialLibrary->materials["default"]);
	CreateLandscape("Test object plane3", "I am simple plane, boring", glm::vec3(10.0f, -12.0f, 40.0f), meshLibrary->meshes["plane"], materialLibrary->materials["default"]);
	CreateLandscape("Test object plane4", "I am simple plane, boring", glm::vec3(10.0f, -10.0f, 80.0f), meshLibrary->meshes["plane"], materialLibrary->materials["default"]);
	CreateLandscape("Test object plane5", "I am simple plane, boring", glm::vec3(10.0f, -5.0f, 120.0f), meshLibrary->meshes["plane"], materialLibrary->materials["default"]);
	CreateLandscape("Test object plane6", "I am simple plane, boring", glm::vec3(10.0f, -1.0f, 160.0f), meshLibrary->meshes["plane"], materialLibrary->materials["default"]);
	CreateLandscape("Test object plane7", "I am simple plane, boring", glm::vec3(10.0f, -4.0f, 200.0f), meshLibrary->meshes["plane"], materialLibrary->materials["default"]);

	CreateLandscape("Visibility test post 1", "Do you see me?", glm::vec3(0.0f, 0.0f, 1000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);
	CreateLandscape("Visibility test post 2", "Do you see me?", glm::vec3(0.0f, 0.0f, 2000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);
	CreateLandscape("Visibility test post 3", "Do you see me?", glm::vec3(0.0f, 0.0f, 3000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);
	CreateLandscape("Visibility test post 4", "Do you see me?", glm::vec3(0.0f, 0.0f, 4000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);
	CreateLandscape("Visibility test post 5", "Do you see me?", glm::vec3(0.0f, 0.0f, 5000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);
	CreateLandscape("Visibility test post 6", "Do you see me?", glm::vec3(0.0f, 0.0f, 6000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);
	CreateLandscape("Visibility test post 7", "Do you see me?", glm::vec3(0.0f, 0.0f, 7000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);
	CreateLandscape("Visibility test post 8", "Do you see me?", glm::vec3(0.0f, 0.0f, 8000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);
	CreateLandscape("Visibility test post 9", "Do you see me?", glm::vec3(0.0f, 0.0f, 9000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);
	CreateLandscape("Visibility test post 10", "Do you see me?", glm::vec3(0.0f, 0.0f, 10000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);
	CreateLandscape("Visibility test post 11", "Do you see me?", glm::vec3(0.0f, 0.0f, 11000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);
	CreateLandscape("Visibility test post 12", "Do you see me?", glm::vec3(0.0f, 0.0f, 12000.0f), meshLibrary->meshes["human"], materialLibrary->materials["default"]);

	CreateLandscape("coin", "lorem ipsum", glm::vec3(0.0f, 50.0f, 100.0f), meshLibrary->meshes["coin"], materialLibrary->materials["gold"]);
			
	currentCamera = std::dynamic_pointer_cast<Camera>(sceneCameras[0]);
	mousePicker->UpdateMousePicker(UBOSG.view, UBOSG.proj, currentCamera);
}

void Scene::PrepeareMainCharacter(enginetool::ScenePart &mesh) {
	mainCharacter = std::make_unique<MainCharacter>("Temp", "Brave hero", glm::vec3(0.0f, 0.0f, 0.0f), ActorType::MainCharacter, actors);
	dynamic_cast<MainCharacter*>(mainCharacter.get())->Init(1000, 1000, 100);
	mainCharacter->assignedMesh = &mesh;
	mainCharacter->assignedMaterial = &materialLibrary->materials["default"];
}

void Scene::CreateCamera(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart &mesh, enginetool::SceneMaterial &material) {
	std::shared_ptr<Actor> camera = std::make_shared<Camera>(name, description, position, ActorType::Camera, actors);
	std::dynamic_pointer_cast<Camera>(camera)->Init(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 60.0f, 0.001f, 200000.0f, 3.14f, 0.0f);
	camera->assignedMesh = &mesh;
	camera->assignedMaterial = &material; 
	sceneCameras.emplace_back(std::move(camera));
}

void Scene::CreateCharacter(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart &mesh, enginetool::SceneMaterial &material) {
	std::shared_ptr<Actor> character = std::make_shared<Character>(name, description, position, ActorType::Character, actors);
	std::dynamic_pointer_cast<Character>(character)->Init(1000, 1000, 100);
	character->assignedMesh = &mesh;
	character->assignedMaterial = &material;
	actors.emplace_back(std::move(character));
}

void Scene::CreateSphereLight(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart &mesh) {
	std::shared_ptr<Actor> light = std::make_shared<SphereLight>(name, description, position, ActorType::SphereLight, actors);
	std::dynamic_pointer_cast<SphereLight>(light)->SetLightColor(glm::vec3(255.0f, 197.0f, 143.0f));  //2600K 100W
	light->assignedMesh = &mesh;
	light->assignedMaterial = &materialLibrary->materials["default"];
	actors.emplace_back(std::move(light));
}

void Scene::CreateLandscape(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart &mesh, enginetool::SceneMaterial &material) {
	std::shared_ptr<Actor> stillObject = std::make_shared<Landscape>(name, description, position, ActorType::Landscape, actors);
	std::dynamic_pointer_cast<Landscape>(stillObject)->Init(1000, 1000);
	stillObject->assignedMesh = &mesh;
	stillObject->assignedMaterial = &material;
	actors.emplace_back(std::move(stillObject));
}

void Scene::CreateCloud(std::string name, std::string description, glm::vec3 position, enginetool::ScenePart &mesh) {
	std::shared_ptr<Actor> cloud = std::make_shared<Cloud>(name, description, position, ActorType::Cloud, actors);
	cloud->assignedMesh = &mesh;
	clouds.emplace_back(std::move(cloud));
}

void Scene::CreateSea(std::string name, std::string description, glm::vec3 position) {
	std::shared_ptr<Actor> sea = std::make_shared<Sea>(name, description, position, ActorType::Sea, actors);
	std::dynamic_pointer_cast<Sea>(sea)->CreateMesh();
	CreateVertexBuffer(std::dynamic_pointer_cast<Sea>(sea)->vertices, m_VertexBuffersOcean);
	CreateIndexBuffer(std::dynamic_pointer_cast<Sea>(sea)->indices, m_IndexBuffersOcean);
	seas.emplace_back(std::move(sea));
}

void Scene::CreateSkybox(std::string name, std::string description, glm::vec3 position, float horizon) {
	std::shared_ptr<Actor> skybox = std::make_shared<Skybox>(name, description, position, ActorType::Skybox, actors, horizon);
	std::dynamic_pointer_cast<Skybox>(skybox)->CreateMesh();
	CreateVertexBuffer(std::dynamic_pointer_cast<Skybox>(skybox)->vertices, m_VertexBuffersSkybox);
	CreateIndexBuffer(std::dynamic_pointer_cast<Skybox>(skybox)->indices, m_IndexBuffersSkybox);
	skyboxes.emplace_back(std::move(skybox));
}

// ------------------ Buffers ---------------------- //

void Scene::CreateVertexBuffer(std::vector<enginetool::VertexLayout>& vertices, enginetool::Buffer& vertexBuffer) {
	VkDeviceSize vertexBufferSize = sizeof(enginetool::VertexLayout) * vertices.size();
	enginetool::Buffer stagingBuffer;
	stagingBuffer.setDevice(logicalDevice);
	stagingBuffer.createStagedBuffer(static_cast<uint32_t>(vertexBufferSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertices.data());
	vertexBuffer.createUnstagedBuffer(static_cast<uint32_t>(vertexBufferSize), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	copyBuffer(&stagingBuffer, &vertexBuffer, vertexBufferSize);
	stagingBuffer.destroy();
}

void Scene::CreateIndexBuffer(std::vector<uint32_t>& indices, enginetool::Buffer& indexBuffer) {
	VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
	enginetool::Buffer stagingBuffer;
	stagingBuffer.setDevice(logicalDevice);
	stagingBuffer.createStagedBuffer(static_cast<uint32_t>(indexBufferSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indices.data());
	indexBuffer.createUnstagedBuffer(static_cast<uint32_t>(indexBufferSize), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	copyBuffer(&stagingBuffer, &indexBuffer, indexBufferSize);
	stagingBuffer.destroy();
}

void Scene::CreateMappedVertexBuffer(std::vector<enginetool::VertexLayout>& vertices, enginetool::Buffer& vertexBuffer) {
	VkDeviceSize vertexBufferSize = sizeof(enginetool::VertexLayout) * vertices.size();
	vertexBuffer.createUnstagedBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	vertexBuffer.unmap();
	vertexBuffer.map(vertexBufferSize);
}

void Scene::CreateMappedIndexBuffer(std::vector<uint32_t>& indices, enginetool::Buffer& indexBuffer){
	VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
	indexBuffer.createUnstagedBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	indexBuffer.unmap();
	indexBuffer.map(indexBufferSize);
};

void Scene::InitMaterials() {
	sky->name = "Sky_materal";
	materialLibrary->LoadSkyboxTexture(sky->skybox);
	sky->assignedPipeline = &skyboxPipeline;

	for (auto& m : materialLibrary->materials) {
		m.second.assignedPipeline = &pbrPipeline;
	}
}

// ------------------ Textures ---------------------- //

void Scene::CreateDepthResources() {
	screenDepthImage->Init(logicalDevice, commandPool, logicalDevice->FindDepthFormat(), 0, 1, 1);
	screenDepthImage->texWidth = logicalDevice->swapchain_extent.width; 
	screenDepthImage->texHeight = logicalDevice->swapchain_extent.height;
	screenDepthImage->CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	screenDepthImage->CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);
	screenDepthImage->TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	reflectionDepthImage->Init(logicalDevice, reflectionCommandPool, logicalDevice->FindDepthFormat(), 0, 1, 1);
	reflectionDepthImage->texWidth = (int32_t)logicalDevice->swapchain_extent.width; 
	reflectionDepthImage->texHeight = (int32_t)logicalDevice->swapchain_extent.height;
	reflectionDepthImage->CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	reflectionDepthImage->CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);
	reflectionDepthImage->TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	refractionDepthImage->Init(logicalDevice, refractionCommandPool, logicalDevice->FindDepthFormat(), 0, 1, 1);
	refractionDepthImage->texWidth = (int32_t)logicalDevice->swapchain_extent.width; 
	refractionDepthImage->texHeight = (int32_t)logicalDevice->swapchain_extent.height;
	refractionDepthImage->CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	refractionDepthImage->CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);
	refractionDepthImage->TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void Scene::PrepareOffscreenImage() {
	reflectionImage->Init(logicalDevice, reflectionCommandPool, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 1);
	reflectionImage->texWidth = logicalDevice->swapchain_extent.width;
	reflectionImage->texHeight = logicalDevice->swapchain_extent.height;
	reflectionImage->CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	reflectionImage->CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	reflectionImage->CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	refractionImage->Init(logicalDevice, refractionCommandPool, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 1);
	refractionImage->texWidth = logicalDevice->swapchain_extent.width;
	refractionImage->texHeight = logicalDevice->swapchain_extent.height;
	refractionImage->CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	refractionImage->CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	refractionImage->CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

// ---------------- Scene navigation ---------------- //

void Scene::TestButton() {Test();}

void Scene::MoveCameraForward() {currentCamera->Dolly(150.0f);}
void Scene::MoveCameraBackward() {currentCamera->Dolly(-150.0f);}
void Scene::StopCameraForwardBackward() {currentCamera->Dolly(0.0f);}
void Scene::MoveCameraLeft() {currentCamera->Truck(150.0f);}
void Scene::MoveCameraRight() {currentCamera->Truck(-150.0f);}
void Scene::StopCameraLeftRight() {currentCamera->Truck(0.0f);}
void Scene::MoveCameraUp() {currentCamera->Pedestal(150.0f);}
void Scene::MoveCameraDown() {currentCamera->Pedestal(-150.0f);}
void Scene::StopCameraUpDown() {currentCamera->Pedestal(0.0f);}

void Scene::MakeMainCharacterJump() {mainCharacter->SetState(ActorState::Jump);}
void Scene::MakeMainCharacterRun() {mainCharacter->SetState(ActorState::Run);}
void Scene::MoveMainCharacterForward() {mainCharacter->SetState(ActorState::WalkForward);}
void Scene::MoveMainCharacterBackward() {mainCharacter->SetState(ActorState::WalkBackward);}
void Scene::MoveMainCharacterLeft() {mainCharacter->SetState(ActorState::WalkLeft);}
void Scene::MoveMainCharacterRight() {mainCharacter->SetState(ActorState::WalkRight);}
void Scene::StopMainCharacter() {mainCharacter->SetState(ActorState::Idle);}

void Scene::MoveSelectedActorForward() {if (selectedActor!=nullptr) {selectedActor->onManualControl(); selectedActor->Dolly(150.0f);}}
void Scene::MoveSelectedActorBackward() {if (selectedActor!=nullptr) {selectedActor->onManualControl(); selectedActor->Dolly(-150.0f);}}
void Scene::StopSelectedActorForwardBackward() {if (selectedActor!=nullptr) {selectedActor->offManualControl(); selectedActor->Dolly(0.0f);}}
void Scene::MoveSelectedActorLeft() {if (selectedActor!=nullptr) {selectedActor->onManualControl(); selectedActor->Strafe(-150.0f);}}
void Scene::MoveSelectedActorRight() {if (selectedActor!=nullptr) {selectedActor->onManualControl(); selectedActor->Strafe(150.0f); }}
void Scene::StopSelectedActorLeftRight() {if (selectedActor!=nullptr) {selectedActor->offManualControl(); selectedActor->Strafe(0.0f);}}
void Scene::MoveSelectedActorUp() {if (selectedActor!=nullptr) {selectedActor->onManualControl(); selectedActor->Pedestal(150.0f);}}
void Scene::MoveSelectedActorDown() {if (selectedActor!=nullptr) {selectedActor->onManualControl(); selectedActor->Pedestal(-150.0f);}}
void Scene::StopSelectedActorUpDown() {if (selectedActor!=nullptr) {selectedActor->offManualControl(); selectedActor->Strafe(0.0f);}}

void Scene::WireframeToggle() {displayWireframe = !displayWireframe;}
void Scene::AabbToggle() {displayAabb = !displayAabb;}
void Scene::SelectionIndicatorToggle() {displaySelectionIndicator = !displaySelectionIndicator;}
void Scene::ConsoleToggle() {guiMainHub->ui_settings.display_imgui = !guiMainHub->ui_settings.display_imgui;}
void Scene::AllGuiToggle() {guiMainHub->guiOverlayVisible = !guiMainHub->guiOverlayVisible;}
void Scene::MainUiToggle() {guiMainHub->ui_settings.display_main_ui = !guiMainHub->ui_settings.display_main_ui;}
void Scene::TextOverlayToggle() {guiMainHub->ui_settings.display_stats_overlay = !guiMainHub->ui_settings.display_stats_overlay;}

// ---------------- Deinitialisation ---------------- //

void Scene::DeInitFramebuffer() {
	for (size_t i = 0; i < logicalDevice->swap_chain_framebuffers.size(); i++) {
		vkDestroyFramebuffer(logicalDevice->get(), logicalDevice->swap_chain_framebuffers[i], nullptr);
	}

	vkDestroyFramebuffer(logicalDevice->get(), logicalDevice->reflectionFramebuffer, nullptr);
	vkDestroyFramebuffer(logicalDevice->get(), logicalDevice->refractionFramebuffer, nullptr);
}

void Scene::CleanUpDepthResources() {
	screenDepthImage->DeInit();
	reflectionDepthImage->DeInit();
	refractionDepthImage->DeInit();
}

void Scene::CleanUpOffscreenImage() {
	reflectionImage->DeInit();
	refractionImage->DeInit();
}

void Scene::DeInitIndexAndVertexBuffer() {
	m_IndexBuffersMeshLibraryObjects.destroy();
	m_VertexBuffersMeshLibraryObjects.destroy();
	m_IndexBuffersOcean.destroy();
	m_VertexBuffersOcean.destroy();
	m_IndexBuffersSkybox.destroy();
	m_VertexBuffersSkybox.destroy();
	m_IndexBuffersSelectRay.destroy();
	m_VertexBuffersSelectRay.destroy();
	m_IndexBuffersAABB.destroy();
	m_VertexBuffersAABB.destroy();
}

void Scene::DeInitScene() {
	DeInitIndexAndVertexBuffer();
	DeInitTextureImage();
	vkDestroyDescriptorPool(logicalDevice->get(), descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->get(), aabbDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->get(), lineDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->get(), descriptor_set_layout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->get(), oceanDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->get(), skybox_descriptor_set_layout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->get(), cloudDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->get(), selectionIndicatorDescriptorSetLayout, nullptr);

	//CleanUpOffscreenImage();

	vkDestroyCommandPool(logicalDevice->get(), commandPool, nullptr);
	vkDestroyCommandPool(logicalDevice->get(), reflectionCommandPool, nullptr);
	vkDestroyCommandPool(logicalDevice->get(), refractionCommandPool, nullptr);
	DeInitUniformBuffer();

	delete sky;
	sky = nullptr;
	
	guiMainHub = nullptr;
	meshLibrary = nullptr;
	logicalDevice = nullptr;
}

void Scene::DeInitTextureImage() {
	sky->skybox.DeInit();
}

void Scene::DeInitUniformBuffer() {
	if (uboDataDynamic.model) {
		alignedFree(uboDataDynamic.model);
	}

	m_UboLine.destroy();
	m_UboSkybox.destroy();
	m_UboSkyboxReflection.destroy();
	m_UboSkyboxRefraction.destroy();
	m_UboOcean.destroy();
	m_UboSlectionIndicator.destroy();
	m_UboStillObjects.destroy();
	m_UboParameters.destroy();
	m_UboClouds.destroy();
	m_UboCloudsDynamic.destroy();
	m_UboReflection.destroy();
	m_UboReflectionParameters.destroy();
	m_UboRefraction.destroy();
	m_UboRefractionParameters.destroy();
}

void Scene::DestroyPipeline() {
	vkDestroyPipeline(logicalDevice->get(), aabbPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), selectRayPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), skyboxPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), skyboxWireframePipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), oceanPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), oceanWireframePipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), pbrPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), pbrWireframePipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), cloudsPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), cloudsWireframePipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), pbrReflectionPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), pbrRefractionPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), skyboxReflectionPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), skyboxRefractionPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->get(), selectionIndicatorPipeline, nullptr);

	vkDestroyPipelineLayout(logicalDevice->get(), pipelineLayout, nullptr);
}

void Scene::FreeCommandBuffers() {
	vkFreeCommandBuffers(logicalDevice->get(), commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	vkFreeCommandBuffers(logicalDevice->get(), reflectionCommandPool, 1, &reflectionCmdBuff);
	vkFreeCommandBuffers(logicalDevice->get(), refractionCommandPool, 1, &refractionCmdBuff);
}