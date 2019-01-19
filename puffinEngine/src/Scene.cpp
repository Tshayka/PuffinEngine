#include <algorithm> // "max" and "min" in VkExtent2D
#include <fstream>
#include <gli/gli.hpp>
#include <iostream>
#include <random>
#include <stb_image.h>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h> // loading obj files

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

void Scene::InitScene(Device* device, GLFWwindow* window, GuiMainHub* guiMainHub, MousePicker* mousePicker) {
	logicalDevice = device;
	this->guiMainHub = guiMainHub;
	this->mousePicker = mousePicker;
	this->window = window;

	InitSwapchainImageViews();
	CreateCommandPool();
	CreateDepthResources();
	PrepareOffscreen();
	CreateFramebuffers();
	LoadAssets();
	CreateUniformBuffer();
	CreateDescriptorPool();
	CreateDescriptorSetLayout();
	CreateDescriptorSet();
	CreateGraphicsPipeline();
	UpdateGUI(0.0f, 0);
	CreateCommandBuffers();
	CreateReflectionCommandBuffer();
	CreateRefractionCommandBuffer();
}

void Scene::CleanUpForSwapchain() {
	DeInitDepthResources();
	DeInitFramebuffer();
	FreeCommandBuffers();
	DestroyPipeline();
	vkDestroyPipelineLayout(logicalDevice->device, pipelineLayout, nullptr);
}

void Scene::RecreateForSwapchain() {
	InitSwapchainImageViews();
	CreateDepthResources();
	PrepareOffscreen();
	CreateFramebuffers();
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

VkImageView Scene::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)//TODO use in swapchain textureLayout class
{
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
	if (vkCreateImageView(logicalDevice->device, &ViewInfo, nullptr, &image_view) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to create texture image view!");
		std::exit(-1);
	}

	return image_view;
}

// ----------------- Framebuffer -------------------- //

void Scene::CreateFramebuffers() {
	// Reflection frambuffer
	std::array<VkImageView, 2> reflectionAttachments = {offscreenPass.reflectionImage.texture_image_view, offscreenPass.reflectionDepthImage.texture_image_view};
	
	VkFramebufferCreateInfo reflectionFramebufferInfo = {};
	reflectionFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	reflectionFramebufferInfo.renderPass = logicalDevice->offscreenRenderPass;
	reflectionFramebufferInfo.attachmentCount = static_cast<uint32_t>(reflectionAttachments.size());
	reflectionFramebufferInfo.pAttachments = reflectionAttachments.data();
	reflectionFramebufferInfo.width = logicalDevice->swapchain_extent.width;
	reflectionFramebufferInfo.height = logicalDevice->swapchain_extent.height;
	reflectionFramebufferInfo.layers = 1;

	ErrorCheck(vkCreateFramebuffer(logicalDevice->device, &reflectionFramebufferInfo, nullptr, &logicalDevice->reflectionFramebuffer));

	// Refraction frambuffer
	std::array<VkImageView, 2> refractionAttachments = {offscreenPass.refractionImage.texture_image_view, offscreenPass.refractionDepthImage.texture_image_view};
	
	VkFramebufferCreateInfo refractionFramebufferInfo = {};
	refractionFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	refractionFramebufferInfo.renderPass = logicalDevice->offscreenRenderPass;
	refractionFramebufferInfo.attachmentCount = static_cast<uint32_t>(refractionAttachments.size());
	refractionFramebufferInfo.pAttachments = refractionAttachments.data();
	refractionFramebufferInfo.width = logicalDevice->swapchain_extent.width;
	refractionFramebufferInfo.height = logicalDevice->swapchain_extent.height;
	refractionFramebufferInfo.layers = 1;

	ErrorCheck(vkCreateFramebuffer(logicalDevice->device, &refractionFramebufferInfo, nullptr, &logicalDevice->refractionFramebuffer));

	// Screen frambuffer
	logicalDevice->swap_chain_framebuffers.resize(logicalDevice->swapchainImageViews.size());

	for (size_t i = 0; i < logicalDevice->swapchainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = { logicalDevice->swapchainImageViews[i], depthImage.texture_image_view };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = logicalDevice->renderPass; // specify with which render pass the framebuffer needs to be compatible
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = logicalDevice->swapchain_extent.width;
		framebufferInfo.height = logicalDevice->swapchain_extent.height;
		framebufferInfo.layers = 1; // swap chain images are single images,

		ErrorCheck(vkCreateFramebuffer(logicalDevice->device, &framebufferInfo, nullptr, &logicalDevice->swap_chain_framebuffers[i]));
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

	ErrorCheck(vkCreateCommandPool(logicalDevice->device, &poolInfo, nullptr, &commandPool));
}

VkCommandBuffer Scene::BeginSingleTimeCommands() {
	VkCommandBufferAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocInfo.commandPool = commandPool;
	AllocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDevice->device, &AllocInfo, &commandBuffer);

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

	vkFreeCommandBuffers(logicalDevice->device, commandPool, 1, &commandBuffer);
}

void Scene::CopyBuffer(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, const VkDeviceSize& size) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer, commandPool);
}

// -------------- Graphics pipeline ----------------- //

void Scene::CreateGraphicsPipeline() {
	VkPushConstantRange PushConstantRange = {};
	PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	PushConstantRange.offset = 0;
	PushConstantRange.size = sizeof(Constants);

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

	std::array<VkDescriptorSetLayout, 6> layouts = { descriptor_set_layout, 
													skybox_descriptor_set_layout, 
													clouds_descriptor_set_layout, 
													oceanDescriptorSetLayout, 
													lineDescriptorSetLayout,
													selectionIndicatorDescriptorSetLayout };

	VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
	PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutInfo.pushConstantRangeCount = 1;
	PipelineLayoutInfo.pPushConstantRanges = &PushConstantRange;
	PipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	PipelineLayoutInfo.pSetLayouts = layouts.data();
	
	ErrorCheck(vkCreatePipelineLayout(logicalDevice->device, &PipelineLayoutInfo, nullptr, &pipelineLayout));

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
	auto vertCubeMapShaderCode = enginetool::readFile("puffinEngine/shaders/skymap_shader.vert.spv"); 
	auto fragCubeMapShaderCode = enginetool::readFile("puffinEngine/shaders/skymap_shader.frag.spv"); 

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

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &skyboxPipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_LINE; 
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &skyboxWireframePipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL; 

	// Skybox refraction pipeline
	PipelineInfo.renderPass = logicalDevice->offscreenRenderPass;
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &skyboxRefractionPipeline));

	// Skybox reflection pipeline
	Rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	PipelineInfo.renderPass = logicalDevice->offscreenRenderPass;
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &skyboxReflectionPipeline));
	
	Rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
	PipelineInfo.renderPass = logicalDevice->renderPass;

	vkDestroyShaderModule(logicalDevice->device, fragCubeMapShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->device, vertCubeMapShaderModule, nullptr);
	
	// II. Models pipeline
	auto vertModelsShaderCode = enginetool::readFile("puffinEngine/shaders/pbr_shader.vert.spv");
	auto fragModelsShaderCode = enginetool::readFile("puffinEngine/shaders/pbr_shader.frag.spv");

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

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &pbrPipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_LINE; 
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &pbrWireframePipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	
	// Models refraction pipeline
	PipelineInfo.renderPass = logicalDevice->offscreenRenderPass;
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &pbrRefractionPipeline));

	// Models reflection pipeline
	Rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	PipelineInfo.renderPass = logicalDevice->offscreenRenderPass;
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &pbrReflectionPipeline));

	vkDestroyShaderModule(logicalDevice->device, fragModelsShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->device, vertModelsShaderModule, nullptr);
	
	Rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
	PipelineInfo.renderPass = logicalDevice->renderPass;

	// III. Selection crystal pipeline
	auto vertSelectionCrystalShaderCode = enginetool::readFile("puffinEngine/shaders/selectionCrystalShader.vert.spv");
	auto fragSelectionCrystalShaderCode = enginetool::readFile("puffinEngine/shaders/selectionCrystalShader.frag.spv");

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

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &selectionIndicatorPipeline));

	vkDestroyShaderModule(logicalDevice->device, fragSelectionCrystalShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->device, vertSelectionCrystalShaderModule, nullptr);

	// IV. Ocean pipeline
	auto vertOceanShaderCode = enginetool::readFile("puffinEngine/shaders/ocean_shader.vert.spv");
	auto fragOceanShaderCode = enginetool::readFile("puffinEngine/shaders/ocean_shader.frag.spv");

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

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &oceanPipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_LINE; 
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &oceanWireframePipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL; 

	vkDestroyShaderModule(logicalDevice->device, fragOceanShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->device, vertOceanShaderModule, nullptr);
	
	// V. Line pipeline
	auto vertLineShaderCode = enginetool::readFile("puffinEngine/shaders/lineShader.vert.spv");
	auto fragLineShaderCode = enginetool::readFile("puffinEngine/shaders/lineShader.frag.spv");

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
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &selectRayPipeline));
	Rasterization.lineWidth = 1.0f;
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &aabbPipeline));
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL;

	vkDestroyShaderModule(logicalDevice->device, fragLineShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->device, vertLineShaderModule, nullptr); 

	// VI. Clouds pipeline
	auto vertCloudsShaderCode = enginetool::readFile("puffinEngine/shaders/clouds_shader.vert.spv");
	auto fragCloudsShaderCode = enginetool::readFile("puffinEngine/shaders/clouds_shader.frag.spv");

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
	
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &cloudsPipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_LINE; 
	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &cloudsWireframePipeline));
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL; 

	vkDestroyShaderModule(logicalDevice->device, fragCloudsShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->device, vertCloudsShaderModule, nullptr);
}

