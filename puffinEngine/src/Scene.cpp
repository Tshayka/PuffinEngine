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

void Scene::InitScene(Device* device, GLFWwindow* window, GuiElement* console, StatusOverlay* statusOverlay) {
	this->console = console;
	logical_device = device;
	status_overlay = statusOverlay;
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
	CreateOffscreenCommandBuffer();
}

// --------------- Command buffers ------------------ //

/* Operations in Vulkan that we want to execute, like drawing operations, need to be submitted to a queue.
These operations first need to be recorded into a VkCommandBuffer before they can be submitted.
These command buffers are allocated from a VkCommandPool that is associated with a specific queue family.
Because the image in the framebuffer depends on which specific image the swap chain will give us,
we need to record a command buffer for each possible image and select the right one at draw time. */

void Scene::InitSwapchainImageViews() {
	logical_device->swapchain_image_views.resize(logical_device->swapchain_images.size());

	for (size_t i = 0; i < logical_device->swapchain_images.size(); i++) {
		logical_device->swapchain_image_views[i] = CreateImageView(logical_device->swapchain_images[i], logical_device->swapchain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void Scene::RecreateForSwapchain() {
	InitSwapchainImageViews();
	CreateGraphicsPipeline();
	CreateDepthResources();
	CreateFramebuffers();
	CreateCommandBuffers();
	CreateOffscreenCommandBuffer();
}

void Scene::CreateFramebuffers() {
	// Offscreen frambuffer
	std::array<VkImageView, 2> ofscreenAttachments = {offscreenPass.offscreenImage.texture_image_view, offscreenPass.offscreenDepthImage.texture_image_view};
	
	VkFramebufferCreateInfo offscreenFramebufferInfo = {};
	offscreenFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	offscreenFramebufferInfo.renderPass = logical_device->offscreenRenderPass;
	offscreenFramebufferInfo.attachmentCount = static_cast<uint32_t>(ofscreenAttachments.size());
	offscreenFramebufferInfo.pAttachments = ofscreenAttachments.data();
	offscreenFramebufferInfo.width = logical_device->swapchain_extent.width;
	offscreenFramebufferInfo.height = logical_device->swapchain_extent.height;
	offscreenFramebufferInfo.layers = 1;

	ErrorCheck(vkCreateFramebuffer(logical_device->device, &offscreenFramebufferInfo, nullptr, &logical_device->offscreenFramebuffer));

	// Screen frambuffer
	logical_device->swap_chain_framebuffers.resize(logical_device->swapchain_image_views.size());

	for (size_t i = 0; i < logical_device->swapchain_image_views.size(); i++)
	{
		std::array<VkImageView, 2> attachments = { logical_device->swapchain_image_views[i], depthImage.texture_image_view };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = logical_device->renderPass; // specify with which render pass the framebuffer needs to be compatible
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = logical_device->swapchain_extent.width;
		framebufferInfo.height = logical_device->swapchain_extent.height;
		framebufferInfo.layers = 1; // swap chain images are single images,

		ErrorCheck(vkCreateFramebuffer(logical_device->device, &framebufferInfo, nullptr, &logical_device->swap_chain_framebuffers[i]));
	}
}

void Scene::CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = logical_device->FindQueueFamilies(logical_device->gpu);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow command buffers to be rerecorded individually, optional

	ErrorCheck(vkCreateCommandPool(logical_device->device, &poolInfo, nullptr, &command_pool));
}

VkCommandBuffer Scene::BeginSingleTimeCommands() {
	VkCommandBufferAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocInfo.commandPool = command_pool;
	AllocInfo.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(logical_device->device, &AllocInfo, &command_buffer);

	VkCommandBufferBeginInfo BeginInfo = {};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &BeginInfo);

	return command_buffer;
}

void Scene::EndSingleTimeCommands(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &command_buffer;

	vkQueueSubmit(logical_device->queue, 1, &SubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(logical_device->queue);

	vkFreeCommandBuffers(logical_device->device, command_pool, 1, &command_buffer);
}

void Scene::CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copyRegion);

	EndSingleTimeCommands(command_buffer);
}

