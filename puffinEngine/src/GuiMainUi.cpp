#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>

#define GLM_FORCE_RADIANS // necessary to make sure that functions like glm::rotate use radians as arguments, to avoid any possible confusion
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp> 

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

void GuiMainUi::Init(Device* device, MaterialLibrary* materialLibrary, VkCommandPool& commandPool, VkRenderPass& renderPass) {
	logicalDevice = device;
	this->commandPool = &commandPool;
	this->materialLibrary = materialLibrary;	
	this->renderPass = &renderPass; 

	SetUp();
	CreateBuffers(); 
	CreateDescriptorSetLayout();
	CreateDescriptorPool();
	CreateDescriptorSet();
	CreateGraphicsPipeline();
}

void GuiMainUi::SetUp() {
	GetDrawData();
}

void GuiMainUi::GenerateText(UiComponent& word, std::string text, TextAlignment align, glm::vec2 position, float scale, float maxWidth, float lineHeight, glm::vec2 padding) {
	uint32_t indexOffset = 0;
	float wrapWidth = maxWidth;

	float w = materialLibrary->font.texWidth;

	float posx = position.x + padding.x;
	float posy = position.y + padding.y;

	for (uint32_t i = 0; i < text.size(); i++) {
		MaterialLibrary::bmchar *charInfo = &materialLibrary->fontChars[(int)text[i]];

		if (charInfo->width == 0)
			charInfo->width = 146;

		float charw = ((float)(charInfo->width) / 146.0f);
		float dimx = scale * charw;
		float charh = ((float)(charInfo->height) / 146.0f);
		float dimy = scale * charh;
		//posy = 1.0f;

		float us = charInfo->x / w;
		float ue = (charInfo->x + charInfo->width) / w;
		float ts = charInfo->y / w;
		float te = (charInfo->y + charInfo->height) / w;

		float xo = scale * charInfo->xoffset / 146.0f;
		float yo = scale * charInfo->yoffset / 146.0f;

		word.vertices.push_back({{ posx + xo, 			posy + yo}, { us, ts } });
		word.vertices.push_back({{ posx + dimx + xo,	posy + yo}, { ue, ts }});
		word.vertices.push_back({{ posx + dimx + xo, 	posy + dimy + yo}, { ue, te }});
		word.vertices.push_back({{ posx + xo,			posy + dimy + yo}, { us, te }});

		std::array<uint32_t, 6> letterIndices = { 0,1,2, 2,3,0 };
		for (const auto& index : letterIndices) {
			word.indices.push_back(indexOffset + index);
		}
		indexOffset += 4;

		float advance = ((float)(charInfo->xadvance) / 146.0f);
		posx += advance * scale;
	}
	indexCount = word.indices.size();

	// // Center
	// for (auto& v : word.vertices) {
	// 	v.pos[0] -= posx / 2.0f;
	// 	v.pos[1] -= 0.5f;
	// }


	// switch (align) {
	// case TextAlignment::alignRight:
	// 	x -= posx;
	// 	break;
	// case TextAlignment::alignCenter:
	// 	v.pos[0] -= posx / 2.0f;
	// 	v.pos[1] -= 0.5f;
	// 	break;
	// }
}

void GuiMainUi::CreateBuffers() {
	uniformBuffers.fontMatrices.CreateBuffer(logicalDevice, sizeof(UBOF), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, nullptr);
	uniformBuffers.fontMatrices.Map();
	uniformBuffers.fontSettings.CreateBuffer(logicalDevice, sizeof(UBOS), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, nullptr);
	uniformBuffers.fontSettings.Map();

	UpdateFontSettingsUniformBuffer();
	UpdateFontUniformBuffer();
}

void GuiMainUi::CreateDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding LayoutBinding = {};
	LayoutBinding.binding = 0;
	LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	LayoutBinding.descriptorCount = 1;
	LayoutBinding.pImmutableSamplers = nullptr;
	LayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding ImageLayoutBinding = {};
	ImageLayoutBinding.binding = 1;
	ImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	ImageLayoutBinding.descriptorCount = 1;
	ImageLayoutBinding.pImmutableSamplers = nullptr;
	ImageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding SettingsLayoutBinding = {};
	SettingsLayoutBinding.binding = 2;
	SettingsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	SettingsLayoutBinding.descriptorCount = 1;
	SettingsLayoutBinding.pImmutableSamplers = nullptr;
	SettingsLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 3> textLayoutBindings = { LayoutBinding, ImageLayoutBinding, SettingsLayoutBinding };

	VkDescriptorSetLayoutCreateInfo textLayoutInfo = {};
	textLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textLayoutInfo.bindingCount = static_cast<uint32_t>(textLayoutBindings.size());
	textLayoutInfo.pBindings = textLayoutBindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(logicalDevice->device, &textLayoutInfo, nullptr, &descriptorSetLayout));
}

