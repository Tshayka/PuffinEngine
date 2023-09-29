#include <iostream>
#include <filesystem>

#include <array>
#include "LoadFile.cpp"
#include "headers/GuiTextOverlay.hpp"

using namespace puffinengine::tool;

#define TEXTOVERLAY_MAX_CHAR_COUNT 2048

//---------- Constructors and dectructors ---------- //

GuiTextOverlay::GuiTextOverlay() {
#if DEBUG_VERSION
	std::cout << "Gui - text overlay - created\n";
#endif 
}

GuiTextOverlay::~GuiTextOverlay() {
#if DEBUG_VERSION
	std::cout << "Gui - text overlay - destroyed\n";
#endif
}

// --------------- Setters and getters -------------- //



// ---------------- Main functions ------------------ //

void GuiTextOverlay::init(Device* device, RenderPass* renderPass, VkCommandPool* commandPool) {
	p_Device = device;
	p_RenderPass = renderPass;
	p_CommandPool = commandPool;
	p_Mapped = static_cast<glm::vec4*>(m_VertexBuffer.getMapped());
	
	loadFontImage();
	createDescriptorSetLayout();
	createDescriptorPool();
	createDescriptorSet();
	createGraphicsPipeline();
}

void GuiTextOverlay::loadFontImage() {
	static unsigned char font24pixels[STB_FONT_HEIGHT][STB_FONT_WIDTH];
	STB_FONT_NAME(m_StbFontData, font24pixels, STB_FONT_HEIGHT);

	m_Font.Init(p_Device, *p_CommandPool, VK_FORMAT_R8_UNORM, 0, 1, 1);
	m_Font.texWidth = STB_FONT_WIDTH;
	m_Font.texHeight = STB_FONT_HEIGHT;
	m_Font.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	m_Font.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	m_Font.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);

	VkDeviceSize uploadSize = STB_FONT_WIDTH * STB_FONT_HEIGHT * sizeof(int);
	enginetool::Buffer stagingBuffer;
	stagingBuffer.setDevice(p_Device);
	stagingBuffer.createStagedBuffer(uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_StbFontData);

	stagingBuffer.map(uploadSize, 0);
	stagingBuffer.copy(STB_FONT_WIDTH * STB_FONT_HEIGHT, &font24pixels[0][0]);
	stagingBuffer.unmap();

	m_Font.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_Font.CopyBufferToImage(stagingBuffer.getBuffer());
	stagingBuffer.destroy();
	m_Font.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDeviceSize vertexBufferSize = TEXTOVERLAY_MAX_CHAR_COUNT * sizeof(glm::vec4);
	m_VertexBuffer.setDevice(p_Device);
	m_VertexBuffer.createUnstagedBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void GuiTextOverlay::createDescriptorSetLayout() {
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

	ErrorCheck(vkCreateDescriptorSetLayout(p_Device->get(), &SceneObjectsLayoutInfo, nullptr, &m_DescriptorSetLayout));
}

void GuiTextOverlay::createDescriptorPool() {
	// Don't forget to rise this numbers when you add bindings
	std::array<VkDescriptorPoolSize, 1> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 1; // maximum number of descriptor sets that will be allocated

	ErrorCheck(vkCreateDescriptorPool(p_Device->get(), &poolInfo, nullptr, &m_DescriptorPool));
}