void Scene::CreateCommandBuffers() {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = logicalDevice->renderPass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = logicalDevice->swapchain_extent.width;
	renderPassInfo.renderArea.extent.height = logicalDevice->swapchain_extent.height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	command_buffers.resize(logicalDevice->swap_chain_framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if the allocated command buffers are primary or secondary, here "primary" can be submitted to a queue for execution, but cannot be called from other command buffers
	allocInfo.commandBufferCount = (uint32_t)command_buffers.size();

	ErrorCheck(vkAllocateCommandBuffers(logicalDevice->device, &allocInfo, command_buffers.data()));
	
	// starting command buffer recording
	for (size_t i = 0; i < command_buffers.size(); i++)	{
		// Set target frame buffer
		renderPassInfo.framebuffer = logicalDevice->swap_chain_framebuffers[i];
		vkBeginCommandBuffer(command_buffers[i], &beginInfo);
		vkCmdBeginRenderPass(command_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)logicalDevice->swapchain_extent.width;
		viewport.height = (float)logicalDevice->swapchain_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffers[i], 0, 1, &viewport);

		scissor.offset = { 0, 0 }; // scissor rectangle covers framebuffer entirely
		scissor.extent = logicalDevice->swapchain_extent;
		vkCmdSetScissor(command_buffers[i], 0, 1, &scissor);

		VkDeviceSize offsets[1] = { 0 };

		if(displaySelectionIndicator && selectedActor!=nullptr) {
			float pointerOffset = selectedActor->position.y + abs(selectedActor->mesh.aabb.max.y)+abs(selectionIndicatorMesh.aabb.max.y)+0.25f;
			pushConstants.renderLimitPlane = glm::vec4(0.0f, 0.0f, 0.0f, horizon );
			pushConstants.color = selectedActor->CalculateSelectionIndicatorColor();
			pushConstants.pos = glm::vec3(selectedActor->position.x, pointerOffset, selectedActor->position.z);
			vkCmdPushConstants(command_buffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Constants), &pushConstants);
			
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 5, 1, &selectionIndicatorDescriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers.selectionIndicator.buffer, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffers.selectionIndicator.buffer , 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, selectionIndicatorPipeline);
			vkCmdDrawIndexed(command_buffers[i], static_cast<uint32_t>(selectIndicatorVertices.size()), 1, 0, 0, 0);
		}

		if (displaySkybox)	{
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &skybox_descriptor_set, 0, nullptr);
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers.skybox.buffer, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffers.skybox.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (displayWireframe) ? (skyboxWireframePipeline) : (skyboxPipeline));
			vkCmdDrawIndexed(command_buffers[i], static_cast<uint32_t>(skybox_indices.size()), 1, 0, 0, 0);
		}

		if (displayOcean) {
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1, &oceanDescriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers.ocean.buffer, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffers.ocean.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (displayWireframe) ? (oceanWireframePipeline) : (oceanPipeline));
			vkCmdDrawIndexed(command_buffers[i], static_cast<uint32_t>(oceanIndices.size()), 1, 0, 0, 0);
		}

		if (displaySceneGeometry) {
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers.objects.buffer, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffers.objects.buffer , 0, VK_INDEX_TYPE_UINT32);

			for (size_t j = 0; j < actors.size(); j++) {
				std::array<VkDescriptorSet, 1> descriptorSets;
				descriptorSets[0] = actors[j]->mesh.assigned_material->descriptor_set;
				vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
				vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (displayWireframe) ? (pbrWireframePipeline) : (*actors[j]->mesh.assigned_material->assigned_pipeline));
				pushConstants.pos = actors[j]->position;
				pushConstants.renderLimitPlane = glm::vec4(0.0f, 0.0f, 0.0f, horizon );
				vkCmdPushConstants(command_buffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Constants), &pushConstants);
				vkCmdDrawIndexed(command_buffers[i], actors[j]->mesh.indexCount, 1, 0, actors[j]->mesh.indexBase, 0);
			}
		}

		if (displayClouds)	{
			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (displayWireframe) ? (cloudsWireframePipeline) : (cloudsPipeline));
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers.clouds.buffer, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffers.clouds.buffer, 0, VK_INDEX_TYPE_UINT32);

			for (uint32_t k = 0; k < DYNAMIC_UB_OBJECTS; k++) {
				uint32_t dynamic_offset = k * static_cast<uint32_t>(dynamicAlignment);
				vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &clouds_descriptor_set, 1, &dynamic_offset);
				vkCmdDrawIndexed(command_buffers[i], static_cast<uint32_t>(clouds_indices.size()), 1, 0, 0, 0);
			}
		}

		if(displayWireframe) {
			UpdateSelectRayDrawData();
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 4, 1, &lineDescriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers.selectRay.buffer, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffers.selectRay.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, selectRayPipeline);
			vkCmdDrawIndexed(command_buffers[i], 2, 1, 0, 0, 0);
		}

		if(displayAabbBorders) {
			UpdateAABBDrawData();
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers.aabb.buffer, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffers.aabb.buffer , 0, VK_INDEX_TYPE_UINT32);

			for (size_t k = 0; k < actors.size(); k++) {
				vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 4, 1, &lineDescriptorSet, 0, nullptr);
				vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, aabbPipeline);
				vkCmdDrawIndexed(command_buffers[i], aabbIndices.size(), 1, 0, k*8, 0);				
			}
		}

		vkCmdEndRenderPass(command_buffers[i]);
		ErrorCheck(vkEndCommandBuffer(command_buffers[i]));
	}
}

void Scene::CreateReflectionCommandBuffer() {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
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
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if the allocated command buffers are primary or secondary, here "primary" can be submitted to a queue for execution, but cannot be called from other command buffers
	allocInfo.commandBufferCount = 1;

	ErrorCheck(vkAllocateCommandBuffers(logicalDevice->device, &allocInfo, &reflectionCmdBuff));
	
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
		vkCmdBindVertexBuffers(reflectionCmdBuff, 0, 1, &vertex_buffers.skybox.buffer, offsets);
		vkCmdBindIndexBuffer(reflectionCmdBuff, index_buffers.skybox.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(reflectionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxReflectionPipeline);
		vkCmdDrawIndexed(reflectionCmdBuff, static_cast<uint32_t>(skybox_indices.size()), 1, 0, 0, 0);
	}

	// 3d object
	vkCmdBindVertexBuffers(reflectionCmdBuff, 0, 1, &vertex_buffers.objects.buffer, offsets);
	vkCmdBindIndexBuffer(reflectionCmdBuff, index_buffers.objects.buffer , 0, VK_INDEX_TYPE_UINT32);

	for (size_t j = 0; j < actors.size(); j++) {
		// reflection
		std::array<VkDescriptorSet, 1> descriptorSets;
		descriptorSets[0] = actors[j]->mesh.assigned_material->reflectDescriptorSet;
		vkCmdBindDescriptorSets(reflectionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
		vkCmdBindPipeline(reflectionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pbrReflectionPipeline);
		pushConstants.pos = actors[j]->position;
		pushConstants.renderLimitPlane = (currentCamera->position.y<0) ? (glm::vec4(0.0f, -1.0f, 0.0f, -0.0f)) : (glm::vec4(0.0f, 1.0f, 0.0f, -0.0f));

		vkCmdPushConstants(reflectionCmdBuff, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Constants), &pushConstants);
		vkCmdDrawIndexed(reflectionCmdBuff, actors[j]->mesh.indexCount, 1, 0, actors[j]->mesh.indexBase, 0);
	}

	vkCmdEndRenderPass(reflectionCmdBuff);
	ErrorCheck(vkEndCommandBuffer(reflectionCmdBuff));
}

void Scene::CreateRefractionCommandBuffer() { 
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
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
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if the allocated command buffers are primary or secondary, here "primary" can be submitted to a queue for execution, but cannot be called from other command buffers
	allocInfo.commandBufferCount = 1;

	ErrorCheck(vkAllocateCommandBuffers(logicalDevice->device, &allocInfo, &refractionCmdBuff));
	
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
		vkCmdBindVertexBuffers(refractionCmdBuff, 0, 1, &vertex_buffers.skybox.buffer, offsets);
		vkCmdBindIndexBuffer(refractionCmdBuff, index_buffers.skybox.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(refractionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxRefractionPipeline);
		vkCmdDrawIndexed(refractionCmdBuff, static_cast<uint32_t>(skybox_indices.size()), 1, 0, 0, 0);
	}

	// 3d object
	vkCmdBindVertexBuffers(refractionCmdBuff, 0, 1, &vertex_buffers.objects.buffer, offsets);
	vkCmdBindIndexBuffer(refractionCmdBuff, index_buffers.objects.buffer , 0, VK_INDEX_TYPE_UINT32);

	for (size_t j = 0; j < actors.size(); j++) {
		std::array<VkDescriptorSet, 1> descriptorSets;
		descriptorSets[0] = actors[j]->mesh.assigned_material->refractDescriptorSet;
		vkCmdBindDescriptorSets(refractionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
		vkCmdBindPipeline(refractionCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pbrRefractionPipeline);
		pushConstants.pos = actors[j]->position;
		pushConstants.renderLimitPlane = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f );
		vkCmdPushConstants(refractionCmdBuff, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Constants), &pushConstants);
		vkCmdDrawIndexed(refractionCmdBuff, actors[j]->mesh.indexCount, 1, 0, actors[j]->mesh.indexBase, 0);
	}

	vkCmdEndRenderPass(refractionCmdBuff);
	ErrorCheck(vkEndCommandBuffer(refractionCmdBuff));
}

void Scene::CreateUniformBuffer() {
	// Ocean Uniform buffers memory -> static
	logicalDevice->CreateUnstagedBuffer(sizeof(UboSea), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.ocean);
	uniform_buffers.ocean.Map();
	
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

	// Static shared uniform buffer object with projection and view matrix
	logicalDevice->CreateUnstagedBuffer(sizeof(UboClouds), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.clouds);
	uniform_buffers.clouds.Map();

	// Uniform buffer object with per-object matrices
	logicalDevice->CreateUnstagedBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &uniform_buffers.clouds_dynamic);
	uniform_buffers.clouds_dynamic.Map();

	// Rotating Objects Uniform buffers memory -> static
	logicalDevice->CreateUnstagedBuffer(sizeof(UboSelectionIndicator), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.selectionIndicator);
	uniform_buffers.selectionIndicator.Map();

	// Objects Uniform buffers memory -> static
	logicalDevice->CreateUnstagedBuffer(sizeof(UboStaticGeometry), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.stillObjects);
	uniform_buffers.stillObjects.Map();

	// Objects Uniform buffers for reflection rendering -> static
	logicalDevice->CreateUnstagedBuffer(sizeof(UboStaticGeometry), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.reflection);
	uniform_buffers.reflection.Map();

	// Objects Uniform buffers for refraction rendering -> static
	logicalDevice->CreateUnstagedBuffer(sizeof(UboStaticGeometry), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.refraction);
	uniform_buffers.refraction.Map();

	// Additional uniform bufer for parameters -> static
	logicalDevice->CreateUnstagedBuffer(sizeof(UboParam), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.parameters);
	uniform_buffers.parameters.Map();

	// Additional uniform bufer parameters for reflection scene -> static
	logicalDevice->CreateUnstagedBuffer(sizeof(UboParam), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.reflectionParameters);
	uniform_buffers.reflectionParameters.Map();

	// Additional uniform bufer parameters for refraction scene -> static
	logicalDevice->CreateUnstagedBuffer(sizeof(UboParam), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.refractionParameters);
	uniform_buffers.refractionParameters.Map();

	// Skybox Uniform buffers memory -> static
	logicalDevice->CreateUnstagedBuffer(sizeof(UboSkybox), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.skybox);
	uniform_buffers.skybox.Map();

	logicalDevice->CreateUnstagedBuffer(sizeof(UboSkybox), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.skyboxReflection);
	uniform_buffers.skyboxReflection.Map();

	logicalDevice->CreateUnstagedBuffer(sizeof(UboSkybox), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.skyboxRefraction);
	uniform_buffers.skyboxRefraction.Map();

	// Line uniform buffer -> static
	logicalDevice->CreateUnstagedBuffer(sizeof(UboStaticGeometry), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.line);
	uniform_buffers.line.Map();

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

