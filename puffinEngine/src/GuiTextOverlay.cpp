#include <iostream>

#include "LoadFile.cpp"
#include "GuiTextOverlay.hpp"

#define TEXTOVERLAY_MAX_CHAR_COUNT 2048

//---------- Constructors and dectructors ---------- //

GuiTextOverlay::GuiTextOverlay() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui - text overlay - created\n";
#endif 
}

GuiTextOverlay::~GuiTextOverlay() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui - text overlay - destroyed\n";
#endif
}

// --------------- Setters and getters -------------- //



// ---------------- Main functions ------------------ //

void GuiTextOverlay::Init(Device* device) {
	logical_device = device; 
	
	SetUp();
	LoadImage();
	CreateDescriptorSetLayout();
	CreateDescriptorPool();
	CreateDescriptorSet();
	CreateGraphicsPipeline();
}

void GuiTextOverlay::SetUp(){
}

void GuiTextOverlay::LoadImage() {
	font.texWidth = STB_FONT_WIDTH;
	font.texHeight = STB_FONT_HEIGHT;
	static unsigned char font24pixels[STB_FONT_HEIGHT][STB_FONT_WIDTH];
	STB_FONT_NAME(stbFontData, font24pixels, STB_FONT_HEIGHT);

	VkDeviceSize indexBufferSize = TEXTOVERLAY_MAX_CHAR_COUNT * sizeof(glm::vec4);
	VkDeviceSize uploadSize = STB_FONT_WIDTH * STB_FONT_HEIGHT * sizeof(int);

	enginetool::Buffer staging_buffer;
	logical_device->CreateStagedBuffer(uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &stbFontData);

	font.Init(logical_device, VK_FORMAT_R8_UNORM);
	font.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	
	staging_buffer.Map(indexBufferSize, 0);
	staging_buffer.CopyTo(&font24pixels[0][0], STB_FONT_WIDTH * STB_FONT_HEIGHT);
	staging_buffer.Unmap();

	font.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	font.CopyBufferToImage(staging_buffer.buffer);
	font.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	staging_buffer.Destroy();

	font.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	font.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);

	logical_device->CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, memory);
}

void GuiTextOverlay::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding TextOverlayLayoutBinding = {};
	TextOverlayLayoutBinding.binding = 0;
	TextOverlayLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	TextOverlayLayoutBinding.descriptorCount = 1;
	TextOverlayLayoutBinding.pImmutableSamplers = nullptr;
	TextOverlayLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> set_layout_bindings = { TextOverlayLayoutBinding };

	VkDescriptorSetLayoutCreateInfo SceneObjectsLayoutInfo = {};
	SceneObjectsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	SceneObjectsLayoutInfo.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());
	SceneObjectsLayoutInfo.pBindings = set_layout_bindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logical_device->device, &SceneObjectsLayoutInfo, nullptr, &descriptorSetLayout));
}

void GuiTextOverlay::CreateDescriptorPool() {
	// Don't forget to rise this numbers when you add bindings
	std::array<VkDescriptorPoolSize, 1> PoolSizes = {};
	PoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	PoolSizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo PoolInfo = {};
	PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.maxSets = 1; // maximum number of descriptor sets that will be allocated

	ErrorCheck(vkCreateDescriptorPool(logical_device->device, &PoolInfo, nullptr, &descriptorPool));
}