void GuiTextOverlay::createDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_DescriptorSetLayout;

		ErrorCheck(vkAllocateDescriptorSets(p_Device->get(), &allocInfo, &m_DescriptorSet));

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_Font.view;
		imageInfo.sampler = m_Font.sampler;

		std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_DescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(p_Device->get(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void GuiTextOverlay::createGraphicsPipeline() {
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	ErrorCheck(vkCreatePipelineCache(p_Device->get(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {}; // describes what kind of geometry will be drawn from the vertices and if primitive restart should be enabled
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; // triangle from every 3 vertices with reuse
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterization = {}; // takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.depthClampEnable = VK_FALSE;
	rasterization.rasterizerDiscardEnable = VK_FALSE; // geometry never passes through the rasterizer stage
	rasterization.polygonMode = VK_POLYGON_MODE_FILL; // determines how fragments are generated for geometry
	rasterization.lineWidth = 1.0f;
	rasterization.cullMode = VK_CULL_MODE_NONE;
	rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization.depthBiasEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; // Enable blending
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &m_Viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &m_Scissor;

	VkPipelineMultisampleStateCreateInfo multisample = {}; // configures multisampling, is one of the ways to perform anti-aliasing
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.sampleShadingEnable = VK_FALSE;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo viewportDynamic = {};
	viewportDynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	viewportDynamic.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	viewportDynamic.pDynamicStates = dynamicStateEnables.data();

	std::array<VkVertexInputBindingDescription, 2> vertexBindings = {};
	vertexBindings[0].binding = 0;
	vertexBindings[0].stride = sizeof(glm::vec4);
	vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	vertexBindings[1].binding = 1;
	vertexBindings[1].stride = sizeof(glm::vec4);
	vertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 2> vertexAttributes = {};
	vertexAttributes[0].binding = 0;
	vertexAttributes[0].location = 0;
	vertexAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributes[0].offset = 0;
	
	vertexAttributes[1].binding = 1;
	vertexAttributes[1].location = 1;
	vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributes[1].offset = sizeof(glm::vec2);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size());
	vertexInputInfo.pVertexBindingDescriptions = vertexBindings.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

	std::array<VkDescriptorSetLayout, 1> layouts = {m_DescriptorSetLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	
	ErrorCheck(vkCreatePipelineLayout(p_Device->get(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));

	std::array <VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;	
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterization;
	pipelineInfo.pMultisampleState = &multisample;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &viewportDynamic;	
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.renderPass = p_RenderPass->get();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	std::filesystem::path p = std::filesystem::current_path().parent_path();
	std::filesystem::path vertModelsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "perform_stats.vert.spv";
	std::filesystem::path fragModelsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "perform_stats.frag.spv";
	
	auto vertModelsShaderCode = enginetool::readFile(vertModelsShaderCodePath.string());
	auto fragModelsShaderCode = enginetool::readFile(fragModelsShaderCodePath.string());

	VkShaderModule vertModelsShaderModule = p_Device->CreateShaderModule(vertModelsShaderCode);
	VkShaderModule fragModelsShaderModule = p_Device->CreateShaderModule(fragModelsShaderCode);

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

	ErrorCheck(vkCreateGraphicsPipelines(p_Device->get(), m_PipelineCache, 1, &pipelineInfo, nullptr, &m_Pipeline));

	vkDestroyShaderModule(p_Device->get(), fragModelsShaderModule, nullptr);
	vkDestroyShaderModule(p_Device->get(), vertModelsShaderModule, nullptr);
}

void GuiTextOverlay::beginTextUpdate() {
	ErrorCheck(vkMapMemory(p_Device->get(), m_VertexBuffer.getMemory(), 0, VK_WHOLE_SIZE, 0, (void **)&p_Mapped));
	m_NumLetters = 0;
}

void GuiTextOverlay::renderText(const std::string& text, float x, float y, const TextAlignment& align) {
	assert(p_Mapped != nullptr); // try-catch

	float fbW = static_cast<float>(p_Device->swapchain_extent.width);
	float fbH = static_cast<float>(p_Device->swapchain_extent.height);

	const float charW = 1.5f / fbW;
	const float charH = 1.5f / fbH;

	x = (x / fbW * 2.0f) - 1.0f;
	y = (y / fbH * 2.0f) - 1.0f;

	// Calculate text width
	float textWidth = 0;
	for (auto letter : text) {
		stb_fontchar *charData = &m_StbFontData[static_cast<uint32_t>(letter) - STB_FIRST_CHAR];
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
	for (const auto &letter : text) {
		stb_fontchar *charData = &m_StbFontData[static_cast<uint32_t>(letter) - STB_FIRST_CHAR];

		p_Mapped->x = (x + (float)charData->x0 * charW);
		p_Mapped->y = (y + (float)charData->y0 * charH);
		p_Mapped->z = charData->s0;
		p_Mapped->w = charData->t0;
		p_Mapped++;

		p_Mapped->x = (x + (float)charData->x1 * charW);
		p_Mapped->y = (y + (float)charData->y0 * charH);
		p_Mapped->z = charData->s1;
		p_Mapped->w = charData->t0;
		p_Mapped++;

		p_Mapped->x = (x + (float)charData->x0 * charW);
		p_Mapped->y = (y + (float)charData->y1 * charH);
		p_Mapped->z = charData->s0;
		p_Mapped->w = charData->t1;
		p_Mapped++;

		p_Mapped->x = (x + (float)charData->x1 * charW);
		p_Mapped->y = (y + (float)charData->y1 * charH);
		p_Mapped->z = charData->s1;
		p_Mapped->w = charData->t1;
		p_Mapped++;

		x += charData->advance * charW;

		m_NumLetters++;
	}
}

// Unmap buffer and update command buffers
void GuiTextOverlay::endTextUpdate() {
	m_VertexBuffer.unmap();
}

void GuiTextOverlay::createUniformBuffer(const VkCommandBuffer& commandBuffer) {
	m_Viewport.x = 0.0f;
	m_Viewport.y = 0.0f;
	m_Viewport.width = static_cast<float>(p_Device->swapchain_extent.width);
	m_Viewport.height = static_cast<float>(p_Device->swapchain_extent.height);
	m_Viewport.minDepth = 0.0f;
	m_Viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &m_Viewport);

	m_Scissor.offset = { 0, 0 }; // scissor rectangle covers framebuffer entirely
	m_Scissor.extent = p_Device->swapchain_extent;	
	vkCmdSetScissor(commandBuffer, 0, 1, &m_Scissor);
	
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_VertexBuffer.getBuffer(), offsets);
	vkCmdBindVertexBuffers(commandBuffer, 1, 1, &m_VertexBuffer.getBuffer(), offsets);

	for (uint32_t j = 0; j < m_NumLetters; j++) {
		vkCmdDraw(commandBuffer, 4, 1, j * 4, 0);
	}
}

void GuiTextOverlay::cleanUpForSwapchain() {
	destroyPipeline();
}

void GuiTextOverlay::recreateForSwapchain() {
	createGraphicsPipeline();
}

void GuiTextOverlay::destroyPipeline(){
	vkDestroyPipelineCache(p_Device->get(), m_PipelineCache, nullptr);
	vkDestroyPipeline(p_Device->get(), m_Pipeline, nullptr);
	vkDestroyPipelineLayout(p_Device->get(), m_PipelineLayout, nullptr);
}

void GuiTextOverlay::deInit() {
	m_VertexBuffer.destroy();
	m_Font.DeInit();
	vkDestroyDescriptorPool(p_Device->get(), m_DescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(p_Device->get(), m_DescriptorSetLayout, nullptr);

	p_Device = nullptr;
	p_CommandPool = nullptr;
}