void Scene::UpdateScene(const float &dt, const float &time, float const &accumulator) {
	UpdateGUI((float)accumulator, (uint32_t)time);

	UpdateDynamicUniformBuffer(time);
	UpdateSkyboxUniformBuffer();
	UpdateOceanUniformBuffer(time);
	UpdateUBOParameters();
	UpdateStaticUniformBuffer(time);
	UpdateSelectionIndicatorUniformBuffer(time);
	UpdateUBOOffscreen(time);

	for(const auto& a : actors) a->UpdatePosition(dt);
	for(const auto& c : sceneCameras) c->UpdatePosition(dt);
	
	CreateCommandBuffers(); 
	CreateReflectionCommandBuffer();
	CreateRefractionCommandBuffer();

	//std::dynamic_pointer_cast<Character>(actors[1])->closestPointBelow = DetectGround();
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

void Scene::SelectActor() {
	glm::vec3 dirFrac;
	dirFrac.x = 1.0f / mousePicker->GetRayDirection().x;
	dirFrac.y = 1.0f / mousePicker->GetRayDirection().y;
	dirFrac.z = 1.0f / mousePicker->GetRayDirection().z;

	glm::vec3 rayOrigin = mousePicker->GetRayOrigin();

	for (auto const& a : actors) {
		if(enginetool::ScenePart::RayIntersection(mousePicker->hitPoint, dirFrac, rayOrigin, mousePicker->GetRayDirection(), a->currentAabb)) {
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

float Scene::DetectGround() {
	return 0.0f;
}

void Scene::DeSelect() {
	selectedActor = nullptr;
	std::cout << "Deselected" << std::endl;
}

void Scene::UpdateStaticUniformBuffer(const float& time) {
	UBOSG.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOSG.proj[1][1] *= -1; //since the Y axis of Vulkan NDC points down
	UBOSG.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOSG.model = glm::mat4(1.0f);
	UBOSG.cameraPos = glm::vec3(currentCamera->position);
	memcpy(uniform_buffers.stillObjects.mapped, &UBOSG, sizeof(UBOSG));
	memcpy(uniform_buffers.line.mapped, &UBOSG, sizeof(UBOSG));
	mousePicker->UpdateMousePicker(UBOSG.view, UBOSG.proj, currentCamera);

	UBOC.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOC.proj[1][1] *= -1; 
	UBOC.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOC.time = time;
	UBOC.view[3][0] *= 0;
	UBOC.view[3][1] *= 0;
	UBOC.view[3][2] *= 0;
	UBOC.cameraPos = currentCamera->position;
	memcpy(uniform_buffers.clouds.mapped, &UBOC, sizeof(UBOC));	
}

void Scene::UpdateSelectionIndicatorUniformBuffer(const float& time) {
	UBOSI.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOSI.proj[1][1] *= -1; 
	UBOSI.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOSI.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), currentCamera->up);
	UBOSI.cameraPos = glm::vec3(currentCamera->position);
	UBOSI.time = time;
	memcpy(uniform_buffers.selectionIndicator.mapped, &UBOSI, sizeof(UBOSI));
}

void Scene::UpdateUBOOffscreen(const float& time) {
	UBOO.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOO.proj[1][1] *= -1; 
	UBOO.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOO.model = glm::mat4(1.0f);
	UBOO.cameraPos = glm::vec3(currentCamera->position);
	memcpy(uniform_buffers.refraction.mapped, &UBOO, sizeof(UBOO));
	UBOO.view[1][0] *= -1;
	UBOO.view[1][1] *= -1;
	UBOO.view[1][2] *= -1;
	UBOO.cameraPos *= glm::vec3(1.0f, -1.0f, 1.0f);
	memcpy(uniform_buffers.reflection.mapped, &UBOO, sizeof(UBOO));
}

void Scene::UpdateUBOParameters() {
	UBOP.light_col = std::dynamic_pointer_cast<SphereLight>(actors[2])->GetLightColor();
	UBOP.exposure = 2.5f;
	UBOP.light_pos[0] = actors[2]->position;
	memcpy(uniform_buffers.parameters.mapped, &UBOP, sizeof(UBOP));
	memcpy(uniform_buffers.refractionParameters.mapped, &UBOP, sizeof(UBOP));

	UBOP.light_pos[0]*= glm::vec3(1.0f, -1.0f, 1.0f);
	memcpy(uniform_buffers.reflectionParameters.mapped, &UBOP, sizeof(UBOP));
}

void Scene::UpdateDynamicUniformBuffer(const float& time) {
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

		memcpy(uniform_buffers.clouds_dynamic.mapped, uboDataDynamic.model, uniform_buffers.clouds_dynamic.size);

		// Flush to make changes visible to the host 
		VkMappedMemoryRange memoryRange = {};
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.memory = uniform_buffers.clouds_dynamic.memory;
		memoryRange.size = uniform_buffers.clouds_dynamic.size;
		
		vkFlushMappedMemoryRanges(logicalDevice->device, 1, &memoryRange);
}

void Scene::UpdateSkyboxUniformBuffer() {
	UBOSB.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOSB.proj[1][1] *= -1;
	UBOSB.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOSB.view[3][0] *= 0;
	UBOSB.view[3][1] *= 0;
	UBOSB.view[3][2] *= 0;
	UBOSB.exposure = 1.0f;
	UBOSB.gamma = 1.0f;
	memcpy(uniform_buffers.skybox.mapped, &UBOSB, sizeof(UBOSB));
	memcpy(uniform_buffers.skyboxRefraction.mapped, &UBOSB, sizeof(UBOSB));

	UBOSB.view[1][0] *= -1;
	UBOSB.view[1][1] *= -1;
	UBOSB.view[1][2] *= -1;
	memcpy(uniform_buffers.skyboxReflection.mapped, &UBOSB, sizeof(UBOSB));
}

void Scene::UpdateOceanUniformBuffer(const float& time) {
	UBOSE.model = glm::mat4(1.0f);
	UBOSE.proj = glm::perspective(glm::radians(currentCamera->FOV), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, currentCamera->clippingNear, currentCamera->clippingFar);
	UBOSE.proj[1][1] *= -1;
	UBOSE.view = glm::lookAt(currentCamera->position, currentCamera->view, currentCamera->up);
	UBOSE.cameraPos = currentCamera->position;
	UBOSE.time = time;
	memcpy(uniform_buffers.ocean.mapped, &UBOSE, sizeof(UBOSE));
}

// ------ Text overlay - performance statistics ----- //

void Scene::UpdateGUI(float frameTimer, uint32_t elapsedTime) {
	guiMainHub->UpdateCommandBuffers(frameTimer, elapsedTime);
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

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->device, &SceneObjectsLayoutInfo, nullptr, &descriptor_set_layout));

	// Skybox
	VkDescriptorSetLayoutBinding SkyboxUBOLayoutBinding = {};
	SkyboxUBOLayoutBinding.binding = 0;
	SkyboxUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	SkyboxUBOLayoutBinding.descriptorCount = 1;
	SkyboxUBOLayoutBinding.pImmutableSamplers = nullptr;
	SkyboxUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding SkyboxImageLayoutBinding = {};
	SkyboxImageLayoutBinding.binding = 1;
	SkyboxImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SkyboxImageLayoutBinding.descriptorCount = 1;
	SkyboxImageLayoutBinding.pImmutableSamplers = nullptr;
	SkyboxImageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> skybox_bindings = { SkyboxUBOLayoutBinding, SkyboxImageLayoutBinding };

	VkDescriptorSetLayoutCreateInfo SkyboxLayoutInfo = {};
	SkyboxLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	SkyboxLayoutInfo.bindingCount = static_cast<uint32_t>(skybox_bindings.size());
	SkyboxLayoutInfo.pBindings = skybox_bindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->device, &SkyboxLayoutInfo, nullptr, &skybox_descriptor_set_layout));
	
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

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->device, &CloudsLayoutInfo, nullptr, &clouds_descriptor_set_layout));

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

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->device, &OceanLayoutInfo, nullptr, &oceanDescriptorSetLayout));

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

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->device, &LineLayoutInfo, nullptr, &lineDescriptorSetLayout));

	// Selection indicator
	VkDescriptorSetLayoutBinding SelectionIndicatorUboLayoutBinding = {};
	SelectionIndicatorUboLayoutBinding.binding = 0;
	SelectionIndicatorUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	SelectionIndicatorUboLayoutBinding.descriptorCount = 1;
	SelectionIndicatorUboLayoutBinding.pImmutableSamplers = nullptr;
	SelectionIndicatorUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> selectionIndicatorBindings = { SelectionIndicatorUboLayoutBinding };

	VkDescriptorSetLayoutCreateInfo SelectionIndicatorLayoutInfo = {};
	SelectionIndicatorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	SelectionIndicatorLayoutInfo.bindingCount = static_cast<uint32_t>(selectionIndicatorBindings.size());
	SelectionIndicatorLayoutInfo.pBindings = selectionIndicatorBindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->device, &SelectionIndicatorLayoutInfo, nullptr, &selectionIndicatorDescriptorSetLayout));
}

void Scene::CreateDescriptorPool() {
	// Don't forget to rise this numbers when you add bindings
	std::array<VkDescriptorPoolSize, 3> PoolSizes = {};
	PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	PoolSizes[0].descriptorCount = static_cast<uint32_t>(1);
	PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	PoolSizes[1].descriptorCount = static_cast<uint32_t>(scene_material.size() * 6 * 3 + 10);
	PoolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	PoolSizes[2].descriptorCount = static_cast<uint32_t>(scene_material.size() * 6 + 9);

	VkDescriptorPoolCreateInfo PoolInfo = {};
	PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.maxSets = static_cast<uint32_t>(scene_material.size()*3 + 7); // maximum number of descriptor sets that will be allocated

	ErrorCheck(vkCreateDescriptorPool(logicalDevice->device, &PoolInfo, nullptr, &descriptorPool));
}