// ---------- Graphics pipeline --------------------- //

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

	std::array<VkDescriptorSetLayout, 4> layouts = { descriptor_set_layout, skybox_descriptor_set_layout, clouds_descriptor_set_layout, oceanDescriptorSetLayout };

	VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
	PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutInfo.pushConstantRangeCount = 1;
	PipelineLayoutInfo.pPushConstantRanges = &PushConstantRange;
	PipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	PipelineLayoutInfo.pSetLayouts = layouts.data();
	
	ErrorCheck(vkCreatePipelineLayout(logical_device->device, &PipelineLayoutInfo, nullptr, &pipeline_layout));

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
	PipelineInfo.layout = pipeline_layout;
	PipelineInfo.renderPass = logical_device->renderPass;
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	// Skybox reflect pipeline
	auto vertCubeMapShaderCode = enginetool::readFile("puffinEngine/shaders/skymap_shader.vert.spv"); 
	auto fragCubeMapShaderCode = enginetool::readFile("puffinEngine/shaders/skymap_shader.frag.spv"); 

	VkShaderModule vertCubeMapShaderModule = logical_device->CreateShaderModule(vertCubeMapShaderCode);
	VkShaderModule fragCubeMapShaderModule = logical_device->CreateShaderModule(fragCubeMapShaderCode);

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

	ErrorCheck(vkCreateGraphicsPipelines(logical_device->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &skybox_pipeline));

	vkDestroyShaderModule(logical_device->device, fragCubeMapShaderModule, nullptr);
	vkDestroyShaderModule(logical_device->device, vertCubeMapShaderModule, nullptr);
	
	// Models pipeline
	auto vertModelsShaderCode = enginetool::readFile("puffinEngine/shaders/pbr_shader.vert.spv");
	auto fragModelsShaderCode = enginetool::readFile("puffinEngine/shaders/pbr_shader.frag.spv");

	VkShaderModule vertModelsShaderModule = logical_device->CreateShaderModule(vertModelsShaderCode);
	VkShaderModule fragModelsShaderModule = logical_device->CreateShaderModule(fragModelsShaderCode);

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

	ErrorCheck(vkCreateGraphicsPipelines(logical_device->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &pbr_pipeline));

	// Offscreen pipeline
	Rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	PipelineInfo.renderPass = logical_device->offscreenRenderPass;
	ErrorCheck(vkCreateGraphicsPipelines(logical_device->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &offscreenPipeline));

	vkDestroyShaderModule(logical_device->device, fragModelsShaderModule, nullptr);
	vkDestroyShaderModule(logical_device->device, vertModelsShaderModule, nullptr);
	
	Rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
	PipelineInfo.renderPass = logical_device->renderPass;

	// Clouds pipeline
	auto vertCloudsShaderCode = enginetool::readFile("puffinEngine/shaders/clouds_shader.vert.spv");
	auto fragCloudsShaderCode = enginetool::readFile("puffinEngine/shaders/clouds_shader.frag.spv");

	VkShaderModule vertCloudsShaderModule = logical_device->CreateShaderModule(vertCloudsShaderCode);
	VkShaderModule fragCloudsShaderModule = logical_device->CreateShaderModule(fragCloudsShaderCode);

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

	ErrorCheck(vkCreateGraphicsPipelines(logical_device->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &clouds_pipeline));

	vkDestroyShaderModule(logical_device->device, fragCloudsShaderModule, nullptr);
	vkDestroyShaderModule(logical_device->device, vertCloudsShaderModule, nullptr);

	// Ocean pipeline
	auto vertOceanShaderCode = enginetool::readFile("puffinEngine/shaders/ocean_shader.vert.spv");
	auto fragOceanShaderCode = enginetool::readFile("puffinEngine/shaders/ocean_shader.frag.spv");

	VkShaderModule vertOceanShaderModule = logical_device->CreateShaderModule(vertOceanShaderCode);
	VkShaderModule fragOceanShaderModule = logical_device->CreateShaderModule(fragOceanShaderCode);

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

	ErrorCheck(vkCreateGraphicsPipelines(logical_device->device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &oceanPipeline));

	vkDestroyShaderModule(logical_device->device, fragOceanShaderModule, nullptr);
	vkDestroyShaderModule(logical_device->device, vertOceanShaderModule, nullptr);
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
	renderPassInfo.renderPass = logical_device->renderPass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = logical_device->swapchain_extent.width;
	renderPassInfo.renderArea.extent.height = logical_device->swapchain_extent.height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	command_buffers.resize(logical_device->swap_chain_framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = command_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if the allocated command buffers are primary or secondary, here "primary" can be submitted to a queue for execution, but cannot be called from other command buffers
	allocInfo.commandBufferCount = (uint32_t)command_buffers.size();

	ErrorCheck(vkAllocateCommandBuffers(logical_device->device, &allocInfo, command_buffers.data()));
	
	// starting command buffer recording
	for (size_t i = 0; i < command_buffers.size(); i++)	{
		// Set target frame buffer
		renderPassInfo.framebuffer = logical_device->swap_chain_framebuffers[i];
		vkBeginCommandBuffer(command_buffers[i], &beginInfo);
		vkCmdBeginRenderPass(command_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)logical_device->swapchain_extent.width;
		viewport.height = (float)logical_device->swapchain_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffers[i], 0, 1, &viewport);

		scissor.offset = { 0, 0 }; // scissor rectangle covers framebuffer entirely
		scissor.extent = logical_device->swapchain_extent;
		vkCmdSetScissor(command_buffers[i], 0, 1, &scissor);

		VkDeviceSize offsets[1] = { 0 };

		// Skybox
		if (display_skybox)	{
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &skybox_descriptor_set, 0, nullptr);
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers.skybox.buffer, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffers.skybox.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_pipeline);
			vkCmdDrawIndexed(command_buffers[i], static_cast<uint32_t>(skybox_indices.size()), 1, 0, 0, 0);
		}

		// Ocean
		if (displayOcean) {
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 3, 1, &oceanDescriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers.ocean.buffer, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffers.ocean.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, oceanPipeline);
			vkCmdDrawIndexed(command_buffers[i], static_cast<uint32_t>(oceanIndices.size()), 1, 0, 0, 0);
		}

		// 3d object
		vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers.objects.buffer, offsets);
		vkCmdBindIndexBuffer(command_buffers[i], index_buffers.objects.buffer , 0, VK_INDEX_TYPE_UINT32);

		for (size_t j = 0; j < actors.size(); j++) {
			std::array<VkDescriptorSet, 1> descriptorSets;
			descriptorSets[0] = actors[j]->mesh.assigned_material->descriptor_set;
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *actors[j]->mesh.assigned_material->assigned_pipeline);
			pushConstants.pos = actors[j]->position;
			vkCmdPushConstants(command_buffers[i], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Constants), &pushConstants);
			vkCmdDrawIndexed(command_buffers[i], actors[j]->mesh.indexCount, 1, 0, actors[j]->mesh.indexBase, 0);
		}

		// Clouds
		if (display_clouds)	{
			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, clouds_pipeline);
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers.clouds.buffer, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffers.clouds.buffer, 0, VK_INDEX_TYPE_UINT32);

			for (uint32_t k = 0; k < DYNAMIC_UB_OBJECTS; k++) {
				uint32_t dynamic_offset = k * static_cast<uint32_t>(dynamicAlignment);
				vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 2, 1, &clouds_descriptor_set, 1, &dynamic_offset);
				vkCmdDrawIndexed(command_buffers[i], static_cast<uint32_t>(clouds_indices.size()), 1, 0, 0, 0);
			}
		}
		
		vkCmdEndRenderPass(command_buffers[i]);
		ErrorCheck(vkEndCommandBuffer(command_buffers[i]));
	}
}

void Scene::CreateOffscreenCommandBuffer() { // Mirrored scene for offscreen render
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = logical_device->offscreenRenderPass;
	renderPassInfo.framebuffer = logical_device->offscreenFramebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = logical_device->swapchain_extent.width;
	renderPassInfo.renderArea.extent.height = logical_device->swapchain_extent.height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = command_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if the allocated command buffers are primary or secondary, here "primary" can be submitted to a queue for execution, but cannot be called from other command buffers
	allocInfo.commandBufferCount = 1;

	ErrorCheck(vkAllocateCommandBuffers(logical_device->device, &allocInfo, &commandBuffer));
	
	ErrorCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)logical_device->swapchain_extent.width;
	viewport.height = (float)logical_device->swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	scissor.offset = { 0, 0 }; // scissor rectangle covers framebuffer entirely
	scissor.extent.height = logical_device->swapchain_extent.height;
	scissor.extent.width = logical_device->swapchain_extent.width;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkDeviceSize offsets[1] = { 0 };

	// 3d object
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertex_buffers.objects.buffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, index_buffers.objects.buffer , 0, VK_INDEX_TYPE_UINT32);

	for (size_t j = 1; j < actors.size(); j++) {
		std::array<VkDescriptorSet, 1> descriptorSets;
		descriptorSets[0] = actors[j]->mesh.assigned_material->offscreenDescriptorSet;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipeline);
		pushConstants.pos = actors[j]->position;
		vkCmdPushConstants(commandBuffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Constants), &pushConstants);
		vkCmdDrawIndexed(commandBuffer, actors[j]->mesh.indexCount, 1, 0, actors[j]->mesh.indexBase, 0);
	}

	vkCmdEndRenderPass(commandBuffer);
	ErrorCheck(vkEndCommandBuffer(commandBuffer));
}

