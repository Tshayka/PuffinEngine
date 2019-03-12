#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>

#include "LoadFile.cpp"
#include "GuiMainUi.hpp"

//---------- Constructors and dectructors ---------- //

GuiMainUi::GuiMainUi() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui - main ui - created\n";
#endif 
}

GuiMainUi::~GuiMainUi() {	
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui - main ui - destroyed\n";
#endif
}

void GuiMainUi::Init(Device* device, VkCommandPool& commandPool) {
	logicalDevice = device;
	this->commandPool = &commandPool; 

	SetUp();
	LoadImage();
	CreateDescriptorSetLayout();
	CreateDescriptorPool();
	CreateDescriptorSet();
	CreateGraphicsPipeline();
}

void GuiMainUi::SetUp() {
	GetDrawData();
}

void GuiMainUi::LoadImage() {
    ImGuiIO& io = ImGui::GetIO();

	unsigned char* fontData;
	io.Fonts->GetTexDataAsRGBA32(&fontData, (int*)&font.texWidth, (int*)&font.texHeight);
	
	VkDeviceSize imageSize = font.texWidth * font.texHeight * 4 * sizeof(char);
	enginetool::Buffer stagingBuffer;
	stagingBuffer.CreateBuffer(logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, fontData);
	
	font.Init(logicalDevice, *commandPool, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 1);
	font.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	font.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	font.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);
	
	font.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	font.CopyBufferToImage(stagingBuffer.buffer);
	stagingBuffer.Destroy();
	font.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GuiMainUi::CreateDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding ConsoleLayoutBinding = {};
	ConsoleLayoutBinding.binding = 0;
	ConsoleLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	ConsoleLayoutBinding.descriptorCount = 1;
	ConsoleLayoutBinding.pImmutableSamplers = nullptr;
	ConsoleLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> set_layout_bindings = { ConsoleLayoutBinding };

	VkDescriptorSetLayoutCreateInfo SceneObjectsLayoutInfo = {};
	SceneObjectsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	SceneObjectsLayoutInfo.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());
	SceneObjectsLayoutInfo.pBindings = set_layout_bindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->device, &SceneObjectsLayoutInfo, nullptr, &descriptorSetLayout));
}


void GuiMainUi::CreateDescriptorPool() {
	// Don't forget to rise this numbers when you add bindings
	std::array<VkDescriptorPoolSize, 1> PoolSizes = {};
	PoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	PoolSizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo PoolInfo = {};
	PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.maxSets = 2; // maximum number of descriptor sets that will be allocated

	ErrorCheck(vkCreateDescriptorPool(logicalDevice->device, &PoolInfo, nullptr, &descriptorPool));
}

void GuiMainUi::CreateDescriptorSet() {
	VkDescriptorSetAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AllocInfo.descriptorPool = descriptorPool;
	AllocInfo.descriptorSetCount = 1;
	AllocInfo.pSetLayouts = &descriptorSetLayout;

	ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &AllocInfo, &descriptorSet));

	VkDescriptorImageInfo ImageInfo = {};
	ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ImageInfo.imageView = font.view;
	ImageInfo.sampler = font.sampler;

	std::array<VkWriteDescriptorSet, 1> WriteDescriptorSets = {};

	WriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	WriteDescriptorSets[0].dstSet = descriptorSet;
	WriteDescriptorSets[0].dstBinding = 0;
	WriteDescriptorSets[0].dstArrayElement = 0;
	WriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	WriteDescriptorSets[0].descriptorCount = 1;
	WriteDescriptorSets[0].pImageInfo = &ImageInfo;

	vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(WriteDescriptorSets.size()), WriteDescriptorSets.data(), 0, nullptr);
}