void Scene::CreateDescriptorSet() {
	// 3D object descriptor set
	for (size_t i = 0; i < scene_material.size(); i++) {
		VkDescriptorImageInfo IrradianceMapImageInfo = {};
		IrradianceMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		IrradianceMapImageInfo.imageView = sky->skybox_texture.texture_image_view;
		IrradianceMapImageInfo.sampler = sky->skybox_texture.texture_sampler;

		VkDescriptorImageInfo AlbedoImageInfo = {};
		AlbedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		AlbedoImageInfo.imageView = scene_material[i].albedo.texture_image_view;
		AlbedoImageInfo.sampler = scene_material[i].albedo.texture_sampler;

		VkDescriptorImageInfo MettalicImageInfo = {};
		MettalicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		MettalicImageInfo.imageView = scene_material[i].metallic.texture_image_view;
		MettalicImageInfo.sampler = scene_material[i].metallic.texture_sampler;

		VkDescriptorImageInfo RoughnessImageInfo = {};
		RoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		RoughnessImageInfo.imageView = scene_material[i].roughness.texture_image_view;
		RoughnessImageInfo.sampler = scene_material[i].roughness.texture_sampler;

		VkDescriptorImageInfo NormalImageInfo = {};
		NormalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		NormalImageInfo.imageView = scene_material[i].normal_map.texture_image_view;
		NormalImageInfo.sampler = scene_material[i].normal_map.texture_sampler;

		VkDescriptorImageInfo AmbientOcclusionImageInfo = {};
		AmbientOcclusionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		AmbientOcclusionImageInfo.imageView = scene_material[i].ambient_occlucion_map.texture_image_view;
		AmbientOcclusionImageInfo.sampler = scene_material[i].ambient_occlucion_map.texture_sampler;

		VkDescriptorSetAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfo.descriptorPool = descriptorPool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &descriptor_set_layout;

		ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &AllocInfo, &scene_material[i].descriptor_set));
		ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &AllocInfo, &scene_material[i].reflectDescriptorSet));
		ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &AllocInfo, &scene_material[i].refractDescriptorSet));

		VkDescriptorBufferInfo BufferInfo = {};
		BufferInfo.buffer = uniform_buffers.stillObjects.buffer;
		BufferInfo.offset = 0;
		BufferInfo.range = sizeof(UboStaticGeometry);

		VkDescriptorBufferInfo ObjectBufferParametersInfo = {};
		ObjectBufferParametersInfo.buffer = uniform_buffers.parameters.buffer;
		ObjectBufferParametersInfo.offset = 0;
		ObjectBufferParametersInfo.range = sizeof(UboParam);

		std::array<VkWriteDescriptorSet, 8> objectDescriptorWrites = {};

		objectDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[0].dstSet = scene_material[i].descriptor_set;
		objectDescriptorWrites[0].dstBinding = 0;
		objectDescriptorWrites[0].dstArrayElement = 0;
		objectDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		objectDescriptorWrites[0].descriptorCount = 1;
		objectDescriptorWrites[0].pBufferInfo = &BufferInfo;

		objectDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[1].dstSet = scene_material[i].descriptor_set;
		objectDescriptorWrites[1].dstBinding = 1;
		objectDescriptorWrites[1].dstArrayElement = 0;
		objectDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		objectDescriptorWrites[1].descriptorCount = 1;
		objectDescriptorWrites[1].pBufferInfo = &ObjectBufferParametersInfo;

		objectDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[2].dstSet = scene_material[i].descriptor_set;
		objectDescriptorWrites[2].dstBinding = 2;
		objectDescriptorWrites[2].dstArrayElement = 0;
		objectDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[2].descriptorCount = 1;
		objectDescriptorWrites[2].pImageInfo = &IrradianceMapImageInfo;

		objectDescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[3].dstSet = scene_material[i].descriptor_set;
		objectDescriptorWrites[3].dstBinding = 3;
		objectDescriptorWrites[3].dstArrayElement = 0;
		objectDescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[3].descriptorCount = 1;
		objectDescriptorWrites[3].pImageInfo = &AlbedoImageInfo;

		objectDescriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[4].dstSet = scene_material[i].descriptor_set;
		objectDescriptorWrites[4].dstBinding = 4;
		objectDescriptorWrites[4].dstArrayElement = 0;
		objectDescriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[4].descriptorCount = 1;
		objectDescriptorWrites[4].pImageInfo = &MettalicImageInfo;

		objectDescriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[5].dstSet = scene_material[i].descriptor_set;
		objectDescriptorWrites[5].dstBinding = 5;
		objectDescriptorWrites[5].dstArrayElement = 0;
		objectDescriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[5].descriptorCount = 1;
		objectDescriptorWrites[5].pImageInfo = &RoughnessImageInfo;

		objectDescriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[6].dstSet = scene_material[i].descriptor_set;
		objectDescriptorWrites[6].dstBinding = 6;
		objectDescriptorWrites[6].dstArrayElement = 0;
		objectDescriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[6].descriptorCount = 1;
		objectDescriptorWrites[6].pImageInfo = &NormalImageInfo;

		objectDescriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectDescriptorWrites[7].dstSet = scene_material[i].descriptor_set;
		objectDescriptorWrites[7].dstBinding = 7;
		objectDescriptorWrites[7].dstArrayElement = 0;
		objectDescriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectDescriptorWrites[7].descriptorCount = 1;
		objectDescriptorWrites[7].pImageInfo = &AmbientOcclusionImageInfo;

		vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(objectDescriptorWrites.size()), objectDescriptorWrites.data(), 0, nullptr);

		// Copy above descriptor set values to reflection ad refracion

		VkDescriptorBufferInfo reflectBufferInfo = {};
		reflectBufferInfo.buffer = uniform_buffers.reflection.buffer;
		reflectBufferInfo.offset = 0;
		reflectBufferInfo.range = sizeof(UboOffscreen);

		VkDescriptorBufferInfo reflectParametersInfo = {};
		reflectParametersInfo.buffer = uniform_buffers.reflectionParameters.buffer;
		reflectParametersInfo.offset = 0;
		reflectParametersInfo.range = sizeof(UboParam);

		std::array<VkWriteDescriptorSet, 8> reflectDescriptorWrites = objectDescriptorWrites;

		reflectDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		reflectDescriptorWrites[0].dstSet = scene_material[i].reflectDescriptorSet;
		reflectDescriptorWrites[0].pBufferInfo = &reflectBufferInfo;
		reflectDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		reflectDescriptorWrites[1].dstSet = scene_material[i].reflectDescriptorSet;
		reflectDescriptorWrites[1].pBufferInfo = &reflectParametersInfo;
		reflectDescriptorWrites[2].dstSet = scene_material[i].reflectDescriptorSet;
		reflectDescriptorWrites[3].dstSet = scene_material[i].reflectDescriptorSet;
		reflectDescriptorWrites[4].dstSet = scene_material[i].reflectDescriptorSet;
		reflectDescriptorWrites[5].dstSet = scene_material[i].reflectDescriptorSet;
		reflectDescriptorWrites[6].dstSet = scene_material[i].reflectDescriptorSet;
		reflectDescriptorWrites[7].dstSet = scene_material[i].reflectDescriptorSet;

		vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(reflectDescriptorWrites.size()), reflectDescriptorWrites.data(), 0, nullptr);

		VkDescriptorBufferInfo refractBufferInfo = {};
		refractBufferInfo.buffer = uniform_buffers.refraction.buffer;
		refractBufferInfo.offset = 0;
		refractBufferInfo.range = sizeof(UboOffscreen);

		VkDescriptorBufferInfo refractParametersInfo = {};
		refractParametersInfo.buffer = uniform_buffers.refractionParameters.buffer;
		refractParametersInfo.offset = 0;
		refractParametersInfo.range = sizeof(UboParam);

		std::array<VkWriteDescriptorSet, 8> refractDescriptorWrites = objectDescriptorWrites;

		refractDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		refractDescriptorWrites[0].dstSet = scene_material[i].refractDescriptorSet;
		refractDescriptorWrites[0].pBufferInfo = &refractBufferInfo;
		refractDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		refractDescriptorWrites[1].dstSet = scene_material[i].refractDescriptorSet;
		refractDescriptorWrites[1].pBufferInfo = &refractParametersInfo;
		refractDescriptorWrites[2].dstSet = scene_material[i].refractDescriptorSet;
		refractDescriptorWrites[3].dstSet = scene_material[i].refractDescriptorSet;
		refractDescriptorWrites[4].dstSet = scene_material[i].refractDescriptorSet;
		refractDescriptorWrites[5].dstSet = scene_material[i].refractDescriptorSet;
		refractDescriptorWrites[6].dstSet = scene_material[i].refractDescriptorSet;
		refractDescriptorWrites[7].dstSet = scene_material[i].refractDescriptorSet;

		vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(refractDescriptorWrites.size()), refractDescriptorWrites.data(), 0, nullptr);


	}

	// SkyBox descriptor set
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &skybox_descriptor_set_layout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &allocInfo, &skybox_descriptor_set));
	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &allocInfo, &skyboxReflectionDescriptorSet));
	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &allocInfo, &skyboxRefractionDescriptorSet));

	VkDescriptorBufferInfo SkyboxBufferInfo = {};
	SkyboxBufferInfo.buffer = uniform_buffers.skybox.buffer;
	SkyboxBufferInfo.offset = 0;
	SkyboxBufferInfo.range = sizeof(UboSkybox);

	VkDescriptorImageInfo ImageInfo = {};
	ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ImageInfo.imageView = sky->skybox_texture.texture_image_view;
	ImageInfo.sampler = sky->skybox_texture.texture_sampler;

	std::array<VkWriteDescriptorSet, 2> skyboxDescriptorWrites = {};

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
	skyboxDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	skyboxDescriptorWrites[1].descriptorCount = 1;
	skyboxDescriptorWrites[1].pImageInfo = &ImageInfo;

	vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(skyboxDescriptorWrites.size()), skyboxDescriptorWrites.data(), 0, nullptr);

	VkDescriptorBufferInfo skyboxReflectBufferInfo = {};
	skyboxReflectBufferInfo.buffer = uniform_buffers.skyboxReflection.buffer;
	skyboxReflectBufferInfo.offset = 0;
	skyboxReflectBufferInfo.range = sizeof(UboSkybox);

	std::array<VkWriteDescriptorSet, 2> skyboxReflectionDescriptorWrites = skyboxDescriptorWrites;

	skyboxReflectionDescriptorWrites[0].dstSet = skyboxReflectionDescriptorSet;
	skyboxReflectionDescriptorWrites[0].pBufferInfo = &skyboxReflectBufferInfo;
	skyboxReflectionDescriptorWrites[1].dstSet = skyboxReflectionDescriptorSet;
	
	vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(skyboxReflectionDescriptorWrites.size()), skyboxReflectionDescriptorWrites.data(), 0, nullptr);

	VkDescriptorBufferInfo skyboxRefractionBufferInfo = {};
	skyboxRefractionBufferInfo.buffer = uniform_buffers.skyboxRefraction.buffer;
	skyboxRefractionBufferInfo.offset = 0;
	skyboxRefractionBufferInfo.range = sizeof(UboSkybox);

	std::array<VkWriteDescriptorSet, 2> skyboxRefractionDescriptorWrites = skyboxDescriptorWrites;

	skyboxRefractionDescriptorWrites[0].dstSet = skyboxRefractionDescriptorSet;
	skyboxRefractionDescriptorWrites[0].pBufferInfo = &skyboxRefractionBufferInfo;
	skyboxRefractionDescriptorWrites[1].dstSet = skyboxRefractionDescriptorSet;
	
	vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(skyboxRefractionDescriptorWrites.size()), skyboxRefractionDescriptorWrites.data(), 0, nullptr);
	
	// Clouds descriptor set
	VkDescriptorSetAllocateInfo CloudsAllocInfo = {};
	CloudsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	CloudsAllocInfo.descriptorPool = descriptorPool;
	CloudsAllocInfo.descriptorSetCount = 1;
	CloudsAllocInfo.pSetLayouts = &clouds_descriptor_set_layout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &CloudsAllocInfo, &clouds_descriptor_set));

	VkDescriptorBufferInfo CloudsBufferInfo = {};
	CloudsBufferInfo.buffer = uniform_buffers.clouds.buffer;
	CloudsBufferInfo.offset = 0;
	CloudsBufferInfo.range = sizeof(UboClouds);

	VkDescriptorBufferInfo CloudsDynamicBufferInfo = {};
	CloudsDynamicBufferInfo.buffer = uniform_buffers.clouds_dynamic.buffer;
	CloudsDynamicBufferInfo.offset = 0;
	CloudsDynamicBufferInfo.range = sizeof(UboCloudsMatrices);

	std::array<VkWriteDescriptorSet, 2> cloudsDescriptorWrites = {};

	cloudsDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	cloudsDescriptorWrites[0].dstSet = clouds_descriptor_set;
	cloudsDescriptorWrites[0].dstBinding = 0;
	cloudsDescriptorWrites[0].dstArrayElement = 0;
	cloudsDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cloudsDescriptorWrites[0].descriptorCount = 1;
	cloudsDescriptorWrites[0].pBufferInfo = &CloudsBufferInfo;

	cloudsDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	cloudsDescriptorWrites[1].dstSet = clouds_descriptor_set;
	cloudsDescriptorWrites[1].dstBinding = 1;
	cloudsDescriptorWrites[1].dstArrayElement = 0;
	cloudsDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	cloudsDescriptorWrites[1].descriptorCount = 1;
	cloudsDescriptorWrites[1].pBufferInfo = &CloudsDynamicBufferInfo;

	vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(cloudsDescriptorWrites.size()), cloudsDescriptorWrites.data(), 0, nullptr);

	// Ocean descriptor set
	VkDescriptorSetAllocateInfo oceanAllocInfo = {};
	oceanAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	oceanAllocInfo.descriptorPool = descriptorPool;
	oceanAllocInfo.descriptorSetCount = 1;
	oceanAllocInfo.pSetLayouts = &oceanDescriptorSetLayout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &oceanAllocInfo, &oceanDescriptorSet));

	VkDescriptorBufferInfo oceanBufferInfo = {};
	oceanBufferInfo.buffer = uniform_buffers.ocean.buffer;
	oceanBufferInfo.offset = 0;
	oceanBufferInfo.range = sizeof(UboSea);

	VkDescriptorImageInfo IrradianceMapImageInfo = {};
	IrradianceMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	IrradianceMapImageInfo.imageView = sky->skybox_texture.texture_image_view;
	IrradianceMapImageInfo.sampler = sky->skybox_texture.texture_sampler;

	VkDescriptorImageInfo ReflectionImageInfo = {};
	ReflectionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ReflectionImageInfo.imageView = offscreenPass.reflectionImage.texture_image_view;
	ReflectionImageInfo.sampler = offscreenPass.reflectionImage.texture_sampler;

	VkDescriptorImageInfo RefractionImageInfo = {};
	RefractionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	RefractionImageInfo.imageView = offscreenPass.refractionImage.texture_image_view;
	RefractionImageInfo.sampler = offscreenPass.refractionImage.texture_sampler;

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

	vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(oceanDescriptorWrites.size()), oceanDescriptorWrites.data(), 0, nullptr);

	// Line descriptor set
	VkDescriptorSetAllocateInfo LineAllocInfo = {};
	LineAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	LineAllocInfo.descriptorPool = descriptorPool;
	LineAllocInfo.descriptorSetCount = 1;
	LineAllocInfo.pSetLayouts = &lineDescriptorSetLayout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &LineAllocInfo, &lineDescriptorSet));

	VkDescriptorBufferInfo LineBufferInfo = {};
	LineBufferInfo.buffer = uniform_buffers.line.buffer;
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

	vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(lineDescriptorWrites.size()), lineDescriptorWrites.data(), 0, nullptr);

	// Selection indicator descriptor set
	VkDescriptorSetAllocateInfo SelectionIndicatorAllocInfo = {};
	SelectionIndicatorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	SelectionIndicatorAllocInfo.descriptorPool = descriptorPool;
	SelectionIndicatorAllocInfo.descriptorSetCount = 1;
	SelectionIndicatorAllocInfo.pSetLayouts = &selectionIndicatorDescriptorSetLayout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &SelectionIndicatorAllocInfo, &selectionIndicatorDescriptorSet));

	VkDescriptorBufferInfo SelectionIndicatorBufferInfo = {};
	SelectionIndicatorBufferInfo.buffer = uniform_buffers.selectionIndicator.buffer;
	SelectionIndicatorBufferInfo.offset = 0;
	SelectionIndicatorBufferInfo.range = sizeof(UboSelectionIndicator);
	
	std::array<VkWriteDescriptorSet, 1> selectionIndicatorDescriptorWrites = {};

	selectionIndicatorDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	selectionIndicatorDescriptorWrites[0].dstSet = selectionIndicatorDescriptorSet;
	selectionIndicatorDescriptorWrites[0].dstBinding = 0;
	selectionIndicatorDescriptorWrites[0].dstArrayElement = 0;
	selectionIndicatorDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	selectionIndicatorDescriptorWrites[0].descriptorCount = 1;
	selectionIndicatorDescriptorWrites[0].pBufferInfo = &SelectionIndicatorBufferInfo;

	vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(selectionIndicatorDescriptorWrites.size()), selectionIndicatorDescriptorWrites.data(), 0, nullptr);
}