void Scene::CreateUniformBuffer() {
	// Skybox Uniform buffers memory -> static
	logical_device->CreateUnstagedBuffer(sizeof(UniformBufferObjectSky), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.skybox);
	uniform_buffers.skybox.Map();

	// Ocean Uniform buffers memory -> static
	logical_device->CreateUnstagedBuffer(sizeof(UboSea), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.ocean);
	uniform_buffers.ocean.Map();
	
	// Clouds Uniform buffers memory -> dynamic
	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = logical_device->gpu_properties.limits.minUniformBufferOffsetAlignment;

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
	logical_device->CreateUnstagedBuffer(sizeof(UboClouds), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.clouds);
	uniform_buffers.clouds.Map();

	// Uniform buffer object with per-object matrices
	logical_device->CreateUnstagedBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &uniform_buffers.clouds_dynamic);
	uniform_buffers.clouds_dynamic.Map();

	// Objects Uniform buffers memory -> static
	logical_device->CreateUnstagedBuffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.objects);
	uniform_buffers.objects.Map();

	// Objects Uniform buffers for offscreen rendering -> static
	logical_device->CreateUnstagedBuffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.mirrored);
	uniform_buffers.mirrored.Map();

	// Additional uniform bufer for parameters -> static
	logical_device->CreateUnstagedBuffer(sizeof(UniformBufferObjectParam), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.parameters);
	uniform_buffers.parameters.Map();

	// Additional uniform bufer parameters for mirrored scene -> static
	logical_device->CreateUnstagedBuffer(sizeof(UniformBufferObjectParam), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers.mirroredParameters);
	uniform_buffers.mirroredParameters.Map();

	// Create random positions for dynamic uniform buffer
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<double> dist(-1.0f, 1.0f);
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
	UpdateUniformBuffer(time);

	std::dynamic_pointer_cast<Camera>(actors[0])->UpdatePosition(dt); // TODO not update when camera not moving?
	std::dynamic_pointer_cast<Character>(actors[1])->UpdatePosition(dt); 
	std::dynamic_pointer_cast<SphereLight>(actors[2])->UpdatePosition(dt);
	actors[3]->UpdatePosition(dt);  
	CreateCommandBuffers();
	CreateOffscreenCommandBuffer();
}

void Scene::UpdateUniformBuffer(const float& time) {
	UniformBufferObject UBO = {};
	UBO.proj = glm::perspective(glm::radians(std::dynamic_pointer_cast<Camera>(actors[0])->FOV), (float)logical_device->swapchain_extent.width / (float)logical_device->swapchain_extent.height, std::dynamic_pointer_cast<Camera>(actors[0])->clippingNear, std::dynamic_pointer_cast<Camera>(actors[0])->clippingFar);
	UBO.proj[1][1] *= -1; //since the Y axis of Vulkan NDC points down
	UBO.view = glm::lookAt(actors[0]->position, std::dynamic_pointer_cast<Camera>(actors[0])->view, std::dynamic_pointer_cast<Camera>(actors[0])->up);
	UBO.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),std::dynamic_pointer_cast<Camera>(actors[0])->up);
	//UBO.model = glm::mat4(1.0f);
	UBO.camera_pos = glm::vec3(actors[0]->position);
	memcpy(uniform_buffers.objects.mapped, &UBO, sizeof(UBO));

	//UBO.model = glm::scale(UBO.model, glm::vec3(1.0f, -1.0f, 1.0f));
	UBO.view[1][0] *= -1;
	UBO.view[1][1] *= -1;
	UBO.view[1][2] *= -1;
	UBO.camera_pos *= glm::vec3(1.0f, -1.0f, 1.0f);
	memcpy(uniform_buffers.mirrored.mapped, &UBO, sizeof(UBO));

	UboClouds UBOC = {};
	UBOC.proj = glm::perspective(glm::radians(std::dynamic_pointer_cast<Camera>(actors[0])->FOV), (float)logical_device->swapchain_extent.width / (float)logical_device->swapchain_extent.height, std::dynamic_pointer_cast<Camera>(actors[0])->clippingNear, std::dynamic_pointer_cast<Camera>(actors[0])->clippingFar);
	UBOC.proj[1][1] *= -1; //since the Y axis of Vulkan NDC points down
	UBOC.view = glm::lookAt(actors[0]->position, std::dynamic_pointer_cast<Camera>(actors[0])->view, std::dynamic_pointer_cast<Camera>(actors[0])->up);
	UBOC.time = time;
	UBOC.camera_pos = actors[0]->position;

	memcpy(uniform_buffers.clouds.mapped, &UBOC, sizeof(UBOC));	
}

void Scene::UpdateUBOParameters() {
	UniformBufferObjectParam UBO_Param = {};
	UBO_Param.light_col = std::dynamic_pointer_cast<SphereLight>(actors[2])->GetLightColor();
	UBO_Param.exposure = 2.5f;
	UBO_Param.light_pos[0] = actors[2]->position;
		
	memcpy(uniform_buffers.parameters.mapped, &UBO_Param, sizeof(UBO_Param));

	UBO_Param.light_pos[0]*= glm::vec3(1.0f, -1.0f, 1.0f);

	memcpy(uniform_buffers.mirroredParameters.mapped, &UBO_Param, sizeof(UBO_Param));
}