void GuiMainUi::CreateDescriptorPool() {
	// Don't forget to rise this numbers when you add bindings
	std::array<VkDescriptorPoolSize, 2> PoolSizes = {};
	PoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	PoolSizes[0].descriptorCount = static_cast<uint32_t>(drawData.componentsToDraw.size());
	PoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	PoolSizes[1].descriptorCount = static_cast<uint32_t>(drawData.componentsToDraw.size()*2);

	VkDescriptorPoolCreateInfo PoolInfo = {};
	PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.maxSets =  static_cast<uint32_t>(drawData.componentsToDraw.size())+1; // maximum number of descriptor sets that will be allocated

	ErrorCheck(vkCreateDescriptorPool(logicalDevice->device, &PoolInfo, nullptr, &descriptorPool));
}

void GuiMainUi::CreateDescriptorSet() {
	for (auto& c : drawData.componentsToDraw) {
		VkDescriptorSetAllocateInfo textAllocInfo = {};
		textAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		textAllocInfo.descriptorPool = descriptorPool;
		textAllocInfo.descriptorSetCount = 1;
		textAllocInfo.pSetLayouts = &descriptorSetLayout;

		ErrorCheck(vkAllocateDescriptorSets(logicalDevice->device, &textAllocInfo, &c.descriptorSet));

		VkDescriptorBufferInfo fontMatricesBufferInfo = {};
		fontMatricesBufferInfo.buffer = uniformBuffers.fontMatrices.buffer;
		fontMatricesBufferInfo.offset = 0;
		fontMatricesBufferInfo.range = sizeof(UBOF);

		VkDescriptorImageInfo textImageInfo = {};
		textImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		textImageInfo.imageView = c.assignedTexture->view;
		textImageInfo.sampler = c.assignedTexture->sampler;

		VkDescriptorBufferInfo fontSettingsBufferInfo = {};
		fontSettingsBufferInfo.buffer = uniformBuffers.fontSettings.buffer;
		fontSettingsBufferInfo.offset = 0;
		fontSettingsBufferInfo.range = sizeof(UBOS);

		std::array<VkWriteDescriptorSet, 3> textDescriptorWrites = {};

		textDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		textDescriptorWrites[0].dstSet =  c.descriptorSet;
		textDescriptorWrites[0].dstBinding = 0;
		textDescriptorWrites[0].dstArrayElement = 0;
		textDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		textDescriptorWrites[0].descriptorCount = 1;
		textDescriptorWrites[0].pBufferInfo = &fontMatricesBufferInfo;

		textDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		textDescriptorWrites[1].dstSet =  c.descriptorSet;
		textDescriptorWrites[1].dstBinding = 1;
		textDescriptorWrites[1].dstArrayElement = 0;
		textDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textDescriptorWrites[1].descriptorCount = 1;
		textDescriptorWrites[1].pImageInfo = &textImageInfo;

		textDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		textDescriptorWrites[2].dstSet = c.descriptorSet;
		textDescriptorWrites[2].dstBinding = 2;
		textDescriptorWrites[2].dstArrayElement = 0;
		textDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		textDescriptorWrites[2].descriptorCount = 1;
		textDescriptorWrites[2].pBufferInfo = &fontSettingsBufferInfo;

		vkUpdateDescriptorSets(logicalDevice->device, static_cast<uint32_t>(textDescriptorWrites.size()), textDescriptorWrites.data(), 0, nullptr);
	}
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
	Rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
	Rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //VK_FRONT_FACE_COUNTER_CLOCKWISE
	Rasterization.depthBiasEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState ColorBlendAttachment = {}; // Enable blending
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

	std::array<VkVertexInputAttributeDescription, 2> vertex_attributes = {};
	vertex_attributes[0].binding = 0;
	vertex_attributes[0].location = 0;
	vertex_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_attributes[0].offset = 0;

	vertex_attributes[1].binding = 0;
	vertex_attributes[1].location = 1;
	vertex_attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_attributes[1].offset = sizeof(float) * 2;

	VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
	VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_bindings.size());
	VertexInputInfo.pVertexBindingDescriptions = vertex_bindings.data();
	VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attributes.size());
	VertexInputInfo.pVertexAttributeDescriptions = vertex_attributes.data();
	
	std::array<VkDescriptorSetLayout, 1> layouts = { descriptorSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &PushConstantRange;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());;
	pipelineLayoutInfo.pSetLayouts = layouts.data();

	ErrorCheck(vkCreatePipelineLayout(logicalDevice->device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

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
	PipelineInfo.renderPass = *renderPass;
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	// I. Image pipeline
	auto vertImageShaderCode = enginetool::readFile("puffinEngine/shaders/uiImageShader.vert.spv");
	auto fragImageShaderCode = enginetool::readFile("puffinEngine/shaders/uiImageShader.frag.spv"); 

	VkShaderModule vertImageShaderModule = logicalDevice->CreateShaderModule(vertImageShaderCode);
	VkShaderModule fragImageShaderModule = logicalDevice->CreateShaderModule(fragImageShaderCode);

	VkPipelineShaderStageCreateInfo vertImageShaderStageInfo = {};
	vertImageShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertImageShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertImageShaderStageInfo.module = vertImageShaderModule;
	vertImageShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragImageShaderStageInfo = {};
	fragImageShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragImageShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragImageShaderStageInfo.module = fragImageShaderModule;
	fragImageShaderStageInfo.pName = "main";

	shaderStages[0] = vertImageShaderStageInfo;
	shaderStages[1] = fragImageShaderStageInfo;

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, pipelineCache, 1, &PipelineInfo, nullptr, &imagePipeline));

	vkDestroyShaderModule(logicalDevice->device, fragImageShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->device, vertImageShaderModule, nullptr);

	// II. Text pipeline
	auto vertTextShaderCode = enginetool::readFile("puffinEngine/shaders/uiTextShader.vert.spv");
	auto fragTextShaderCode = enginetool::readFile("puffinEngine/shaders/uiTextShader.frag.spv"); 

	VkShaderModule vertTextShaderModule = logicalDevice->CreateShaderModule(vertTextShaderCode);
	VkShaderModule fragTextShaderModule = logicalDevice->CreateShaderModule(fragTextShaderCode);

	VkPipelineShaderStageCreateInfo vertTextShaderStageInfo = {};
	vertTextShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertTextShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertTextShaderStageInfo.module = vertTextShaderModule;
	vertTextShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragTextShaderStageInfo = {};
	fragTextShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragTextShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragTextShaderStageInfo.module = fragTextShaderModule;
	fragTextShaderStageInfo.pName = "main";

	shaderStages[0] = vertTextShaderStageInfo;
	shaderStages[1] = fragTextShaderStageInfo;

	ColorBlendAttachment.blendEnable = VK_TRUE;
	ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, pipelineCache, 1, &PipelineInfo, nullptr, &textPipeline));

	vkDestroyShaderModule(logicalDevice->device, fragTextShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->device, vertTextShaderModule, nullptr);

	// III. Background pipeline
	auto vertBackgroundShaderCode = enginetool::readFile("puffinEngine/shaders/uiBackgroundShader.vert.spv");
	auto fragBackgroundShaderCode = enginetool::readFile("puffinEngine/shaders/uiBackgroundShader.frag.spv");

	VkShaderModule vertBackgroundShaderModule = logicalDevice->CreateShaderModule(vertBackgroundShaderCode);
	VkShaderModule fragBackgroundShaderModule = logicalDevice->CreateShaderModule(fragBackgroundShaderCode);

	VkPipelineShaderStageCreateInfo vertBackgroundShaderStageInfo = {};
	vertBackgroundShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertBackgroundShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertBackgroundShaderStageInfo.module = vertBackgroundShaderModule;
	vertBackgroundShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragBackgroundShaderStageInfo = {};
	fragBackgroundShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragBackgroundShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragBackgroundShaderStageInfo.module = fragBackgroundShaderModule;
	fragBackgroundShaderStageInfo.pName = "main"; 

	shaderStages[0] = vertBackgroundShaderStageInfo;
	shaderStages[1] = fragBackgroundShaderStageInfo;

	ErrorCheck(vkCreateGraphicsPipelines(logicalDevice->device, pipelineCache, 1, &PipelineInfo, nullptr, &backgroundPipeline));

	vkDestroyShaderModule(logicalDevice->device, fragBackgroundShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice->device, vertBackgroundShaderModule, nullptr); 
}