void GuiMainUi::CreateGraphicsPipeline() {
	VkPipelineCacheCreateInfo PipelineCacheCreateInfo = {};
	PipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	ErrorCheck(vkCreatePipelineCache(logicalDevice->device, &PipelineCacheCreateInfo, nullptr, &pipelineCache));

	VkPushConstantRange PushConstantRange = {};
	PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	PushConstantRange.offset = 0;
	PushConstantRange.size = sizeof(PushConstBlock);
	
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {}; // describes what kind of geometry will be drawn from the vertices and if primitive restart should be enabled
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // triangle from every 3 vertices without reuse
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo Rasterization = {}; // takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader
	Rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	Rasterization.depthClampEnable = VK_FALSE;
	Rasterization.rasterizerDiscardEnable = VK_FALSE; // geometry never passes through the rasterizer stage
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL; // determines how fragments are generated for geometry
	Rasterization.lineWidth = 1.0f;
	Rasterization.cullMode = VK_CULL_MODE_NONE;
	Rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;//VK_FRONT_FACE_COUNTER_CLOCKWISE
	Rasterization.depthBiasEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState ColorBlendAttachment = {}; // Enable blending
	ColorBlendAttachment.blendEnable = VK_TRUE;
	ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	
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
	DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	std::array<VkDynamicState, 2> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo ViewportDynamic = {};
	ViewportDynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	ViewportDynamic.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	ViewportDynamic.pDynamicStates = dynamicStateEnables.data();

	std::array<VkVertexInputBindingDescription, 1> vertex_bindings = {};
	vertex_bindings[0].binding = 0;
	vertex_bindings[0].stride = sizeof(Vertex);
	vertex_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 3> vertex_attributes = {};
	vertex_attributes[0].binding = 0;
	vertex_attributes[0].location = 0;
	vertex_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_attributes[0].offset = offsetof(Vertex, pos);

	vertex_attributes[1].binding = 0;
	vertex_attributes[1].location = 1;
	vertex_attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_attributes[1].offset = offsetof(Vertex, uv);

	vertex_attributes[2].binding = 0;
	vertex_attributes[2].location = 2;
	vertex_attributes[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	vertex_attributes[2].offset = offsetof(Vertex, col); 

	VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
	VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_bindings.size());
	VertexInputInfo.pVertexBindingDescriptions = vertex_bindings.data();
	VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attributes.size());
	VertexInputInfo.pVertexAttributeDescriptions = vertex_attributes.data();
	
	std::array<VkDescriptorSetLayout, 1> layouts = { descriptorSetLayout };

	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
	PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstantRange;
	PipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());;
	PipelineLayoutCreateInfo.pSetLayouts = layouts.data();
	
	ErrorCheck(vkCreatePipelineLayout(logicalDevice->device, &PipelineLayoutCreateInfo, nullptr, &pipelineLayout));

	std::array <VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo PipelineInfo = {};
	PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	PipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	PipelineInfo.pStages = shaderStages.data();
	PipelineInfo.pVertexInputState = &VertexInputInfo;
	PipelineInfo.pInputAssemblyState = &inputAssembly;
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

	auto vertModelsShaderCode = enginetool::readFile("puffinEngine/shaders/imgui_menu_shader.vert.spv"); 
	auto fragModelsShaderCode = enginetool::readFile("puffinEngine/shaders/imgui_menu_shader.frag.spv"); 

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

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, pipelineCache, 1, &PipelineInfo, nullptr, &pipeline));

	vkDestroyShaderModule(logicalDevice->device, fragModelsShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->device, vertModelsShaderModule, nullptr); 
}

void GuiMainUi::NewFrame() {
    
}

void GuiMainUi::GetDrawData() {
	UiComponent rectangle;

    rectangle.position = glm::vec2(100.0f, 100.0f);

    rectangle.vertices = {
		{{-0.25f, -0.25f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}},
		{{0.25f, -0.25f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.5f}},
		{{0.25f, 0.25f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 0.5f}},
		{{-0.25f, 0.25f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.5f}}
    };

    rectangle.indices = { 0, 1, 2, 2, 3, 0 };

    float clipX1 = (float)(int)(0.5f + rectangle.position.x - 1.0f); //add column offset
    float clipX2 = (float)(int)(0.5f + rectangle.position.x - 1.0f);

    rectangle.clipExtent = glm::vec4(clipX1, std::numeric_limits<float>::lowest(), clipX2, std::numeric_limits<float>::max()); 

    drawData.componentsToDraw.push_back(rectangle);

	for (int32_t i = 0; i < drawData.componentsToDraw.size(); i++) {
        drawData.totalVerticesCount += drawData.componentsToDraw[i].vertices.size();
        drawData.totalIndicesCount += drawData.componentsToDraw[i].indices.size();
    }
}