void Scene::UpdateDynamicUniformBuffer(const float& time) {
	uint32_t dim = static_cast<uint32_t>(pow(DYNAMIC_UB_OBJECTS, (1.0f / 3.0f)));
	glm::vec3 offset(5.0f, 25.0f, 5.0f);

		for (uint32_t x = 0; x < dim; x++) {
			for (uint32_t y = 0; y < dim; y++) {
				for (uint32_t z = 0; z < dim; z++) {
					uint32_t index = x * dim * dim + y * dim + z;

					// Aligned offset
					glm::mat4* modelMat = (glm::mat4*)(((uint64_t)uboDataDynamic.model + (index * dynamicAlignment)));
			
					// Update matrices
					glm::vec3 pos = glm::vec3(-((dim * offset.x) / 2.0f) + offset.x / 2.0f + x * offset.x, -((dim * offset.y) / 2.0f) + offset.y / 2.0f + y * offset.y, -((dim * offset.z) / 2.0f) + offset.z / 2.0f + z * offset.z);
					
					pos += rnd_pos[index];
				
					*modelMat = glm::translate(glm::mat4(1.0f), pos);
					*modelMat = glm::translate(*modelMat, glm::vec3(0.01 + time * 0.1, 0.0f, 0.03 + time * 0.1));
					*modelMat = glm::rotate(*modelMat, time * glm::radians(90.0f), glm::vec3(1.0f, 1.0f, 0.0f));
					*modelMat = glm::rotate(*modelMat, time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					*modelMat = glm::rotate(*modelMat, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				}
			}
		}

		memcpy(uniform_buffers.clouds_dynamic.mapped, uboDataDynamic.model, uniform_buffers.clouds_dynamic.size);

		// Flush to make changes visible to the host 
		VkMappedMemoryRange memoryRange = {};
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.memory = uniform_buffers.clouds_dynamic.memory;
		memoryRange.size = uniform_buffers.clouds_dynamic.size;
		
		vkFlushMappedMemoryRanges(logical_device->device, 1, &memoryRange);
}

void Scene::UpdateSkyboxUniformBuffer() {
	UniformBufferObjectSky UBO_Sky = {};
	UBO_Sky.proj = glm::perspective(glm::radians(std::dynamic_pointer_cast<Camera>(actors[0])->FOV), (float)logical_device->swapchain_extent.width / (float)logical_device->swapchain_extent.height, std::dynamic_pointer_cast<Camera>(actors[0])->clippingNear, std::dynamic_pointer_cast<Camera>(actors[0])->clippingFar);
	UBO_Sky.proj[1][1] *= -1;
	UBO_Sky.view = glm::lookAt(std::dynamic_pointer_cast<Camera>(actors[0])->position, std::dynamic_pointer_cast<Camera>(actors[0])->view, std::dynamic_pointer_cast<Camera>(actors[0])->up);
	UBO_Sky.view[3][0] *= 0;
	UBO_Sky.view[3][1] *= 0;
	UBO_Sky.view[3][2] *= 0;
	UBO_Sky.exposure = 1.0f;
	UBO_Sky.gamma = 1.0f;
	
	memcpy(uniform_buffers.skybox.mapped, &UBO_Sky, sizeof(UBO_Sky));
}

void Scene::UpdateOceanUniformBuffer(const float& time) {
	UboSea uboOcean = {};
	uboOcean.model = glm::mat4(1.0f);
	uboOcean.proj = glm::perspective(glm::radians(std::dynamic_pointer_cast<Camera>(actors[0])->FOV), (float)logical_device->swapchain_extent.width / (float)logical_device->swapchain_extent.height, std::dynamic_pointer_cast<Camera>(actors[0])->clippingNear, std::dynamic_pointer_cast<Camera>(actors[0])->clippingFar);
	uboOcean.proj[1][1] *= -1;
	uboOcean.view = glm::lookAt(actors[0]->position, std::dynamic_pointer_cast<Camera>(actors[0])->view, std::dynamic_pointer_cast<Camera>(actors[0])->up);
	uboOcean.cameraPos = actors[0]->position;
	uboOcean.time = time;
	
	memcpy(uniform_buffers.ocean.mapped, &uboOcean, sizeof(uboOcean));
}

// ------ Text overlay - performance statistics ----- //

void Scene::UpdateGUI(float frameTimer, uint32_t elapsedTime) {
	status_overlay->UpdateCommandBuffers(frameTimer, elapsedTime);
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

	ErrorCheck(vkCreateDescriptorSetLayout(logical_device->device, &SceneObjectsLayoutInfo, nullptr, &descriptor_set_layout));

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

	ErrorCheck(vkCreateDescriptorSetLayout(logical_device->device, &SkyboxLayoutInfo, nullptr, &skybox_descriptor_set_layout));
	
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

	ErrorCheck(vkCreateDescriptorSetLayout(logical_device->device, &CloudsLayoutInfo, nullptr, &clouds_descriptor_set_layout));

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

	VkDescriptorSetLayoutBinding offscreenLayoutBinding = {};
	offscreenLayoutBinding.binding = 2;
	offscreenLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	offscreenLayoutBinding.descriptorCount = 1;
	offscreenLayoutBinding.pImmutableSamplers = nullptr;
	offscreenLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 3> oceanBindings = { oceanUBOLayoutBinding, skyboxReflectionLayoutBinding, offscreenLayoutBinding };

	VkDescriptorSetLayoutCreateInfo OceanLayoutInfo = {};
	OceanLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	OceanLayoutInfo.bindingCount = static_cast<uint32_t>(oceanBindings.size());
	OceanLayoutInfo.pBindings = oceanBindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logical_device->device, &OceanLayoutInfo, nullptr, &oceanDescriptorSetLayout));
}

