#include <iostream>
#include <filesystem>

#include <array>
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

void GuiTextOverlay::init(Device* device, VkCommandPool& commandPool) {
	p_LogicalDevice = device;
	this->p_CommandPool = &commandPool;

	m_VertexBuffer.setDevice(device);
	p_Mapped = (glm::vec4*)m_VertexBuffer.getMapped();
	
	loadFontImage();
	createDescriptorSetLayout();
	createDescriptorPool();
	createDescriptorSet();
	createGraphicsPipeline();
}

void GuiTextOverlay::loadFontImage() {
	static unsigned char font24pixels[STB_FONT_HEIGHT][STB_FONT_WIDTH];
	STB_FONT_NAME(m_StbFontData, font24pixels, STB_FONT_HEIGHT);

	m_Font.Init(p_LogicalDevice, *p_CommandPool, VK_FORMAT_R8_UNORM, 0, 1, 1);
	m_Font.texWidth = STB_FONT_WIDTH;
	m_Font.texHeight = STB_FONT_HEIGHT;
	m_Font.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	m_Font.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	m_Font.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);

	VkDeviceSize uploadSize = STB_FONT_WIDTH * STB_FONT_HEIGHT * sizeof(int);
	enginetool::Buffer stagingBuffer;
	stagingBuffer.setDevice(p_LogicalDevice);
	stagingBuffer.createStagedBuffer(uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_StbFontData);

	stagingBuffer.map(uploadSize, 0);
	stagingBuffer.copy(STB_FONT_WIDTH * STB_FONT_HEIGHT, &font24pixels[0][0]);
	stagingBuffer.unmap();

	m_Font.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_Font.CopyBufferToImage(stagingBuffer.getBuffer());
	stagingBuffer.destroy();
	m_Font.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDeviceSize vertexBufferSize = TEXTOVERLAY_MAX_CHAR_COUNT * sizeof(glm::vec4);
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

	ErrorCheck(vkCreateDescriptorSetLayout(p_LogicalDevice->get(), &SceneObjectsLayoutInfo, nullptr, &m_DescriptorSetLayout));
}

void GuiTextOverlay::createDescriptorPool() {
	// Don't forget to rise this numbers when you add bindings
	std::array<VkDescriptorPoolSize, 1> PoolSizes = {};
	PoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	PoolSizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo PoolInfo = {};
	PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.maxSets = 1; // maximum number of descriptor sets that will be allocated

	ErrorCheck(vkCreateDescriptorPool(p_LogicalDevice->get(), &PoolInfo, nullptr, &m_DescriptorPool));
}