void GuiMainUi::NewFrame() {
    
}

void GuiMainUi::GetDrawData() {
	CreateHealtBar();

	InfoBox testInfoBox;

	testInfoBox.text.assignedPipeline = &textPipeline;
	testInfoBox.icon.assignedPipeline = &imagePipeline;
	testInfoBox.background.assignedPipeline = &backgroundPipeline;
	testInfoBox.text.assignedTexture = &materialLibrary->font;  
	testInfoBox.icon.assignedTexture = &materialLibrary->icons["cameraIcon"];
	testInfoBox.background.assignedTexture = &materialLibrary->icons["defaultIcon"];
	
	testInfoBox.background.position = glm::vec2(500.0f, 150.0f);
	testInfoBox.background.dimension = glm::vec2(110.0f, 110.0f);
	
	GenerateText(testInfoBox.text, "Puffin Engine", TextAlignment::left,  glm::vec2(20.0f, 300.0f), 100.0f, 700.0f, 1.0f, glm::vec2(0.0f, 0.0f));

	float fbW = (float)logicalDevice->swapchain_extent.width;
	float fbH = (float)logicalDevice->swapchain_extent.height;

	float dimx = testInfoBox.background.dimension.x;
	float dimy = testInfoBox.background.dimension.y;

	int posx = testInfoBox.background.position.x;
	int posy = testInfoBox.background.position.y;

	testInfoBox.background.vertices = {
		{{posx, posy}, 					{1.0f, 0.0f}}, 	// top left
		{{posx + dimx, posy}, 			{0.0f, 0.0f}}, 	// top right
		{{posx + dimx, posy + dimy}, 	{0.0f, 1.0f}}, 	// bottom right
		{{posx, posy + dimy}, 			{1.0f, 1.0f}}	// bottom left
	};

	dimx = testInfoBox.background.dimension.x-10.0f;
	dimy = testInfoBox.background.dimension.y-10.0f;

	posx = testInfoBox.background.position.x+5.0f;
	posy = testInfoBox.background.position.y+5.0f;

	testInfoBox.icon.vertices = {
		{{posx, posy}, 					{1.0f, 0.0f}}, 	// top left
		{{posx + dimx, posy}, 			{0.0f, 0.0f}}, 	// top right
		{{posx + dimx, posy + dimy}, 	{0.0f, 1.0f}}, 	// bottom right
		{{posx, posy + dimy}, 			{1.0f, 1.0f}}	// bottom left
	};



	testInfoBox.icon.indices = { 0, 1, 2, 2, 3, 0 };
	testInfoBox.background.indices = { 0, 1, 2, 2, 3, 0 };

	// float clipX1 = (float)(int)(0.5f + testInfoBox.position.x - 1.0f); //add column offset
	// float clipX2 = (float)(int)(0.5f + testInfoBox.position.x - 1.0f);

	//testInfoBox.icon.clipExtent = glm::vec4(clipX1, std::numeric_limits<float>::lowest(), clipX2, std::numeric_limits<float>::max()); 

	drawData.componentsToDraw.push_back(testInfoBox.background);
	drawData.componentsToDraw.push_back(testInfoBox.icon);
	drawData.componentsToDraw.push_back(testInfoBox.text);
	

	for (int32_t i = 0; i < drawData.componentsToDraw.size(); i++) {
		drawData.totalVerticesCount += drawData.componentsToDraw[i].vertices.size();
		drawData.totalIndicesCount += drawData.componentsToDraw[i].indices.size();
	}
}