void Scene::CreateDescriptorPool() {
	// Don't forget to rise this numbers when you add bindings
	std::array<VkDescriptorPoolSize, 3> PoolSizes = {};
	PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	PoolSizes[0].descriptorCount = static_cast<uint32_t>(1);
	PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	PoolSizes[1].descriptorCount = static_cast<uint32_t>(scene_material.size() * 12 + 7);
	PoolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	PoolSizes[2].descriptorCount = static_cast<uint32_t>(scene_material.size() * 4 + 5);

	VkDescriptorPoolCreateInfo PoolInfo = {};
	PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.maxSets = static_cast<uint32_t>(scene_material.size()*2 + 4); // maximum number of descriptor sets that will be allocated

	ErrorCheck(vkCreateDescriptorPool(logical_device->device, &PoolInfo, nullptr, &descriptor_pool));
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
		AllocInfo.descriptorPool = descriptor_pool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &descriptor_set_layout;

		ErrorCheck(vkAllocateDescriptorSets(logical_device->device, &AllocInfo, &scene_material[i].descriptor_set));

		ErrorCheck(vkAllocateDescriptorSets(logical_device->device, &AllocInfo, &scene_material[i].offscreenDescriptorSet));

		VkDescriptorBufferInfo BufferInfo = {};
		BufferInfo.buffer = uniform_buffers.objects.buffer;
		BufferInfo.offset = 0;
		BufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorBufferInfo ObjectBufferParametersInfo = {};
		ObjectBufferParametersInfo.buffer = uniform_buffers.parameters.buffer;
		ObjectBufferParametersInfo.offset = 0;
		ObjectBufferParametersInfo.range = sizeof(UniformBufferObjectParam);

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

		vkUpdateDescriptorSets(logical_device->device, static_cast<uint32_t>(objectDescriptorWrites.size()), objectDescriptorWrites.data(), 0, nullptr);

		VkDescriptorBufferInfo OffscreenBufferInfo = {};
		OffscreenBufferInfo.buffer = uniform_buffers.mirrored.buffer;
		OffscreenBufferInfo.offset = 0;
		OffscreenBufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorBufferInfo MirroredParametersInfo = {};
		MirroredParametersInfo.buffer = uniform_buffers.mirroredParameters.buffer;
		MirroredParametersInfo.offset = 0;
		MirroredParametersInfo.range = sizeof(UniformBufferObjectParam);

		std::array<VkWriteDescriptorSet, 8> offscreenDescriptorWrites = objectDescriptorWrites;

		offscreenDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		offscreenDescriptorWrites[0].dstSet = scene_material[i].offscreenDescriptorSet;
		offscreenDescriptorWrites[0].pBufferInfo = &OffscreenBufferInfo;
		offscreenDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		offscreenDescriptorWrites[1].dstSet = scene_material[i].offscreenDescriptorSet;
		offscreenDescriptorWrites[1].pBufferInfo = &MirroredParametersInfo;
		offscreenDescriptorWrites[2].dstSet = scene_material[i].offscreenDescriptorSet;
		offscreenDescriptorWrites[3].dstSet = scene_material[i].offscreenDescriptorSet;
		offscreenDescriptorWrites[4].dstSet = scene_material[i].offscreenDescriptorSet;
		offscreenDescriptorWrites[5].dstSet = scene_material[i].offscreenDescriptorSet;
		offscreenDescriptorWrites[6].dstSet = scene_material[i].offscreenDescriptorSet;
		offscreenDescriptorWrites[7].dstSet = scene_material[i].offscreenDescriptorSet;

		vkUpdateDescriptorSets(logical_device->device, static_cast<uint32_t>(offscreenDescriptorWrites.size()), offscreenDescriptorWrites.data(), 0, nullptr);
	}

	// SkyBox descriptor set
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptor_pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &skybox_descriptor_set_layout;

	ErrorCheck(vkAllocateDescriptorSets(logical_device->device, &allocInfo, &skybox_descriptor_set));

	VkDescriptorBufferInfo SkyboxBufferInfo = {};
	SkyboxBufferInfo.buffer = uniform_buffers.skybox.buffer;
	SkyboxBufferInfo.offset = 0;
	SkyboxBufferInfo.range = sizeof(UniformBufferObjectSky);

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

	vkUpdateDescriptorSets(logical_device->device, static_cast<uint32_t>(skyboxDescriptorWrites.size()), skyboxDescriptorWrites.data(), 0, nullptr);
	
	// Clouds descriptor set
	VkDescriptorSetAllocateInfo CloudsAllocInfo = {};
	CloudsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	CloudsAllocInfo.descriptorPool = descriptor_pool;
	CloudsAllocInfo.descriptorSetCount = 1;
	CloudsAllocInfo.pSetLayouts = &clouds_descriptor_set_layout;

	ErrorCheck(vkAllocateDescriptorSets(logical_device->device, &CloudsAllocInfo, &clouds_descriptor_set));

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

	vkUpdateDescriptorSets(logical_device->device, static_cast<uint32_t>(cloudsDescriptorWrites.size()), cloudsDescriptorWrites.data(), 0, nullptr);

	// Ocean descriptor set
	VkDescriptorSetAllocateInfo oceanAllocInfo = {};
	oceanAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	oceanAllocInfo.descriptorPool = descriptor_pool;
	oceanAllocInfo.descriptorSetCount = 1;
	oceanAllocInfo.pSetLayouts = &oceanDescriptorSetLayout;

	ErrorCheck(vkAllocateDescriptorSets(logical_device->device, &oceanAllocInfo, &oceanDescriptorSet));

	VkDescriptorBufferInfo oceanBufferInfo = {};
	oceanBufferInfo.buffer = uniform_buffers.ocean.buffer;
	oceanBufferInfo.offset = 0;
	oceanBufferInfo.range = sizeof(UboSea);

	VkDescriptorImageInfo IrradianceMapImageInfo = {};
	IrradianceMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	IrradianceMapImageInfo.imageView = sky->skybox_texture.texture_image_view;
	IrradianceMapImageInfo.sampler = sky->skybox_texture.texture_sampler;

	VkDescriptorImageInfo OffscreenImageInfo = {};
	OffscreenImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	OffscreenImageInfo.imageView = offscreenPass.offscreenImage.texture_image_view;
	OffscreenImageInfo.sampler = offscreenPass.offscreenImage.texture_sampler;

	std::array<VkWriteDescriptorSet, 3> oceanDescriptorWrites = {};

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
	oceanDescriptorWrites[2].pImageInfo = &OffscreenImageInfo;

	vkUpdateDescriptorSets(logical_device->device, static_cast<uint32_t>(oceanDescriptorWrites.size()), oceanDescriptorWrites.data(), 0, nullptr);
}

// ------------- Populate scene --------------------- //

void Scene::LoadAssets() {
	InitMaterials();

	// Skybox
	float horizon = 125.0f;
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

	CreateBuffers(skybox_indices, skybox_vertices, vertex_buffers.skybox, index_buffers.skybox);
	

	// Ocean
	int vSize = 32;
	float offset = horizon / 2.0f; 
	float scale = horizon / (float)vSize;

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
     
		std::cout << "ocean vertices size: " << " " << oceanVertices.size() << std::endl;

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

	CreateBuffers(oceanIndices, oceanVertices, vertex_buffers.ocean, index_buffers.ocean);

	// Clouds
	std::string cloud_filename = { "puffinEngine/assets/models/cloud.obj" };
	LoadFromFile(cloud_filename, cloud_mesh, clouds_indices, clouds_vertices);
	CreateBuffers(clouds_indices, clouds_vertices, vertex_buffers.clouds, index_buffers.clouds);
	

	// Scene objects/actors
	CreateCamera();
	CreateCharacter();
	CreateSphereLight();
	CreateStillObject();
	
	std::dynamic_pointer_cast<Camera>(actors[0])->Init(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 60.0f, 0.001f, 1000.0f, 3.14f, 0.0f);
	std::dynamic_pointer_cast<Character>(actors[1])->Init(1000, 1000, 1000);

	for (uint32_t i = 0; i < actors.size(); i++) {
		LoadFromFile(actors[i]->mesh.meshFilename, actors[i]->mesh, objects_indices, objects_vertices);
	}
	CreateBuffers(objects_indices, objects_vertices, vertex_buffers.objects, index_buffers.objects);

	// assign shaders to meshes
	actors[0]->mesh.assigned_material = &scene_material[4]; //camera
	actors[1]->mesh.assigned_material = &scene_material[2]; //character
	actors[2]->mesh.assigned_material = &scene_material[3]; //lightbulb
	actors[3]->mesh.assigned_material = &scene_material[0]; //rusty plane 
	
}

void Scene::CreateCamera() {
	std::shared_ptr<Actor> camera = std::make_shared<Camera>("Test Camera", "Temporary object created for testing purpose", glm::vec3(3.0f, 4.0f, 3.0f));
	camera->mesh.meshFilename = "puffinEngine/assets/models/cloud.obj";
	actors.emplace_back(std::move(camera));
}