void GuiTextOverlay::createDescriptorSet() {
		VkDescriptorSetAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfo.descriptorPool = m_DescriptorPool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &m_DescriptorSetLayout;

		ErrorCheck(vkAllocateDescriptorSets(p_LogicalDevice->get(), &AllocInfo, &m_DescriptorSet));

		VkDescriptorImageInfo ImageInfo = {};
		ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageInfo.imageView = m_Font.view;
		ImageInfo.sampler = m_Font.sampler;

		std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_DescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &ImageInfo;

		vkUpdateDescriptorSets(p_LogicalDevice->get(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void GuiTextOverlay::createGraphicsPipeline() {
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	ErrorCheck(vkCreatePipelineCache(p_LogicalDevice->get(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));

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
	ViewportState.pViewports = &m_Viewport;
	ViewportState.scissorCount = 1;
	ViewportState.pScissors = &m_Scissor;

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

	std::array<VkDescriptorSetLayout, 1> layouts = {m_DescriptorSetLayout};

	VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
	PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	PipelineLayoutInfo.pSetLayouts = layouts.data();
	
	ErrorCheck(vkCreatePipelineLayout(p_LogicalDevice->get(), &PipelineLayoutInfo, nullptr, &m_PipelineLayout));

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
	PipelineInfo.layout = m_PipelineLayout;
	PipelineInfo.renderPass = p_LogicalDevice->renderPass;
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	std::filesystem::path p = std::filesystem::current_path().parent_path();
	std::filesystem::path vertModelsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "perform_stats.vert.spv";
	std::filesystem::path fragModelsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "perform_stats.frag.spv";
	
	auto vertModelsShaderCode = enginetool::readFile(vertModelsShaderCodePath.string());
	auto fragModelsShaderCode = enginetool::readFile(fragModelsShaderCodePath.string());

	VkShaderModule vertModelsShaderModule = p_LogicalDevice->CreateShaderModule(vertModelsShaderCode);
	VkShaderModule fragModelsShaderModule = p_LogicalDevice->CreateShaderModule(fragModelsShaderCode);

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

	ErrorCheck(vkCreateGraphicsPipelines(p_LogicalDevice->get(), m_PipelineCache, 1, &PipelineInfo, nullptr, &m_Pipeline));

	vkDestroyShaderModule(p_LogicalDevice->get(), fragModelsShaderModule, nullptr);
	vkDestroyShaderModule(p_LogicalDevice->get(), vertModelsShaderModule, nullptr);
}

void GuiTextOverlay::beginTextUpdate() {
	ErrorCheck(vkMapMemory(p_LogicalDevice->get(), m_VertexBuffer.getMemory(), 0, VK_WHOLE_SIZE, 0, (void **)&p_Mapped));
	m_NumLetters = 0;
}

void GuiTextOverlay::renderText(std::string text, float x, float y, TextAlignment align) {
	assert(p_Mapped != nullptr);

	float fbW = (float)p_LogicalDevice->swapchain_extent.width;
	float fbH = (float)p_LogicalDevice->swapchain_extent.height;

	const float charW = 1.5f / fbW;
	const float charH = 1.5f / fbH;

	x = (x / fbW * 2.0f) - 1.0f;
	y = (y / fbH * 2.0f) - 1.0f;

	// Calculate text width
	float textWidth = 0;
	for (auto letter : text) {
		stb_fontchar *charData = &m_StbFontData[(uint32_t)letter - STB_FIRST_CHAR];
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
		stb_fontchar *charData = &m_StbFontData[(uint32_t)letter - STB_FIRST_CHAR];

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

void GuiTextOverlay::createUniformBuffer(const VkCommandBuffer& command_buffer) {
	m_Viewport.x = 0.0f;
	m_Viewport.y = 0.0f;
	m_Viewport.width = (float)p_LogicalDevice->swapchain_extent.width;
	m_Viewport.height = (float)p_LogicalDevice->swapchain_extent.height;
	m_Viewport.minDepth = 0.0f;
	m_Viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &m_Viewport);

	m_Scissor.offset = { 0, 0 }; // scissor rectangle covers framebuffer entirely
	m_Scissor.extent = p_LogicalDevice->swapchain_extent;	
	vkCmdSetScissor(command_buffer, 0, 1, &m_Scissor);
	
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &m_VertexBuffer.getBuffer(), offsets);
	vkCmdBindVertexBuffers(command_buffer, 1, 1, &m_VertexBuffer.getBuffer(), offsets);

	for (uint32_t j = 0; j < m_NumLetters; j++) {
		vkCmdDraw(command_buffer, 4, 1, j * 4, 0);
	}
}

void GuiTextOverlay::deInit() {
	m_VertexBuffer.destroy();
	m_Font.DeInit();
	vkDestroyPipelineCache(p_LogicalDevice->get(), m_PipelineCache, nullptr);
	vkDestroyPipeline(p_LogicalDevice->get(), m_Pipeline, nullptr);
	vkDestroyPipelineLayout(p_LogicalDevice->get(), m_PipelineLayout, nullptr);
	vkDestroyDescriptorPool(p_LogicalDevice->get(), m_DescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(p_LogicalDevice->get(), m_DescriptorSetLayout, nullptr);

	p_LogicalDevice = nullptr;
	p_CommandPool = nullptr;
}