void GuiMainUi::CreateHealtBar(){
	HealthBar mainCharacterBar;

	mainCharacterBar.backgroundElementA.assignedPipeline = &backgroundPipeline;
	mainCharacterBar.backgroundElementA.assignedTexture = &materialLibrary->icons["defaultIcon"];
	mainCharacterBar.backgroundElementB.assignedPipeline = &backgroundPipeline;
	mainCharacterBar.backgroundElementB.assignedTexture = &materialLibrary->icons["defaultIcon"];
	mainCharacterBar.backgroundElementC.assignedPipeline = &backgroundPipeline;
	mainCharacterBar.backgroundElementC.assignedTexture = &materialLibrary->icons["defaultIcon"];
	mainCharacterBar.backgroundElementD.assignedPipeline = &backgroundPipeline;
	mainCharacterBar.backgroundElementD.assignedTexture = &materialLibrary->icons["defaultIcon"];

	float WPCNT = (float)logicalDevice->swapchain_extent.width*0.01f;
	float HPCNT = (float)logicalDevice->swapchain_extent.height*0.01f;

	float shapeOffsetX = WPCNT*2;

	float textLenght = WPCNT * 20.0f;
	
	float posAx = WPCNT;
	float posAy = HPCNT;

	float dimAx = WPCNT*8;
	float dimAy = HPCNT*8;

	float posBx = dimAx + posAx + WPCNT;
	float posBy = HPCNT;

	float dimBx = WPCNT*25;
	float dimBy = HPCNT*4;

	float posCx = dimBx + posBx;
	float posCy = HPCNT;

	float dimCx = textLenght;
	float dimCy = HPCNT*4;

	float posDx = dimAx + posAx + WPCNT;
	float posDy = dimBy + posBy;

	float dimDx = WPCNT*25;
	float dimDy = HPCNT*4;


	mainCharacterBar.backgroundElementA.vertices = {
		{{posAx, posAy}, 					{1.0f, 0.0f}}, 	// top left
		{{posAx + dimAx, posAy}, 			{0.0f, 0.0f}}, 	// top right
		{{posAx + dimAx + shapeOffsetX, posAy + dimAy}, 	{0.0f, 1.0f}}, 	// bottom right
		{{posAx, posAy + dimAy}, 			{1.0f, 1.0f}}	// bottom left
	};

	mainCharacterBar.backgroundElementB.vertices = {
		{{posBx, posBy}, 					{1.0f, 0.0f}}, 	// top left
		{{posBx + dimBx, posBy}, 			{0.0f, 0.0f}}, 	// top right
		{{posBx + dimBx, posBy + dimBy}, 	{0.0f, 1.0f}}, 	// bottom right
		{{posBx + shapeOffsetX/2, posBy + dimBy}, 			{1.0f, 1.0f}}	// bottom left
	};

	mainCharacterBar.backgroundElementC.vertices = {
		{{posCx, posCy}, 					{1.0f, 0.0f}}, 	// top left
		{{posCx + dimCx, posCy}, 			{0.0f, 0.0f}}, 	// top right
		{{posCx + dimCx - shapeOffsetX, posCy + dimCy}, 	{0.0f, 1.0f}}, 	// bottom right
		{{posCx, posCy + dimCy}, 			{1.0f, 1.0f}}	// bottom left
	};

	mainCharacterBar.backgroundElementD.vertices = {
		{{posDx + shapeOffsetX/2, posDy}, 					{1.0f, 0.0f}}, 	// top left
		{{posDx + dimDx, posDy}, 			{0.0f, 0.0f}}, 	// top right
		{{posDx + dimDx - shapeOffsetX, posDy + dimDy}, 	{0.0f, 1.0f}}, 	// bottom right
		{{posDx + shapeOffsetX, posDy + dimDy}, 			{1.0f, 1.0f}}	// bottom left
	};

	mainCharacterBar.backgroundElementA.indices = { 0, 1, 2, 2, 3, 0 };
	mainCharacterBar.backgroundElementB.indices = { 0, 1, 2, 2, 3, 0 };
	mainCharacterBar.backgroundElementC.indices = { 0, 1, 2, 2, 3, 0 };
	mainCharacterBar.backgroundElementD.indices = { 0, 1, 2, 2, 3, 0 };
	
	// mainCharacterBar.backgroundElemntB.vertices = {
	// 	{{posx, posy}, 					{1.0f, 0.0f}}, 	// top left
	// 	{{posx + dimx, posy}, 			{0.0f, 0.0f}}, 	// top right
	// 	{{posx + dimx, posy + dimy}, 	{0.0f, 1.0f}}, 	// bottom right
	// 	{{posx, posy + dimy}, 			{1.0f, 1.0f}}	// bottom left
	// };
	
	// mainCharacterBar.backgroundElemntC.vertices = {
	// 	{{posx, posy}, 					{1.0f, 0.0f}}, 	// top left
	// 	{{posx + dimx, posy}, 			{0.0f, 0.0f}}, 	// top right
	// 	{{posx + dimx, posy + dimy}, 	{0.0f, 1.0f}}, 	// bottom right
	// 	{{posx, posy + dimy}, 			{1.0f, 1.0f}}	// bottom left
	// };
	
	// mainCharacterBar.backgroundElemntD.vertices = {
	// 	{{posx, posy}, 					{1.0f, 0.0f}}, 	// top left
	// 	{{posx + dimx, posy}, 			{0.0f, 0.0f}}, 	// top right
	// 	{{posx + dimx, posy + dimy}, 	{0.0f, 1.0f}}, 	// bottom right
	// 	{{posx, posy + dimy}, 			{1.0f, 1.0f}}	// bottom left
	// };



	drawData.componentsToDraw.push_back(mainCharacterBar.backgroundElementA);
	drawData.componentsToDraw.push_back(mainCharacterBar.backgroundElementB);
	drawData.componentsToDraw.push_back(mainCharacterBar.backgroundElementC);
	drawData.componentsToDraw.push_back(mainCharacterBar.backgroundElementD);
}