void Scene::CreateCharacter() {
	std::shared_ptr<Actor> character = std::make_shared<Character>("Test Character", "Temporary object created for testing purpose", glm::vec3(4.0f, 10.0f, 4.0f));
	character->mesh.meshFilename = "puffinEngine/assets/models/cloud.obj";
	actors.emplace_back(std::move(character));
}

void Scene::CreateSphereLight() {
	std::shared_ptr<Actor> light = std::make_shared<SphereLight>("Test Light", "Sphere light", glm::vec3(0.0f, 6.0f, 5.0f));
	light->mesh.meshFilename = "puffinEngine/assets/models/sphere.obj";
	std::dynamic_pointer_cast<SphereLight>(light)->SetLightColor(glm::vec3(255.0f, 197.0f, 143.0f));  //2600K 100W
	actors.emplace_back(std::move(light));
}

void Scene::CreateStillObject() {
	std::shared_ptr<Actor> stillObject = std::make_shared<Actor>("Test object", "I am simple plane", glm::vec3(10.0f, -16.0f, 2.0f));
	stillObject->mesh.meshFilename = "puffinEngine/assets/models/plane.obj";
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
	mesh.boundingBox = mesh.GetAABB(vertices);
}

void Scene::CreateBuffers(std::vector<uint32_t>& indices, std::vector<enginetool::VertexLayout>& vertices, enginetool::Buffer& vertex_buffer, enginetool::Buffer& index_buffer) {
	VkDeviceSize vertex_buffer_size = sizeof(enginetool::VertexLayout) * vertices.size();
	VkDeviceSize index_buffer_size = sizeof(uint32_t) * indices.size(); // now equal to the number of indices times the size of the index type

	enginetool::Buffer vertexStagingBuffer, index_staging_buffer;

	logical_device->CreateStagedBuffer(static_cast<uint32_t>(vertex_buffer_size), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexStagingBuffer, vertices.data());
	logical_device->CreateUnstagedBuffer(static_cast<uint32_t>(vertex_buffer_size), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertex_buffer);

	CopyBuffer(vertexStagingBuffer.buffer, vertex_buffer.buffer, vertex_buffer_size);

	logical_device->CreateStagedBuffer(static_cast<uint32_t>(index_buffer_size), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &index_staging_buffer, indices.data());
	logical_device->CreateUnstagedBuffer(static_cast<uint32_t>(index_buffer_size), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &index_buffer);

	CopyBuffer(index_staging_buffer.buffer, index_buffer.buffer, index_buffer_size);

	vertexStagingBuffer.Destroy();
	index_staging_buffer.Destroy();
}

void Scene::InitMaterials() {
	if (display_skybox) {
		sky->name = "Sky_materal";
		LoadSkyboxTexture(sky->skybox_texture);
		sky->assigned_pipeline = &skybox_pipeline;
	}
	
	rust->name = "Rust";
	LoadTexture("puffinEngine/assets/textures/rustAlbedo.jpg", rust->albedo);
	LoadTexture("puffinEngine/assets/textures/rustMetallic.jpg", rust->metallic);
	LoadTexture("puffinEngine/assets/textures/rustRoughness.jpg", rust->roughness);
	LoadTexture("puffinEngine/assets/textures/rustNormal.jpg", rust->normal_map);
	LoadTexture("puffinEngine/assets/textures/rustAo.jpg", rust->ambient_occlucion_map);
	rust->assigned_pipeline = &pbr_pipeline;
	scene_material.emplace_back(*rust);

	chrome->name = "Chrome";
	LoadTexture("puffinEngine/assets/textures/chromeAlbedo.jpg", chrome->albedo);
	LoadTexture("puffinEngine/assets/textures/chromeMetallic.jpg", chrome->metallic);
	LoadTexture("puffinEngine/assets/textures/chromeRoughness.jpg", chrome->roughness);
	LoadTexture("puffinEngine/assets/textures/chromeNormal.jpg", chrome->normal_map);
	LoadTexture("puffinEngine/assets/textures/chromeAo.jpg", chrome->ambient_occlucion_map);
	chrome->assigned_pipeline = &pbr_pipeline;
	scene_material.emplace_back(*chrome);

	plastic->name = "Plastic";
	LoadTexture("puffinEngine/assets/textures/plasticAlbedo.jpg", plastic->albedo);
	LoadTexture("puffinEngine/assets/textures/plasticMetallic.jpg", plastic->metallic);
	LoadTexture("puffinEngine/assets/textures/plasticRoughness.jpg", plastic->roughness);
	LoadTexture("puffinEngine/assets/textures/plasticNormal.jpg", plastic->normal_map);
	LoadTexture("puffinEngine/assets/textures/plasticAo.jpg", plastic->ambient_occlucion_map);
	plastic->assigned_pipeline = &pbr_pipeline;
	scene_material.emplace_back(*plastic);

	lightbulbMat->name = "Lightbulb material";
	LoadTexture("puffinEngine/assets/textures/icons/lightbulbIcon.jpg", lightbulbMat->albedo);
	LoadTexture("puffinEngine/assets/textures/metallic.jpg", lightbulbMat->metallic);
	LoadTexture("puffinEngine/assets/textures/roughness.jpg", lightbulbMat->roughness);
	LoadTexture("puffinEngine/assets/textures/normal.jpg", lightbulbMat->normal_map);
	LoadTexture("puffinEngine/assets/textures/ao.jpg", lightbulbMat->ambient_occlucion_map);
	lightbulbMat->assigned_pipeline = &pbr_pipeline;
	scene_material.emplace_back(*lightbulbMat);

	cameraMat->name = "Camera material";
	LoadTexture("puffinEngine/assets/textures/icons/cameraIcon.jpg", cameraMat->albedo);
	LoadTexture("puffinEngine/assets/textures/metallic.jpg", cameraMat->metallic);
	LoadTexture("puffinEngine/assets/textures/roughness.jpg", cameraMat->roughness);
	LoadTexture("puffinEngine/assets/textures/normal.jpg", cameraMat->normal_map);
	LoadTexture("puffinEngine/assets/textures/ao.jpg", cameraMat->ambient_occlucion_map);
	cameraMat->assigned_pipeline = &pbr_pipeline;
	scene_material.emplace_back(*cameraMat);

	characterMat->name = "Character material";
	LoadTexture("puffinEngine/assets/textures/icons/characterIcon.jpg", characterMat->albedo);
	LoadTexture("puffinEngine/assets/textures/metallic.jpg", characterMat->metallic);
	LoadTexture("puffinEngine/assets/textures/roughness.jpg", characterMat->roughness);
	LoadTexture("puffinEngine/assets/textures/normal.jpg", characterMat->normal_map);
	LoadTexture("puffinEngine/assets/textures/ao.jpg", characterMat->ambient_occlucion_map);
	characterMat->assigned_pipeline = &pbr_pipeline;
	scene_material.emplace_back(*characterMat);
}