void GuiMainUi::UpdateDrawData() {
	if (drawData.totalVerticesCount == 0)
		return;
	
	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = drawData.totalVerticesCount * sizeof(Vertex);
	VkDeviceSize indexBufferSize = drawData.totalIndicesCount * sizeof(uint16_t);

	// Update buffers only if vertex or index count has been changed compared to current buffer size

	// Vertex buffer
	if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != drawData.totalVerticesCount)) {
		vertexBuffer.Unmap();
		vertexBuffer.Destroy();

		vertexBuffer.CreateBuffer(logicalDevice, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr);
		vertexCount = drawData.totalVerticesCount;
		vertexBuffer.Map();
	}

	// Index buffer
	if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < drawData.totalIndicesCount)) {
		indexBuffer.Unmap();
		indexBuffer.Destroy();

		indexBuffer.CreateBuffer(logicalDevice, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr);
		indexCount = drawData.totalIndicesCount;
		indexBuffer.Map();
	}

	 Vertex* vtxDst = (Vertex*)vertexBuffer.mapped;
	 uint16_t* idxDst = (uint16_t*)indexBuffer.mapped;

	 for (int32_t i = 0; i < drawData.componentsToDraw.size(); i++) {
	    memcpy(vtxDst, drawData.componentsToDraw[i].vertices.data(), drawData.componentsToDraw[i].vertices.size() * sizeof(Vertex));
	  	memcpy(idxDst, drawData.componentsToDraw[i].indices.data(), drawData.componentsToDraw[i].indices.size() * sizeof(uint16_t));
	 	vtxDst += drawData.componentsToDraw[i].vertices.size() ;
	 	idxDst += drawData.componentsToDraw[i].indices.size();
	}

	vertexBuffer.Flush();
	indexBuffer.Flush();
}

void GuiMainUi::CreateUniformBuffer(VkCommandBuffer command_buffer) {
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)logicalDevice->swapchain_extent.width;
	viewport.height = (float)logicalDevice->swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	// UI scale and translate via push constants
	pushConstBlock.scale = glm::vec2(2.0f / viewport.width, 2.0f / viewport.height );
	pushConstBlock.translate = glm::vec2(-1.0f);
	vkCmdPushConstants(command_buffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

	VkDeviceSize offsets[1] = { 0 };

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertexBuffer.buffer, offsets);
	vkCmdBindIndexBuffer(command_buffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

	// Render the command lists:
	int32_t vertexOffset = 0;
	int32_t indexOffset = 0;

	

	for (int32_t i = 0; i < drawData.componentsToDraw.size(); i++) {
		scissor.offset.x = 0.0f;//std::max((int32_t)(drawData.componentsToDraw[i].clipExtent.x),0);
		scissor.offset.y = 0.0f;//std::max((int32_t)(drawData.componentsToDraw[i].clipExtent.y), 0);
		scissor.extent.width = (float)logicalDevice->swapchain_extent.width;//(uint32_t)(drawData.componentsToDraw[i].clipExtent.z - drawData.componentsToDraw[i].clipExtent.x);
		scissor.extent.height = (float)logicalDevice->swapchain_extent.height;//(uint32_t)(drawData.componentsToDraw[i].clipExtent.w - drawData.componentsToDraw[i].clipExtent.y);
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);
		vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(drawData.componentsToDraw[i].indices.size()), 1, indexOffset, vertexOffset, 0);
		//vkCmdDrawIndexed(command_buffers[i], actors[j]->mesh.indexCount, 1, 0, actors[j]->mesh.indexBase, 0);
		indexOffset += static_cast<uint32_t>(drawData.componentsToDraw[i].indices.size());
		vertexOffset += static_cast<uint32_t>(drawData.componentsToDraw[i].vertices.size());
	}

	//std::cout << drawData.componentsToDraw[0].indices.size() << " " <<  drawData.componentsToDraw[0].vertices.size() <<  std::endl;
	//std::cout << scissor.offset.x << " " << scissor.offset.y << std::endl;
	//std::cout << scissor.extent.width << " " << scissor.extent.height << std::endl;
}

void GuiMainUi::DeInit() {
	indexBuffer.Destroy();
	vertexBuffer.Destroy();
	font.DeInit();
	vkDestroyPipelineCache(logicalDevice->device, pipelineCache, nullptr);
	vkDestroyPipeline(logicalDevice->device, pipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice->device, pipelineLayout, nullptr);
	vkDestroyDescriptorPool(logicalDevice->device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->device, descriptorSetLayout, nullptr);

	logicalDevice = nullptr;
	commandPool = nullptr;
}