void GuiMainUi::UpdateDrawData() {
	if (drawData.totalVerticesCount == 0)
		return;
	
	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = drawData.totalVerticesCount * sizeof(Vertex);
	VkDeviceSize indexBufferSize = drawData.totalIndicesCount * sizeof(uint32_t);

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
	 uint32_t* idxDst = (uint32_t*)indexBuffer.mapped;

	 for (int32_t i = 0; i < drawData.componentsToDraw.size(); i++) {
	    memcpy(vtxDst, drawData.componentsToDraw[i].vertices.data(), drawData.componentsToDraw[i].vertices.size() * sizeof(Vertex));
	  	memcpy(idxDst, drawData.componentsToDraw[i].indices.data(), drawData.componentsToDraw[i].indices.size() * sizeof(uint32_t));
	 	vtxDst += drawData.componentsToDraw[i].vertices.size() ;
	 	idxDst += drawData.componentsToDraw[i].indices.size();
	}

	vertexBuffer.Flush();
	indexBuffer.Flush();
}

void GuiMainUi::UpdateFontUniformBuffer() {
	UBOF.proj = glm::mat4(1.0f);
	//UBOF.proj = glm::perspective(glm::radians(45.0f), (float)logicalDevice->swapchain_extent.width / (float)logicalDevice->swapchain_extent.height, 0.001f, 256.0f);
	UBOF.model = glm::mat4(1.0f);
	memcpy(uniformBuffers.fontMatrices.mapped, &UBOF, sizeof(UBOF));	
} 