void Scene::PrepareOffscreen() {
	offscreenPass.offscreenImage.texWidth = (int32_t)logical_device->swapchain_extent.width;
	offscreenPass.offscreenImage.texHeight = (int32_t)logical_device->swapchain_extent.height;
	offscreenPass.offscreenImage.Init(logical_device, VK_FORMAT_R8G8B8A8_UNORM);
	offscreenPass.offscreenImage.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	offscreenPass.offscreenImage.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);
	offscreenPass.offscreenImage.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	offscreenPass.offscreenDepthImage.texWidth = (int32_t)logical_device->swapchain_extent.width; 
	offscreenPass.offscreenDepthImage.texHeight = (int32_t)logical_device->swapchain_extent.height; 
	offscreenPass.offscreenDepthImage.Init(logical_device, logical_device->FindDepthFormat());
	offscreenPass.offscreenDepthImage.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	offscreenPass.offscreenDepthImage.CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
	//offscreenPass.offscreenDepthImage.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
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

	ErrorCheck(vkCreateImage(logical_device->device, &ImageInfo, nullptr, &layer.texture));

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(logical_device->device, layer.texture, &memory_requirements); // TODO

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memory_requirements.size;
	allocInfo.memoryTypeIndex = logical_device->FindMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(logical_device->device, &allocInfo, nullptr, &layer.texture_image_memory) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to allocate image memory!");
		std::exit(-1);
	}

	ErrorCheck(vkBindImageMemory(logical_device->device, layer.texture, layer.texture_image_memory, 0));

	enginetool::Buffer texture_staging_buffer;
	logical_device->CreateStagedBuffer(texCube.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &texture_staging_buffer, texCube.data());
	
	vkGetBufferMemoryRequirements(logical_device->device, texture_staging_buffer.buffer, &memory_requirements);

	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	std::vector<VkBufferImageCopy> bufferCopyRegions;
	uint32_t offset = 0;

	for (uint32_t face = 0; face < 6; face++)
	{
		for (uint32_t level = 0; level < static_cast<uint32_t>(mipLevels); level++)
		{
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

	EndSingleTimeCommands(command_buffer);

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

	if (vkCreateSampler(logical_device->device, &SamplerInfo, nullptr, &layer.texture_sampler) != VK_SUCCESS) {
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

	if (vkCreateImageView(logical_device->device, &ViewInfo, nullptr, &layer.texture_image_view) != VK_SUCCESS)	{
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
	logical_device->CreateStagedBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &image_staging_buffer, pixels);

	stbi_image_free(pixels);
	
	layer.Init(logical_device, VK_FORMAT_R8G8B8A8_UNORM);
	layer.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	layer.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	layer.CopyBufferToImage(image_staging_buffer.buffer);
	layer.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	image_staging_buffer.Destroy();

	layer.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);
	layer.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

VkImageView Scene::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)//TODO
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
	if (vkCreateImageView(logical_device->device, &ViewInfo, nullptr, &image_view) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to create texture image view!");
		std::exit(-1);
	}

	return image_view;
}

// --------------- Depth buffering ------------------ //

void Scene::CreateDepthResources() {
	depthImage.Init(logical_device, logical_device->FindDepthFormat());

	depthImage.texWidth = logical_device->swapchain_extent.width; 
	depthImage.texHeight = logical_device->swapchain_extent.height; 

	depthImage.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	depthImage.CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT);

	depthImage.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
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
		actors[0]->Truck(-axes[0] * 15.0f);
		actors[0]->Dolly(axes[1] * 15.0f);

		std::dynamic_pointer_cast<Camera>(actors[0])->GamepadMove(-axes[2], axes[3], WIDTH, HEIGHT, 0.15f);

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
			actors[0]->ResetPosition();
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
		case GLFW_KEY_W:
			std::cout << "Moving " << actors[0]->name << " foward " << key << std::endl;
			actors[0]->Dolly(15.0f);
			break;
		case GLFW_KEY_S:
			std::cout << "Moving " << actors[0]->name << " back " << key << std::endl;
			actors[0]->Dolly(-15.0f);
			break;
		case GLFW_KEY_E:
			std::cout << "Moving " << actors[0]->name << " up " << key << std::endl;
			actors[0]->Pedestal(15.0f);
			break;
		case GLFW_KEY_Q:
			std::cout << "Moving " << actors[0]->name << " down " << key << std::endl;
			actors[0]->Pedestal(-15.0f);
			break;
		case GLFW_KEY_D:
			std::cout << "Moving " << actors[0]->name << " right " << key << std::endl;
			actors[0]->Truck(-15.0f);
			break;
		case GLFW_KEY_A:
			std::cout << "Moving " << actors[0]->name << " left " << key << std::endl;
			actors[0]->Truck(15.0f);
			break;
		case GLFW_KEY_R:
			std::cout << "Reset " << actors[0]->name << " "<< key << std::endl;
			actors[0]->ResetPosition();
			break;
		case GLFW_KEY_8:
			std::cout << "Moving " << actors[3]->name << " foward " << key << std::endl;
			actors[2]->Dolly(15.0f);
			break;
		case GLFW_KEY_7:
			std::cout << "Moving " << actors[3]->name << " back " << key << std::endl;
			actors[2]->Dolly(-15.0f);
			break;
		case GLFW_KEY_6:
			std::cout << "Moving " << actors[3]->name << " left " << key << std::endl;
			actors[2]->Strafe(15.0f);
			break;
		case GLFW_KEY_5:
			std::cout << "Moving " << actors[3]->name << " right " << key << std::endl;
			actors[2]->Strafe(-15.0f);
			break;
		case GLFW_KEY_0:
			std::cout << "Moving " << actors[2]->name << " up " << key << std::endl;
			actors[2]->Pedestal(15.0f);
			break;
		case GLFW_KEY_9:
			std::cout << "Moving " << actors[2]->name << " down " << key << std::endl;
			actors[2]->Pedestal(-15.0f);
			break;
		case GLFW_KEY_UP:
			std::cout << "Moving " << actors[1]->name << " foward " << key << std::endl;
			actors[1]->Dolly(15.0f);
			break;
		case GLFW_KEY_DOWN:
			std::cout << "Moving " << actors[1]->name << " back " << key << std::endl;
			actors[1]->Dolly(-15.0f);
			break;
		case GLFW_KEY_LEFT:
			std::cout << "Moving " << actors[1]->name << " left " << key << std::endl;
			actors[1]->Strafe(15.0f);
			break;
		case GLFW_KEY_RIGHT:
			std::cout << "Moving " << actors[1]->name << " right " << key << std::endl;
			actors[1]->Strafe(-15.0f);
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
			status_overlay->guiOverlayVisible = !status_overlay->guiOverlayVisible;
			break;
		case GLFW_KEY_2:
			status_overlay->ui_settings.display_stats_overlay = !status_overlay->ui_settings.display_stats_overlay;
			break;
		case GLFW_KEY_3:
			if (status_overlay->ui_settings.display_imgui)	{
				status_overlay->ui_settings.display_imgui = false;
				std::cout << "Console turned off! " << key << std::endl;
			}
			else {
				status_overlay->ui_settings.display_imgui = true;
				std::cout << "Console turned on! " << key << std::endl;
			}
			break;
		case GLFW_KEY_4:
			status_overlay->ui_settings.display_main_ui = !status_overlay->ui_settings.display_main_ui;
			break;
		}
	}

	if (state == GLFW_RELEASE)
	{

		switch (key)
		{
		case GLFW_KEY_W:
			std::cout << "Key released: " << key << std::endl;
			actors[0]->Dolly(0.0f);
			break;
		case GLFW_KEY_E:
			std::cout << "Key released: " << key << std::endl;
			actors[0]->Pedestal(0.0f);
			break;
		case GLFW_KEY_S:
			std::cout << "Key released: " << key << std::endl;
			actors[0]->Dolly(0.0f);
			break;
		case GLFW_KEY_Q:
			std::cout << "Key released: " << key << std::endl;
			actors[0]->Pedestal(0.0f);
			break;
		case GLFW_KEY_D:
			std::cout << "Key released: " << key << std::endl;
			actors[0]->Truck(0.0f);
			break;
		case GLFW_KEY_A:
			std::cout << "Key released: " << key << std::endl;
			actors[0]->Truck(0.0f);
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
			std::cout << "Key released: " << key << std::endl;
			actors[1]->Dolly(0.0f);
			break;
		case GLFW_KEY_DOWN:
			std::cout << "Key released: " << key << std::endl;
			actors[1]->Dolly(0.0f);
			break;
		case GLFW_KEY_LEFT:
			std::cout << "Key released:" << key << std::endl;
			actors[1]->Strafe(0.0f);
			break;
		case GLFW_KEY_RIGHT:
			std::cout << "Key released: " << key << std::endl;
			actors[1]->Strafe(0.0f);
			break;
		case GLFW_KEY_Z:
			std::cout <<  "Key released: " << key << std::endl;
			break;
		case GLFW_KEY_SPACE:
			std::cout << "Key released: " << key << std::endl;
			std::cout << "Going back ground level" << std::endl;
			std::dynamic_pointer_cast<Character>(actors[1])->EndJump();			
			break;
		//case GLFW_KEY_1:
		//	std::cout << "Key released: " << key << std::endl;
		//	//
		//	break;
		}
	}

}