// ------------- Populate scene --------------------- //

	// layout of struct VertexLayout {
	//glm::vec3 pos, glm::vec3 color,	glm::vec2 text_coord, glm::vec3 normals;

void Scene::UpdateSelectRayDrawData() {
	rayVertices = {
		{ {mousePicker->GetRayOrigin().x, mousePicker->GetRayOrigin().y-1.0f, mousePicker->GetRayOrigin().z}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{ mousePicker->hitPoint, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}
	};

	enginetool::VertexLayout* vtxDst = (enginetool::VertexLayout*)vertex_buffers.selectRay.mapped;
	memcpy(vtxDst, rayVertices.data(), 2 * sizeof(enginetool::VertexLayout));
	vertex_buffers.selectRay.Flush();
	
	uint32_t* idxDst = (uint32_t*)index_buffers.selectRay.mapped;
	memcpy(idxDst, rayIndices.data(), 2 * sizeof(uint32_t));
	index_buffers.selectRay.Flush();
}

void Scene::CreateSelectRay() {
	rayVertices = {
		{ mousePicker->hitPoint, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{ {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}
	};

	rayIndices = {0,1};

	CreateMappedVertexBuffer(rayVertices, vertex_buffers.selectRay);
	CreateMappedIndexBuffer(rayIndices, index_buffers.selectRay);
}

void Scene::CreateSkybox() noexcept {
	skybox_vertices = {
		{{-horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.667f}, {0.0f, 1.0f, 0.0f}},
		{{horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.667f}, {0.0f, 1.0f, 0.0f}},
		{{horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{{horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{{-horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{{-horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.66f}, {0.0f, 1.0f, 0.0f}},
		
		{{-horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.33f}, {0.0f, -1.0f, 0.0f}},
		{{-horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.0f}, {0.0f, -1.0f, 0.0f}},
		{{horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.0f}, {0.0f, -1.0f, 0.0f}},
		{{horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.0f}, {0.0f, -1.0f, 0.0f}},
		{{horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.33f}, {0.0f, -1.0f, 0.0f}},
		{{-horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.33f}, {0.0f, -1.0f, 0.0f}},
		
		{{-horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.667f}, {0.0f, 0.0f, -1.0f}},
		{{-horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.33f}, {0.0f, 0.0f, -1.0f}},
		{{horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.33f}, {0.0f, 0.0f, -1.0f}},
		{{horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.33f}, {0.0f, 0.0f, -1.0f}},
		{{horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.667f}, {0.0f, 0.0f, -1.0f}},
		{{-horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.667f}, {0.0f, 0.0f, -1.0f}},
		
		{{horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.667f}, {-1.0f, 0.0f, 0.0f}},
		{{horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.33f}, {-1.0f, 0.0f, 0.0f}},
		{{horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.33f}, {-1.0f, 0.0f, 0.0f}},
		{{horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.33f}, {-1.0f, 0.0f, 0.0f}},
		{{horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.667f}, {-1.0f, 0.0f, 0.0f}},
		{{horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.66f}, {-1.0f, 0.0f, 0.0f}},
		
		{{horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.66f}, {0.0f, 0.0f, 1.0f}},
		{{horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.33f}, {0.0f, 0.0f, 1.0f}},
		{{-horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.33f}, {0.0f, 0.0f, 1.0f}},
		{{-horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.33f}, {0.0f, 0.0f, 1.0f}},
		{{-horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.667f}, {0.0f, 0.0f, 1.0f}},
		{{horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.667f}, {0.0f, 0.0f, 1.0f}},
		
		{{-horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.66f}, {1.0f, 0.0f, 0.0f}},
		{{-horizon, horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.33f}, {1.0f, 0.0f, 0.0f}},
		{{-horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.33f}, {1.0f, 0.0f, 0.0f}},
		{{-horizon, horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.33f}, {1.0f, 0.0f, 0.0f}},
		{{-horizon, -horizon, horizon}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.667f}, {1.0f, 0.0f, 0.0f}},
		{{-horizon, -horizon, -horizon}, {1.0f, 1.0f, 1.0f}, {0.75f, 0.667f}, {1.0f, 0.0f, 0.0f}}
	};

	skybox_indices = {
		0,1,2,3,4,5,
		6,7,8,9,10,11,
		12,13,14,15,16,17,
		18,19,20,21,22,23,
		24,25,26,27,28,29,
		30,31,32,33,34,35
	};

	CreateVertexBuffer(skybox_vertices, vertex_buffers.skybox);
	CreateIndexBuffer(skybox_indices, index_buffers.skybox);
}

void Scene::CreateAABBBuffers() {
	CreateMappedVertexBuffer(aabbVertices, vertex_buffers.aabb);
	CreateIndexBuffer(aabbIndices, index_buffers.aabb);
}

void Scene::UpdateAABBDrawData() {
	enginetool::VertexLayout* vtxDst = (enginetool::VertexLayout*)vertex_buffers.aabb.mapped;

	for (const auto& k : actors) {
		aabbVertices = {
			{{k->currentAabb.max.x, k->currentAabb.max.y, k->currentAabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
			{{k->currentAabb.min.x, k->currentAabb.max.y, k->currentAabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
			{{k->currentAabb.min.x, k->currentAabb.min.y, k->currentAabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
			{{k->currentAabb.max.x, k->currentAabb.min.y, k->currentAabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
			{{k->currentAabb.max.x, k->currentAabb.min.y, k->currentAabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
			{{k->currentAabb.max.x, k->currentAabb.max.y, k->currentAabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
			{{k->currentAabb.min.x, k->currentAabb.max.y, k->currentAabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
			{{k->currentAabb.min.x, k->currentAabb.min.y, k->currentAabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}
		};

		memcpy(vtxDst, aabbVertices.data(), aabbVertices.size() * sizeof(enginetool::VertexLayout));
		vtxDst += 8;
	}

	vertex_buffers.aabb.Flush();
}	

void Scene::GetAABBDrawData(const enginetool::ScenePart& mesh) noexcept {

	// aabbVertices = {
	// 	{{mesh.aabb.max.x, mesh.aabb.max.y, mesh.aabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	// 	{{mesh.aabb.min.x, mesh.aabb.max.y, mesh.aabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	// 	{{mesh.aabb.min.x, mesh.aabb.min.y, mesh.aabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	// 	{{mesh.aabb.max.x, mesh.aabb.min.y, mesh.aabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	// 	{{mesh.aabb.max.x, mesh.aabb.min.y, mesh.aabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	// 	{{mesh.aabb.max.x, mesh.aabb.max.y, mesh.aabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	// 	{{mesh.aabb.min.x, mesh.aabb.max.y, mesh.aabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	// 	{{mesh.aabb.min.x, mesh.aabb.min.y, mesh.aabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}
	// };


	aabbVertices.push_back({{mesh.aabb.max.x, mesh.aabb.max.y, mesh.aabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}});
	aabbVertices.push_back({{mesh.aabb.min.x, mesh.aabb.max.y, mesh.aabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}});
	aabbVertices.push_back({{mesh.aabb.min.x, mesh.aabb.min.y, mesh.aabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}});
	aabbVertices.push_back({{mesh.aabb.max.x, mesh.aabb.min.y, mesh.aabb.max.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}});
	aabbVertices.push_back({{mesh.aabb.max.x, mesh.aabb.min.y, mesh.aabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}});
	aabbVertices.push_back({{mesh.aabb.max.x, mesh.aabb.max.y, mesh.aabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}});
	aabbVertices.push_back({{mesh.aabb.min.x, mesh.aabb.max.y, mesh.aabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}});	
	aabbVertices.push_back({{mesh.aabb.min.x, mesh.aabb.min.y, mesh.aabb.min.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}});

	aabbIndices = {0,1,1,2,2,3,3,0,4,7,7,6,6,5,5,4,0,5,1,6,2,7,3,4};
}

void Scene::CreateOcean() noexcept {
	int multipler = 200;
	int vSize = multipler;//density of grid
	float offset = multipler / 2.0f; 
	float scale = multipler / (float)vSize;

	oceanVertices.resize(6*(vSize-1)*(vSize-1));

        // create local vertices

        for (uint z = 0; z < vSize; ++z) {
            for (uint x = 0; x < vSize; ++x) {
                uint index = z * vSize + x;

				oceanVertices[index].pos.x = scale*(float)x - offset;
				oceanVertices[index].pos.y =  0.0f;
				oceanVertices[index].pos.z = scale*(float)z - offset;

                oceanVertices[index].color.x = 1.0f;
				oceanVertices[index].color.y = 1.0f;
				oceanVertices[index].color.z = 1.0f;

				oceanVertices[index].text_coord.x = (float)x - offset;
				oceanVertices[index].text_coord.y = (float)z - offset;

				oceanVertices[index].normals.x = 0.0f;
				oceanVertices[index].normals.y = 1.0f;
				oceanVertices[index].normals.z = 0.0f;
            }
        }
     
		std::cout << "Ocean vertices size: " << oceanVertices.size() << std::endl;

		for(int z = 0; z < vSize-1; ++z) {
			for(int x = 0; x < vSize-1; ++x) {
				int topLeft = (z*vSize)+x;
				int topRight = topLeft + 1;
				int bottomLeft = ((z+1)*vSize)+x;
				int bottomRight = bottomLeft + 1;
				oceanIndices.emplace_back(topLeft);
				oceanIndices.emplace_back(bottomLeft);
				oceanIndices.emplace_back(topRight);
				oceanIndices.emplace_back(topRight);
				oceanIndices.emplace_back(bottomLeft);
				oceanIndices.emplace_back(bottomRight);
			}
		}

	CreateVertexBuffer(oceanVertices, vertex_buffers.ocean);
	CreateIndexBuffer(oceanIndices, index_buffers.ocean);
}

void Scene::CreateSelectionIndicator() {
	LoadFromFile("puffinEngine/assets/models/selectionCoinSmallB.obj", selectionIndicatorMesh, selectIndicatorIndices, selectIndicatorVertices);
	selectionIndicatorMesh.GetAABB(selectIndicatorVertices);
	CreateVertexBuffer(selectIndicatorVertices, vertex_buffers.selectionIndicator);
	CreateIndexBuffer(selectIndicatorIndices, index_buffers.selectionIndicator);
}

void Scene::LoadAssets() {
	InitMaterials();
	CreateSelectRay();
	CreateSkybox();
	CreateOcean();
	CreateSelectionIndicator();

	// Clouds
	std::string cloud_filename = { "puffinEngine/assets/models/sphere.obj" };
	LoadFromFile(cloud_filename, cloud_mesh, clouds_indices, clouds_vertices);
	CreateVertexBuffer(clouds_vertices, vertex_buffers.clouds);
	CreateIndexBuffer(clouds_indices, index_buffers.clouds);
	
	// Scene objects/actors
	CreateLandscape("Test object sphere", "I am simple sphere, hello!", glm::vec3(7.0f, 6.0f, 10.0f),"puffinEngine/assets/models/sphere.obj");
	CreateCamera();
	CreateCharacter();
	CreateSphereLight();
	CreateLandscape("Test object plane", "I am simple plane, boring", glm::vec3(10.0f, -16.0f, 2.0f),"puffinEngine/assets/models/planeHorizontal1000x1000x1000originMid.obj");
	CreateLandscape("Test object sphere", "I am simple sphere, watch me", glm::vec3(5.0f, 7.0f, 12.0f),"puffinEngine/assets/models/box100x100x100originMId.obj");
	
	std::dynamic_pointer_cast<Camera>(sceneCameras[0])->Init(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 60.0f, 0.001f, 200000.0f, 3.14f, 0.0f);
	std::dynamic_pointer_cast<Character>(actors[1])->Init(1000, 1000, 1000);

	for (uint32_t i = 0; i < actors.size(); i++) {
		LoadFromFile(actors[i]->mesh.meshFilename, actors[i]->mesh, objects_indices, objectsVertices);
		actors[i]->mesh.GetAABB(objectsVertices);
		GetAABBDrawData(actors[i]->mesh);
	}
	CreateVertexBuffer(objectsVertices, vertex_buffers.objects);
	CreateIndexBuffer(objects_indices, index_buffers.objects);
	CreateAABBBuffers();

	// assign shaders to meshes
	sceneCameras[0]->mesh.assigned_material = &scene_material[4]; //camera

	actors[0]->mesh.assigned_material = &scene_material[2]; //green plastic sphere
	actors[1]->mesh.assigned_material = &scene_material[5]; //character
	actors[2]->mesh.assigned_material = &scene_material[3]; //lightbulb
	actors[3]->mesh.assigned_material = &scene_material[0]; //rusty plane
	actors[4]->mesh.assigned_material = &scene_material[1]; //chrome sphere  

	currentCamera = std::dynamic_pointer_cast<Camera>(sceneCameras[0]);
	mousePicker->UpdateMousePicker(UBOSG.view, UBOSG.proj, currentCamera);	
}

void Scene::CreateCamera() {
	std::shared_ptr<Actor> camera = std::make_shared<Camera>("Test Camera", "Temporary object created for testing purpose", glm::vec3(3.0f, 4.0f, 3.0f), ActorType::Camera);
	camera->mesh.meshFilename = "puffinEngine/assets/models/cloud.obj";
	sceneCameras.emplace_back(std::move(camera));
}

void Scene::CreateCharacter() {
	std::shared_ptr<Actor> character = std::make_shared<Character>("Test Character", "Temporary object created for testing purpose", glm::vec3(4.0f, 10.0f, 4.0f), ActorType::Character);
	std::dynamic_pointer_cast<Character>(character)->Init(1000, 1000, 100);
	character->mesh.meshFilename = "puffinEngine/assets/models/box180x500x500originMidBot.obj";
	actors.emplace_back(std::move(character));
}

void Scene::CreateSphereLight() {
	std::shared_ptr<Actor> light = std::make_shared<SphereLight>("Test Light", "Sphere light", glm::vec3(0.0f, 6.0f, 5.0f), ActorType::SphereLight);
	light->mesh.meshFilename = "puffinEngine/assets/models/sphere.obj";
	std::dynamic_pointer_cast<SphereLight>(light)->SetLightColor(glm::vec3(255.0f, 197.0f, 143.0f));  //2600K 100W
	actors.emplace_back(std::move(light));
}

void Scene::CreateLandscape(std::string name, std::string description, glm::vec3 position, std::string meshFilename) {
	std::shared_ptr<Actor> stillObject = std::make_shared<Landscape>(name, description, position, ActorType::Landscape);
	std::dynamic_pointer_cast<Landscape>(stillObject)->Init(1000, 1000);
	stillObject->mesh.meshFilename = meshFilename;
	actors.emplace_back(std::move(stillObject));
}

// Use proxy design pattern!
void Scene::LoadFromFile(const std::string &filename, enginetool::ScenePart& mesh, std::vector<uint32_t>& indices, std::vector<enginetool::VertexLayout>& vertices) {
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

void Scene::CreateVertexBuffer(std::vector<enginetool::VertexLayout>& vertices, enginetool::Buffer& vertexBuffer) {
	VkDeviceSize vertexBufferSize = sizeof(enginetool::VertexLayout) * vertices.size();
	enginetool::Buffer vertexStagingBuffer;
	logicalDevice->CreateStagedBuffer(static_cast<uint32_t>(vertexBufferSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexStagingBuffer, vertices.data());
	logicalDevice->CreateUnstagedBuffer(static_cast<uint32_t>(vertexBufferSize), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer);
	CopyBuffer(vertexStagingBuffer.buffer, vertexBuffer.buffer, vertexBufferSize);
	vertexStagingBuffer.Destroy();
}

void Scene::CreateIndexBuffer(std::vector<uint32_t>& indices, enginetool::Buffer& indexBuffer) {
	VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
	enginetool::Buffer indexStagingBuffer;
	logicalDevice->CreateStagedBuffer(static_cast<uint32_t>(indexBufferSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &indexStagingBuffer, indices.data());
	logicalDevice->CreateUnstagedBuffer(static_cast<uint32_t>(indexBufferSize), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer);
	CopyBuffer(indexStagingBuffer.buffer, indexBuffer.buffer, indexBufferSize);
	indexStagingBuffer.Destroy();
}

void Scene::CreateMappedVertexBuffer(std::vector<enginetool::VertexLayout>& vertices, enginetool::Buffer& vertexBuffer) {
	VkDeviceSize vertexBufferSize = sizeof(enginetool::VertexLayout) * vertices.size();
	logicalDevice->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBuffer.buffer, vertexBuffer.memory);
	vertexBuffer.device = logicalDevice->device;
	vertexBuffer.Unmap();
	vertexBuffer.Map();
}

void Scene::CreateMappedIndexBuffer(std::vector<uint32_t>& indices, enginetool::Buffer& indexBuffer){
	VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
	logicalDevice->CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indexBuffer.buffer, indexBuffer.memory);
	indexBuffer.device = logicalDevice->device;
	indexBuffer.Unmap();
	indexBuffer.Map();
};


void Scene::InitMaterials() {
	sky->name = "Sky_materal";
	LoadSkyboxTexture(sky->skybox_texture);
	sky->assigned_pipeline = &skyboxPipeline;
	
	rust->name = "Rust";
	LoadTexture("puffinEngine/assets/textures/rustAlbedo.jpg", rust->albedo);
	LoadTexture("puffinEngine/assets/textures/rustMetallic.jpg", rust->metallic);
	LoadTexture("puffinEngine/assets/textures/rustRoughness.jpg", rust->roughness);
	LoadTexture("puffinEngine/assets/textures/rustNormal.jpg", rust->normal_map);
	LoadTexture("puffinEngine/assets/textures/rustAo.jpg", rust->ambient_occlucion_map);
	rust->assigned_pipeline = &pbrPipeline;
	scene_material.emplace_back(*rust);

	chrome->name = "Chrome";
	LoadTexture("puffinEngine/assets/textures/chromeAlbedo.jpg", chrome->albedo);
	LoadTexture("puffinEngine/assets/textures/chromeMetallic.jpg", chrome->metallic);
	LoadTexture("puffinEngine/assets/textures/chromeRoughness.jpg", chrome->roughness);
	LoadTexture("puffinEngine/assets/textures/chromeNormal.jpg", chrome->normal_map);
	LoadTexture("puffinEngine/assets/textures/chromeAo.jpg", chrome->ambient_occlucion_map);
	chrome->assigned_pipeline = &pbrPipeline;
	scene_material.emplace_back(*chrome);

	plastic->name = "Plastic";
	LoadTexture("puffinEngine/assets/textures/plasticAlbedo.jpg", plastic->albedo);
	LoadTexture("puffinEngine/assets/textures/plasticMetallic.jpg", plastic->metallic);
	LoadTexture("puffinEngine/assets/textures/plasticRoughness.jpg", plastic->roughness);
	LoadTexture("puffinEngine/assets/textures/plasticNormal.jpg", plastic->normal_map);
	LoadTexture("puffinEngine/assets/textures/plasticAo.jpg", plastic->ambient_occlucion_map);
	plastic->assigned_pipeline = &pbrPipeline;
	scene_material.emplace_back(*plastic);

	lightbulbMat->name = "Lightbulb material";
	LoadTexture("puffinEngine/assets/textures/icons/lightbulbIcon.jpg", lightbulbMat->albedo);
	LoadTexture("puffinEngine/assets/textures/metallic.jpg", lightbulbMat->metallic);
	LoadTexture("puffinEngine/assets/textures/roughness.jpg", lightbulbMat->roughness);
	LoadTexture("puffinEngine/assets/textures/normal.jpg", lightbulbMat->normal_map);
	LoadTexture("puffinEngine/assets/textures/ao.jpg", lightbulbMat->ambient_occlucion_map);
	lightbulbMat->assigned_pipeline = &pbrPipeline;
	scene_material.emplace_back(*lightbulbMat);

	cameraMat->name = "Camera material";
	LoadTexture("puffinEngine/assets/textures/icons/cameraIcon.jpg", cameraMat->albedo);
	LoadTexture("puffinEngine/assets/textures/metallic.jpg", cameraMat->metallic);
	LoadTexture("puffinEngine/assets/textures/roughness.jpg", cameraMat->roughness);
	LoadTexture("puffinEngine/assets/textures/normal.jpg", cameraMat->normal_map);
	LoadTexture("puffinEngine/assets/textures/ao.jpg", cameraMat->ambient_occlucion_map);
	cameraMat->assigned_pipeline = &pbrPipeline;
	scene_material.emplace_back(*cameraMat);

	characterMat->name = "Character material";
	LoadTexture("puffinEngine/assets/textures/icons/characterIcon.jpg", characterMat->albedo);
	LoadTexture("puffinEngine/assets/textures/metallic.jpg", characterMat->metallic);
	LoadTexture("puffinEngine/assets/textures/roughness.jpg", characterMat->roughness);
	LoadTexture("puffinEngine/assets/textures/normal.jpg", characterMat->normal_map);
	LoadTexture("puffinEngine/assets/textures/ao.jpg", characterMat->ambient_occlucion_map);
	characterMat->assigned_pipeline = &pbrPipeline;
	scene_material.emplace_back(*characterMat);
}

// ------------------ Textures ---------------------- //

void Scene::LoadSkyboxTexture(TextureLayout& layer) {
	std::string texture = "puffinEngine/assets/skybox/car_cubemap.ktx";
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	
	gli::texture_cube texCube(gli::load(texture));
	layer.texWidth = texCube.extent().x;
	layer.texHeight = texCube.extent().y;
	size_t mipLevels = texCube.levels();
	
	VkImageCreateInfo ImageInfo = {};
	ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageInfo.format = format;
	ImageInfo.extent.width = layer.texWidth;
	ImageInfo.extent.height = layer.texHeight;
	ImageInfo.extent.depth = 1;
	ImageInfo.mipLevels = static_cast<uint32_t>(mipLevels);
	ImageInfo.arrayLayers = 6;
	ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	ErrorCheck(vkCreateImage(logicalDevice->device, &ImageInfo, nullptr, &layer.texture));

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(logicalDevice->device, layer.texture, &memory_requirements); // TODO

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memory_requirements.size;
	allocInfo.memoryTypeIndex = logicalDevice->FindMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(logicalDevice->device, &allocInfo, nullptr, &layer.texture_image_memory) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to allocate image memory!");
		std::exit(-1);
	}

	ErrorCheck(vkBindImageMemory(logicalDevice->device, layer.texture, layer.texture_image_memory, 0));

	enginetool::Buffer texture_staging_buffer;
	logicalDevice->CreateStagedBuffer(texCube.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &texture_staging_buffer, texCube.data());
	
	vkGetBufferMemoryRequirements(logicalDevice->device, texture_staging_buffer.buffer, &memory_requirements);

	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	std::vector<VkBufferImageCopy> bufferCopyRegions;
	uint32_t offset = 0;

	for (uint32_t face = 0; face < 6; face++) {
		for (uint32_t level = 0; level < static_cast<uint32_t>(mipLevels); level++)	{
			VkBufferImageCopy Region = {};
			Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			Region.imageSubresource.mipLevel = level;
			Region.imageSubresource.baseArrayLayer = face;
			Region.imageSubresource.layerCount = 1;
			Region.imageExtent.width = texCube[face][level].extent().x;
			Region.imageExtent.height = texCube[face][level].extent().y;
			Region.imageExtent.depth = 1;
			Region.bufferOffset = offset;

			bufferCopyRegions.emplace_back(Region);

			offset += static_cast<uint32_t>(texCube[face][level].size());
		}
	}

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = layer.texture;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = static_cast<uint32_t>(mipLevels);
	barrier.subresourceRange.layerCount = 6;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	
	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkCmdCopyBufferToImage(command_buffer, texture_staging_buffer.buffer, layer.texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());

	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = layer.texture;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = static_cast<uint32_t>(mipLevels);
	barrier.subresourceRange.layerCount = 6;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	EndSingleTimeCommands(command_buffer, commandPool);

	texture_staging_buffer.Destroy();

	// Create sampler
	VkSamplerCreateInfo SamplerInfo = {};
	SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	SamplerInfo.magFilter = VK_FILTER_LINEAR;
	SamplerInfo.minFilter = VK_FILTER_LINEAR;
	SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	SamplerInfo.mipLodBias = 0.0f;
	SamplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	SamplerInfo.minLod = 0.0f;
	SamplerInfo.maxLod = static_cast<float>(mipLevels);
	SamplerInfo.anisotropyEnable = VK_TRUE;
	SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	SamplerInfo.maxAnisotropy = 2.0f;
	SamplerInfo.unnormalizedCoordinates = VK_FALSE;
	SamplerInfo.compareEnable = VK_FALSE;

	if (vkCreateSampler(logicalDevice->device, &SamplerInfo, nullptr, &layer.texture_sampler) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to create texture sampler!");
		std::exit(-1);
	}
	
	// Create image view
	VkImageViewCreateInfo ViewInfo = {};
	ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	ViewInfo.format = format;
	ViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	ViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	ViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	ViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	ViewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	ViewInfo.subresourceRange.layerCount = 6;
	ViewInfo.subresourceRange.levelCount = static_cast<uint32_t>(mipLevels);
	ViewInfo.image = layer.texture;

	if (vkCreateImageView(logicalDevice->device, &ViewInfo, nullptr, &layer.texture_image_view) != VK_SUCCESS)	{
		assert(0 && "Vulkan ERROR: failed to create texture image view!");
		std::exit(-1);
	}
}

void Scene::LoadTexture(std::string texture, TextureLayout& layer) {
	stbi_uc* pixels = stbi_load(texture.c_str(), (int*)&layer.texWidth, (int*)&layer.texHeight, &layer.texChannels, STBI_rgb_alpha);

	if (!pixels) {
		assert(0 && "Vulkan ERROR: failed to load texture image!");
		std::exit(-1);
	}

	VkDeviceSize image_size = layer.texWidth * layer.texHeight * 4; //  pixels are laid out row by row with 4 bytes per pixel
	enginetool::Buffer image_staging_buffer;
	logicalDevice->CreateStagedBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &image_staging_buffer, pixels);

	stbi_image_free(pixels);
	
	layer.Init(logicalDevice, VK_FORMAT_R8G8B8A8_UNORM);
	layer.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	layer.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	layer.CopyBufferToImage(image_staging_buffer.buffer);
	layer.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	image_staging_buffer.Destroy();

	layer.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);
	layer.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

// --------------- Depth buffering ------------------ //

void Scene::CreateDepthResources() {
	depthImage.Init(logicalDevice, logicalDevice->FindDepthFormat());
	depthImage.texWidth = logicalDevice->swapchain_extent.width; 
	depthImage.texHeight = logicalDevice->swapchain_extent.height; 
	depthImage.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	depthImage.CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
	depthImage.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void Scene::PrepareOffscreen() {
	offscreenPass.reflectionImage.Init(logicalDevice, VK_FORMAT_R8G8B8A8_UNORM);
	offscreenPass.reflectionImage.texWidth = (int32_t)logicalDevice->swapchain_extent.width;
	offscreenPass.reflectionImage.texHeight = (int32_t)logicalDevice->swapchain_extent.height;
	offscreenPass.reflectionImage.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	offscreenPass.reflectionImage.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);
	offscreenPass.reflectionImage.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	offscreenPass.reflectionDepthImage.Init(logicalDevice, logicalDevice->FindDepthFormat());
	offscreenPass.reflectionDepthImage.texWidth = (int32_t)logicalDevice->swapchain_extent.width; 
	offscreenPass.reflectionDepthImage.texHeight = (int32_t)logicalDevice->swapchain_extent.height; 
	offscreenPass.reflectionDepthImage.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	offscreenPass.reflectionDepthImage.CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT);

	offscreenPass.refractionImage.Init(logicalDevice, VK_FORMAT_R8G8B8A8_UNORM);
	offscreenPass.refractionImage.texWidth = (int32_t)logicalDevice->swapchain_extent.width;
	offscreenPass.refractionImage.texHeight = (int32_t)logicalDevice->swapchain_extent.height;
	offscreenPass.refractionImage.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	offscreenPass.refractionImage.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);
	offscreenPass.refractionImage.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	offscreenPass.refractionDepthImage.Init(logicalDevice, logicalDevice->FindDepthFormat());
	offscreenPass.refractionDepthImage.texWidth = (int32_t)logicalDevice->swapchain_extent.width; 
	offscreenPass.refractionDepthImage.texHeight = (int32_t)logicalDevice->swapchain_extent.height; 
	offscreenPass.refractionDepthImage.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	offscreenPass.refractionDepthImage.CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
}


// ------------------- NAVIGATION ------------------- //

void Scene::ConnectGamepad()
{
	//double seconds = 0.0;
	//seconds = glfwGetTime();
	//std::cout << "Time passed since init: " << seconds << std::endl;

	int present = glfwJoystickPresent(GLFW_JOYSTICK_1);
	/*std::cout << "Joystick/Gamepad 1 status: " << present << std::endl;*/

	if (1 == present)
	{
		int axes_count;
		const float *axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
		//std::cout << "Joystick/Gamepad 1 axes avaliable: " << axes_count << std::endl;
		currentCamera->Truck(-axes[0] * 15.0f);
		currentCamera->Dolly(axes[1] * 15.0f);

		currentCamera->GamepadMove(-axes[2], axes[3], WIDTH, HEIGHT, 0.15f);

		/*std::cout << "Left Stick X Axis: " << axes[0] << std::endl;
		std::cout << "Left Stick Y Axis: " << axes[1] << std::endl;
		std::cout << "Right Stick X Axis: " << axes[2] << std::endl;
		std::cout << "Right Stick Y Axis: " << axes[3] << std::endl;
		std::cout << "Left Trigger/L2: " << axes[4] << std::endl;
		std::cout << "Right Trigger/R2: " << axes[5] << std::endl;*/

		int button_count;
		const unsigned char *buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &button_count);
		//std::cout << "Joystick/Gamepad 1 buttons avaliable: " << button_count << std::endl;


		if (GLFW_PRESS == buttons[0])
		{
			//std::cout << "X button pressed: "<< std::endl;
			//break;
		}
		else if (GLFW_RELEASE == buttons[0])
		{
			//std::cout << "X button released: "<< std::endl;
		}
		if (GLFW_PRESS == buttons[4])
		{
			std::cout << "Left bumper pressed: reset camera" << std::endl;
			currentCamera->ResetPosition();
			//break;
		}

		/*std::cout << "A button: " << buttons[0] << std::endl;
		std::cout << "B button: " << buttons[1] << std::endl;
		std::cout << "X button: " << buttons[2] << std::endl;
		std::cout << "Y button: " << buttons[3] << std::endl;
		std::cout << "Left bumper " << buttons[4] << std::endl;
		std::cout << "Right bumper " << buttons[5] << std::endl;
		std::cout << "Back" << buttons[6] << std::endl;
		std::cout << "Start" << buttons[7] << std::endl;
		std::cout << "Guide?" << buttons[8] << std::endl;
		std::cout << "Disconnect?" << buttons[9] << std::endl;
		std::cout << "Up" << buttons[10] << std::endl;
		std::cout << "Right" << buttons[11] << std::endl;
		std::cout << "Down" << buttons[12] << std::endl;
		std::cout << "Left" << buttons[13] << std::endl;*/

		const char *name = glfwGetJoystickName(GLFW_JOYSTICK_1);
		//std::cout << "Your Joystick/Gamepad 1 name: " << name << std::endl;
	}
}

// add later chain of responsibility here!
void Scene::PressKey(int key)
{
	int state = glfwGetKey(window, key);

	if (state == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_X:
			displaySelectionIndicator = !displaySelectionIndicator;
			break;			
		case GLFW_KEY_V:
			displayWireframe = !displayWireframe;
			break;
		case GLFW_KEY_B:
			displayAabbBorders = !displayAabbBorders;
			break;
		case GLFW_KEY_W:
			std::cout << "Moving " << currentCamera->name << " foward " << key << std::endl;
			currentCamera->Dolly(150.0f);
			break;
		case GLFW_KEY_S:
			std::cout << "Moving " << currentCamera->name << " back " << key << std::endl;
			currentCamera->Dolly(-150.0f);
			break;
		case GLFW_KEY_E:
			std::cout << "Moving " << currentCamera->name << " up " << key << std::endl;
			currentCamera->Pedestal(150.0f);
			break;
		case GLFW_KEY_Q:
			std::cout << "Moving " << currentCamera->name << " down " << key << std::endl;
			currentCamera->Pedestal(-150.0f);
			break;
		case GLFW_KEY_D:
			std::cout << "Moving " << currentCamera->name << " right " << key << std::endl;
			currentCamera->Truck(-150.0f);
			break;
		case GLFW_KEY_A:
			std::cout << "Moving " << currentCamera->name << " left " << key << std::endl;
			currentCamera->Truck(150.0f);
			break;
		case GLFW_KEY_R:
			std::cout << "Reset " << currentCamera->name << " "<< key << std::endl;
			currentCamera->ResetPosition();
			break;
		case GLFW_KEY_T:
			if(selectedActor!=nullptr) {
				std::cout << "Reset " << selectedActor->name << " "<< key << std::endl;
				selectedActor->ResetPosition();
			}
			break;
		case GLFW_KEY_8:
			std::cout << "Moving " << actors[3]->name << " foward " << key << std::endl;
			actors[2]->Dolly(150.0f);
			break;
		case GLFW_KEY_7:
			std::cout << "Moving " << actors[3]->name << " back " << key << std::endl;
			actors[2]->Dolly(-150.0f);
			break;
		case GLFW_KEY_6:
			std::cout << "Moving " << actors[3]->name << " left " << key << std::endl;
			actors[2]->Strafe(150.0f);
			break;
		case GLFW_KEY_5:
			std::cout << "Moving " << actors[3]->name << " right " << key << std::endl;
			actors[2]->Strafe(-150.0f);
			break;
		case GLFW_KEY_0:
			std::cout << "Moving " << actors[2]->name << " up " << key << std::endl;
			actors[2]->Pedestal(150.0f);
			break;
		case GLFW_KEY_9:
			std::cout << "Moving " << actors[2]->name << " down " << key << std::endl;
			actors[2]->Pedestal(-150.0f);
			break;
		case GLFW_KEY_UP:
			if(selectedActor!=nullptr) {
			std::cout << "Moving " << selectedActor->name << " foward " << key << std::endl;
			selectedActor->onManualControl();
			selectedActor->Dolly(150.0f);
			}
			break;
		case GLFW_KEY_DOWN:
			if(selectedActor!=nullptr) {
			std::cout << "Moving " << selectedActor->name << " back " << key << std::endl;
			selectedActor->onManualControl();
			selectedActor->Dolly(-150.0f);
			}
			break;
		case GLFW_KEY_LEFT:
			if(selectedActor!=nullptr) {
			std::cout << "Moving " << selectedActor->name << " left " << key << std::endl;
			selectedActor->onManualControl();
			selectedActor->Strafe(150.0f);
			}
			break;
		case GLFW_KEY_RIGHT:
			if(selectedActor!=nullptr) {
			std::cout << "Moving " << selectedActor->name << " right " << key << std::endl;
			selectedActor->onManualControl();
			selectedActor->Strafe(-150.0f);
			}
			break;
		case GLFW_KEY_SPACE:
			std::cout << "Character makes a jump! " << key << std::endl;
			std::dynamic_pointer_cast<Character>(actors[1])->StartJump();
			break;
		case GLFW_KEY_Z:
			std::cout << "Saving to file " << key << std::endl;
			actors[1]->SaveToFile();
			break;
		case GLFW_KEY_1:
			guiMainHub->guiOverlayVisible = !guiMainHub->guiOverlayVisible;
			break;
		case GLFW_KEY_2:
			guiMainHub->ui_settings.display_stats_overlay = !guiMainHub->ui_settings.display_stats_overlay;
			break;
		case GLFW_KEY_3:
			if (guiMainHub->ui_settings.display_imgui)	{
				guiMainHub->ui_settings.display_imgui = false;
				std::cout << "Console turned off! " << key << std::endl;
			}
			else {
				guiMainHub->ui_settings.display_imgui = true;
				std::cout << "Console turned on! " << key << std::endl;
			}
			break;
		case GLFW_KEY_4:
			guiMainHub->ui_settings.display_main_ui = !guiMainHub->ui_settings.display_main_ui;
			break;
		}
	}

	if (state == GLFW_RELEASE)
	{

		switch (key)
		{
		case GLFW_KEY_V:
			std::cout << "Key released: " << key << std::endl;
			break;
		case GLFW_KEY_W:
			std::cout << "Key released: " << key << std::endl;
			currentCamera->Dolly(0.0f);
			break;
		case GLFW_KEY_E:
			std::cout << "Key released: " << key << std::endl;
			currentCamera->Pedestal(0.0f);
			break;
		case GLFW_KEY_S:
			std::cout << "Key released: " << key << std::endl;
			currentCamera->Dolly(0.0f);
			break;
		case GLFW_KEY_Q:
			std::cout << "Key released: " << key << std::endl;
			currentCamera->Pedestal(0.0f);
			break;
		case GLFW_KEY_D:
			std::cout << "Key released: " << key << std::endl;
			currentCamera->Truck(0.0f);
			break;
		case GLFW_KEY_A:
			std::cout << "Key released: " << key << std::endl;
			currentCamera->Truck(0.0f);
			break;
		case GLFW_KEY_0:
			std::cout << "Key released: " << key << std::endl;
			actors[2]->Pedestal(0.0f);
			break;
		case GLFW_KEY_9:
			std::cout << "Key released: " << key << std::endl;
			actors[2]->Pedestal(0.0f);
			break;
		case GLFW_KEY_8:
			std::cout << "Key released: " << key << std::endl;
			actors[2]->Dolly(0.0f);
			break;
		case GLFW_KEY_7:
			std::cout << "Key released: " << key << std::endl;
			actors[2]->Dolly(0.0f);
			break;
		case GLFW_KEY_6:
			std::cout << "Key released: " << key << std::endl;
			actors[2]->Strafe(0.0f);
			break;
		case GLFW_KEY_5:
			std::cout << "Key released: " << key << std::endl;
			actors[2]->Strafe(0.0f);
			break;
		case GLFW_KEY_UP:
			if(selectedActor!=nullptr) {
			std::cout << "Key released: " << key << std::endl;
			selectedActor->offManualControl();
			selectedActor->Dolly(0.0f);
			}
			break;
		case GLFW_KEY_DOWN:
			if(selectedActor!=nullptr) {
			std::cout << "Key released: " << key << std::endl;
			selectedActor->offManualControl();
			selectedActor->Dolly(0.0f);
			}
			break;
		case GLFW_KEY_LEFT:
			if(selectedActor!=nullptr) {
			std::cout << "Key released:" << key << std::endl;
			selectedActor->offManualControl();
			selectedActor->Strafe(0.0f);
			}
			break;
		case GLFW_KEY_RIGHT:
			if(selectedActor!=nullptr) {
			std::cout << "Key released: " << key << std::endl;
			selectedActor->offManualControl();
			selectedActor->Strafe(0.0f);
			}
			break;
		case GLFW_KEY_Z:
			std::cout <<  "Key released: " << key << std::endl;
			break;
		case GLFW_KEY_SPACE:
			std::cout << "Key released: " << key << std::endl;
			std::cout << "Going back ground level" << std::endl;
			std::dynamic_pointer_cast<Character>(actors[1])->EndJump();			
			break;
		}
	}

}


// ---------------- Deinitialisation ---------------- //

void Scene::DeInitFramebuffer() {
	for (size_t i = 0; i < logicalDevice->swap_chain_framebuffers.size(); i++) {
		vkDestroyFramebuffer(logicalDevice->device, logicalDevice->swap_chain_framebuffers[i], nullptr);
	}

	vkDestroyFramebuffer(logicalDevice->device, logicalDevice->reflectionFramebuffer, nullptr);
	vkDestroyFramebuffer(logicalDevice->device, logicalDevice->refractionFramebuffer, nullptr);
}

void Scene::DeInitDepthResources() {
	depthImage.DeInit();
	offscreenPass.reflectionDepthImage.DeInit();
	offscreenPass.refractionDepthImage.DeInit();
}

void Scene::DeInitIndexAndVertexBuffer() {
	index_buffers.ocean.Destroy();
	vertex_buffers.ocean.Destroy();
	index_buffers.skybox.Destroy();
	vertex_buffers.skybox.Destroy();
	index_buffers.clouds.Destroy();
	vertex_buffers.clouds.Destroy();
	index_buffers.objects.Destroy();
	vertex_buffers.objects.Destroy();
	index_buffers.selectRay.Destroy();
	vertex_buffers.selectRay.Destroy();
	index_buffers.aabb.Destroy();
	vertex_buffers.aabb.Destroy();
	index_buffers.selectionIndicator.Destroy();
	vertex_buffers.selectionIndicator.Destroy();
}

void Scene::DeInitScene() {
	DeInitIndexAndVertexBuffer();
	DeInitTextureImage();
	vkDestroyDescriptorPool(logicalDevice->device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->device, lineDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->device, descriptor_set_layout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->device, oceanDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->device, skybox_descriptor_set_layout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->device, clouds_descriptor_set_layout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->device, selectionIndicatorDescriptorSetLayout, nullptr);

	vkDestroyCommandPool(logicalDevice->device, commandPool, nullptr);
	DeInitUniformBuffer();
	
	delete rust;
	delete sky;
	delete chrome;
	delete plastic;
	delete lightbulbMat;
	delete cameraMat;
	delete characterMat;
	
	guiMainHub = nullptr;
	logicalDevice = nullptr;
	window = nullptr;

	rust = nullptr;
	sky = nullptr;
	chrome = nullptr;
	plastic = nullptr;
	lightbulbMat = nullptr;
	cameraMat = nullptr;
	characterMat = nullptr;
}

void Scene::DeInitTextureImage() {
	vkDestroyImageView(logicalDevice->device, sky->skybox_texture.texture_image_view, nullptr);
	vkDestroyImage(logicalDevice->device, sky->skybox_texture.texture, nullptr);
	vkDestroySampler(logicalDevice->device, sky->skybox_texture.texture_sampler, nullptr);
	vkFreeMemory(logicalDevice->device, sky->skybox_texture.texture_image_memory, nullptr);

	for (size_t i = 0; i < scene_material.size(); i++) {
		scene_material[i].albedo.DeInit();
		scene_material[i].metallic.DeInit();
		scene_material[i].roughness.DeInit();
		scene_material[i].normal_map.DeInit();
		scene_material[i].ambient_occlucion_map.DeInit();
	}

	offscreenPass.reflectionImage.DeInit();
	offscreenPass.refractionImage.DeInit();
}

void Scene::DeInitUniformBuffer() {
	if (uboDataDynamic.model) {
		alignedFree(uboDataDynamic.model);
	}

	uniform_buffers.line.Destroy();
	uniform_buffers.skybox.Destroy();
	uniform_buffers.skyboxReflection.Destroy();
	uniform_buffers.skyboxRefraction.Destroy();
	uniform_buffers.ocean.Destroy();
	uniform_buffers.selectionIndicator.Destroy();
	uniform_buffers.stillObjects.Destroy();
	uniform_buffers.parameters.Destroy();
	uniform_buffers.clouds.Destroy();
	uniform_buffers.clouds_dynamic.Destroy();
	uniform_buffers.reflection.Destroy();
	uniform_buffers.reflectionParameters.Destroy();
	uniform_buffers.refraction.Destroy();
	uniform_buffers.refractionParameters.Destroy();
}

void Scene::DestroyPipeline() {
	vkDestroyPipeline(logicalDevice->device, aabbPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, selectRayPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, skyboxPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, skyboxWireframePipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, oceanPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, oceanWireframePipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, pbrPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, pbrWireframePipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, cloudsPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, cloudsWireframePipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, pbrReflectionPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, pbrRefractionPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, skyboxReflectionPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, skyboxRefractionPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, selectionIndicatorPipeline, nullptr);
}

void Scene::FreeCommandBuffers() {
	vkFreeCommandBuffers(logicalDevice->device, commandPool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
	vkFreeCommandBuffers(logicalDevice->device, commandPool, 1, &reflectionCmdBuff);
	vkFreeCommandBuffers(logicalDevice->device, commandPool, 1, &refractionCmdBuff);
}