void GuiMainUi::UpdateFontSettingsUniformBuffer() {
	memcpy(uniformBuffers.fontSettings.mapped, &UBOS, sizeof(UBOS));
}

void GuiMainUi::CreateUniformBuffer(VkCommandBuffer command_buffer) {
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)logicalDevice->swapchain_extent.width;
	viewport.height = (float)logicalDevice->swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	scissor.offset.x = 0.0f;//std::max((int32_t)(drawData.componentsToDraw[i].clipExtent.x),0);
	scissor.offset.y = 0.0f;//std::max((int32_t)(drawData.componentsToDraw[i].clipExtent.y), 0);
	scissor.extent.width = (float)logicalDevice->swapchain_extent.width;//(uint32_t)(drawData.componentsToDraw[i].clipExtent.z - drawData.componentsToDraw[i].clipExtent.x);
	scissor.extent.height = (float)logicalDevice->swapchain_extent.height;//(uint32_t)(drawData.componentsToDraw[i].clipExtent.w - drawData.componentsToDraw[i].clipExtent.y);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	VkDeviceSize offsets[1] = { 0 };

	// UI scale and translate via push constants
	pushConstBlock.frameBufferSize = glm::vec2((float)logicalDevice->swapchain_extent.width/2.0f, (float)logicalDevice->swapchain_extent.height/2.0f);
	vkCmdPushConstants(command_buffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);


	vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertexBuffer.buffer, offsets);
	vkCmdBindIndexBuffer(command_buffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	
	// Render the command lists:
	int32_t vertexOffset = 0;
	int32_t indexOffset = 0;

	for (int32_t i = 0; i < drawData.componentsToDraw.size(); i++) {
		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &drawData.componentsToDraw[i].descriptorSet, 0, nullptr);
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *drawData.componentsToDraw[i].assignedPipeline);
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
	uniformBuffers.fontMatrices.Destroy();
	uniformBuffers.fontSettings.Destroy();
	vkDestroyPipelineCache(logicalDevice->device, pipelineCache, nullptr);
	vkDestroyPipeline(logicalDevice->device, imagePipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, textPipeline, nullptr);
	vkDestroyPipeline(logicalDevice->device, backgroundPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice->device, pipelineLayout, nullptr);
	vkDestroyDescriptorPool(logicalDevice->device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->device, descriptorSetLayout, nullptr);

	logicalDevice = nullptr;
	commandPool = nullptr;
	materialLibrary = nullptr;
	renderPass = nullptr;	
}