// ---------------- Deinitialisation ---------------- //

void Scene::CleanUpForSwapchain() {
	DeInitImageView();
	DeInitFramebuffer();
	FreeCommandBuffers();
	DestroyPipeline();
	vkDestroyPipelineLayout(logical_device->device, pipeline_layout, nullptr);
	//logical_device->DestroyRenderPass();
}

void Scene::DeInitFramebuffer() {
	for (size_t i = 0; i < logical_device->swap_chain_framebuffers.size(); i++) {
		vkDestroyFramebuffer(logical_device->device, logical_device->swap_chain_framebuffers[i], nullptr);
	}

	vkDestroyFramebuffer(logical_device->device, logical_device->offscreenFramebuffer, nullptr);
}

void Scene::DeInitImageView() {
	depthImage.DeInit();
	offscreenPass.offscreenDepthImage.DeInit();
}

void Scene::DeInitIndexAndVertexBuffer() {
	index_buffers.ocean.Destroy();
	vertex_buffers.ocean.Destroy();

	index_buffers.skybox.Destroy();
	vertex_buffers.skybox.Destroy();

	index_buffers.clouds.Destroy();
	vertex_buffers.clouds.Destroy();
	
	index_buffers.objects.Destroy();//destroy only first model!
	vertex_buffers.objects.Destroy();//destroy only first model!
}

void Scene::DeInitScene()
{
	status_overlay = nullptr;
	console = nullptr;
	
	DeInitIndexAndVertexBuffer();
	DeInitTextureImage();
	vkDestroyDescriptorPool(logical_device->device, descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(logical_device->device, descriptor_set_layout, nullptr);
	vkDestroyDescriptorSetLayout(logical_device->device, oceanDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logical_device->device, skybox_descriptor_set_layout, nullptr);
	vkDestroyDescriptorSetLayout(logical_device->device, clouds_descriptor_set_layout, nullptr);

	vkDestroyCommandPool(logical_device->device, command_pool, nullptr);
	DeInitUniformBuffer();
	
	delete rust;
	delete sky;
	delete chrome;
	delete plastic;
	delete lightbulbMat;
	delete cameraMat;
	delete characterMat;
	
	logical_device = nullptr;
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
	vkDestroyImageView(logical_device->device, sky->skybox_texture.texture_image_view, nullptr);
	vkDestroyImage(logical_device->device, sky->skybox_texture.texture, nullptr);
	vkDestroySampler(logical_device->device, sky->skybox_texture.texture_sampler, nullptr);
	vkFreeMemory(logical_device->device, sky->skybox_texture.texture_image_memory, nullptr);

	for (size_t i = 0; i < scene_material.size(); i++) {
		scene_material[i].albedo.DeInit();
		scene_material[i].metallic.DeInit();
		scene_material[i].roughness.DeInit();
		scene_material[i].normal_map.DeInit();
		scene_material[i].ambient_occlucion_map.DeInit();
	}

	offscreenPass.offscreenImage.DeInit();
}

void Scene::DeInitUniformBuffer() {
	if (uboDataDynamic.model) {
		alignedFree(uboDataDynamic.model);
	}

	uniform_buffers.skybox.Destroy();
	uniform_buffers.ocean.Destroy();
	uniform_buffers.objects.Destroy();
	uniform_buffers.parameters.Destroy();
	uniform_buffers.clouds.Destroy();
	uniform_buffers.clouds_dynamic.Destroy();
	uniform_buffers.mirrored.Destroy();
	uniform_buffers.mirroredParameters.Destroy();
}

void Scene::DestroyPipeline() {
	vkDestroyPipeline(logical_device->device, skybox_pipeline, nullptr);
	vkDestroyPipeline(logical_device->device, oceanPipeline, nullptr);
	vkDestroyPipeline(logical_device->device, pbr_pipeline, nullptr);
	vkDestroyPipeline(logical_device->device, clouds_pipeline, nullptr);
	vkDestroyPipeline(logical_device->device, offscreenPipeline, nullptr);
}

void Scene::FreeCommandBuffers() {
	vkFreeCommandBuffers(logical_device->device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
	vkFreeCommandBuffers(logical_device->device, command_pool, 1, &commandBuffer);
}