void GuiTextOverlay::CreateDescriptorSet() {
		VkDescriptorSetAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfo.descriptorPool = descriptorPool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &descriptorSetLayout;

		ErrorCheck(vkAllocateDescriptorSets(logical_device->device, &AllocInfo, &descriptorSet));

		VkDescriptorImageInfo ImageInfo = {};
		ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageInfo.imageView = font.texture_image_view;
		ImageInfo.sampler = font.texture_sampler;

		std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &ImageInfo;

		vkUpdateDescriptorSets(logical_device->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void GuiTextOverlay::CreateGraphicsPipeline() {
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	ErrorCheck(vkCreatePipelineCache(logical_device->device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {}; // describes what kind of geometry will be drawn from the vertices and if primitive restart should be enabled
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; // triangle from every 3 vertices with reuse
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo Rasterization = {}; // takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader
	Rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	Rasterization.depthClampEnable = VK_FALSE;
	Rasterization.rasterizerDiscardEnable = VK_FALSE; // geometry never passes through the rasterizer stage
	Rasterization.polygonMode = VK_POLYGON_MODE_FILL; // determines how fragments are generated for geometry
	Rasterization.lineWidth = 1.0f;
	Rasterization.cullMode = VK_CULL_MODE_NONE;
	Rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	Rasterization.depthBiasEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; // Enable blending
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo ColorBlending = {};
	ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	ColorBlending.logicOpEnable = VK_FALSE;
	ColorBlending.logicOp = VK_LOGIC_OP_COPY;
	ColorBlending.attachmentCount = 1;
	ColorBlending.pAttachments = &colorBlendAttachment;
	ColorBlending.blendConstants[0] = 0.0f;
	ColorBlending.blendConstants[1] = 0.0f;
	ColorBlending.blendConstants[2] = 0.0f;
	ColorBlending.blendConstants[3] = 0.0f;

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

	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo ViewportDynamic = {};
	ViewportDynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	ViewportDynamic.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	ViewportDynamic.pDynamicStates = dynamicStateEnables.data();

	std::array<VkVertexInputBindingDescription, 2> vertex_bindings = {};
	vertex_bindings[0].binding = 0;
	vertex_bindings[0].stride = sizeof(glm::vec4);
	vertex_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	vertex_bindings[1].binding = 1;
	vertex_bindings[1].stride = sizeof(glm::vec4);
	vertex_bindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 2> vertex_attributes = {};
	vertex_attributes[0].binding = 0;
	vertex_attributes[0].location = 0;
	vertex_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_attributes[0].offset = 0;
	
	vertex_attributes[1].binding = 1;
	vertex_attributes[1].location = 1;
	vertex_attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_attributes[1].offset = sizeof(glm::vec2);

	VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
	VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_bindings.size());
	VertexInputInfo.pVertexBindingDescriptions = vertex_bindings.data();
	VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attributes.size());
	VertexInputInfo.pVertexAttributeDescriptions = vertex_attributes.data();

	std::array<VkDescriptorSetLayout, 1> layouts = {descriptorSetLayout};

	VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
	PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	PipelineLayoutInfo.pSetLayouts = layouts.data();
	
	ErrorCheck(vkCreatePipelineLayout(logical_device->device, &PipelineLayoutInfo, nullptr, &pipelineLayout));

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
	PipelineInfo.renderPass = logical_device->renderPass;
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	
	auto vertModelsShaderCode = enginetool::readFile("puffinEngine/shaders/perform_stats.vert.spv");
	auto fragModelsShaderCode = enginetool::readFile("puffinEngine/shaders/perform_stats.frag.spv");

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

	ErrorCheck(vkCreateGraphicsPipelines(logical_device->device, pipelineCache, 1, &PipelineInfo, nullptr, &pipeline));

	vkDestroyShaderModule(logical_device->device, fragModelsShaderModule, nullptr);
	vkDestroyShaderModule(logical_device->device, vertModelsShaderModule, nullptr);
}

void GuiTextOverlay::BeginTextUpdate() {
	ErrorCheck(vkMapMemory(logical_device->device, memory, 0, VK_WHOLE_SIZE, 0, (void **)&mapped));
	num_letters = 0;
}

void GuiTextOverlay::RenderText(std::string text, float x, float y, TextAlignment align) {
	assert(mapped != nullptr);

	float fbW = (float)logical_device->swapchain_extent.width;
	float fbH = (float)logical_device->swapchain_extent.height;

	const float charW = 1.5f / fbW;
	const float charH = 1.5f / fbH;

	x = (x / fbW * 2.0f) - 1.0f;
	y = (y / fbH * 2.0f) - 1.0f;

	// Calculate text width
	float textWidth = 0;
	for (auto letter : text) {
		stb_fontchar *charData = &stbFontData[(uint32_t)letter - STB_FIRST_CHAR];
		textWidth += charData->advance * charW;
	}

	switch (align) {
	case TextAlignment::alignRight:
		x -= textWidth;
		break;
	case TextAlignment::alignCenter:
		x -= textWidth / 2.0f;
		break;
	}

	// Generate a uv mapped quad per char in the new text
	for (auto letter : text) {
		stb_fontchar *charData = &stbFontData[(uint32_t)letter - STB_FIRST_CHAR];

		mapped->x = (x + (float)charData->x0 * charW);
		mapped->y = (y + (float)charData->y0 * charH);
		mapped->z = charData->s0;
		mapped->w = charData->t0;
		mapped++;

		mapped->x = (x + (float)charData->x1 * charW);
		mapped->y = (y + (float)charData->y0 * charH);
		mapped->z = charData->s1;
		mapped->w = charData->t0;
		mapped++;

		mapped->x = (x + (float)charData->x0 * charW);
		mapped->y = (y + (float)charData->y1 * charH);
		mapped->z = charData->s0;
		mapped->w = charData->t1;
		mapped++;

		mapped->x = (x + (float)charData->x1 * charW);
		mapped->y = (y + (float)charData->y1 * charH);
		mapped->z = charData->s1;
		mapped->w = charData->t1;
		mapped++;

		x += charData->advance * charW;

		num_letters++;
	}
}

// Unmap buffer and update command buffers
void GuiTextOverlay::EndTextUpdate() {
	vkUnmapMemory(logical_device->device, memory);
	mapped = nullptr;
}

void GuiTextOverlay::CreateUniformBuffer(VkCommandBuffer command_buffer) {
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)logical_device->swapchain_extent.width;
	viewport.height = (float)logical_device->swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	scissor.offset = { 0, 0 }; // scissor rectangle covers framebuffer entirely
	scissor.extent = logical_device->swapchain_extent;	
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);
	
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertexBuffer, offsets);
	vkCmdBindVertexBuffers(command_buffer, 1, 1, &vertexBuffer, offsets);

	for (uint32_t j = 0; j < num_letters; j++) {
		vkCmdDraw(command_buffer, 4, 1, j * 4, 0);
	}
}

void GuiTextOverlay::DeInit() {
	vkDestroyBuffer(logical_device->device, vertexBuffer, nullptr);
	vkFreeMemory(logical_device->device, memory, nullptr);
	font.DeInit();
	vkDestroyPipelineCache(logical_device->device, pipelineCache, nullptr);
	vkDestroyPipeline(logical_device->device, pipeline, nullptr);
	vkDestroyPipelineLayout(logical_device->device, pipelineLayout, nullptr);
	vkDestroyDescriptorPool(logical_device->device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(logical_device->device, descriptorSetLayout, nullptr);

	mapped = nullptr;
}
