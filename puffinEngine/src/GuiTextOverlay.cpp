#include <iostream>

#include "LoadFile.cpp"
#include "GuiTextOverlay.hpp"

#define TEXTOVERLAY_MAX_CHAR_COUNT 2048

//---------- Constructors and dectructors ---------- //

GuiTextOverlay::GuiTextOverlay() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Text overlay created\n";
#endif 
}

GuiTextOverlay::~GuiTextOverlay() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Text overlay destroyed\n";
#endif
}

// --------------- Setters and getters -------------- //



// ---------------- Main functions ------------------ //

void GuiTextOverlay::Init(Device* device) {
	logical_device = device; 
	
	SetUp();
	CreateCommandPool();
	InitResources();
	CreateViewAndSampler();
	CreateDescriptorSetLayout();
	CreateDescriptorPool();
	CreateDescriptorSet();
	CreateGraphicsPipeline();
}

void GuiTextOverlay::SetUp(){
	STB_FONT_NAME(stbFontData, font24pixels, STB_FONT_HEIGHT);
}

void GuiTextOverlay::CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = logical_device->FindQueueFamilies(logical_device->gpu);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow command buffers to be rerecorded individually, optional

	ErrorCheck(vkCreateCommandPool(logical_device->device, &poolInfo, nullptr, &command_pool));
}

VkCommandBuffer GuiTextOverlay::BeginSingleTimeCommands() {
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

void GuiTextOverlay::EndSingleTimeCommands(VkCommandBuffer command_buffer) {
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &command_buffer;

	vkQueueSubmit(logical_device->queue, 1, &SubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(logical_device->queue);

	vkFreeCommandBuffers(logical_device->device, command_pool, 1, &command_buffer);
}

void GuiTextOverlay::InitResources() {
	VkDeviceSize textoverlay_buffer_size = TEXTOVERLAY_MAX_CHAR_COUNT * sizeof(glm::vec4);
	
	logical_device->CreateBuffer(textoverlay_buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, memory);
	
	VkImageCreateInfo ImageInfo = {};
	ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageInfo.format = VK_FORMAT_R8_UNORM;
	ImageInfo.extent.width = STB_FONT_WIDTH;
	ImageInfo.extent.height = STB_FONT_HEIGHT;
	ImageInfo.extent.depth = 1;
	ImageInfo.mipLevels = 1;
	ImageInfo.arrayLayers = 1;
	ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	ErrorCheck(vkCreateImage(logical_device->device, &ImageInfo, nullptr, &image));

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(logical_device->device, image, &memory_requirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memory_requirements.size;
	allocInfo.memoryTypeIndex = logical_device->FindMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


	if (vkAllocateMemory(logical_device->device, &allocInfo, nullptr, &image_memory) != VK_SUCCESS)	{
		assert(0 && "Vulkan ERROR: failed to allocate image memory!");
		std::exit(-1);
	}

	ErrorCheck(vkBindImageMemory(logical_device->device, image, image_memory, 0));

	enginetool::Buffer staging_buffer;

	logical_device->CreateStagedBuffer(allocInfo.allocationSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, staging_buffer.memory);

	//vkGetBufferMemoryRequirements(logical_device->device, textoverlay_staging_buffer, &memory_requirements);

	staging_buffer.Map(allocInfo.allocationSize, 0);
	staging_buffer.CopyTo(&font24pixels[0][0], STB_FONT_WIDTH * STB_FONT_HEIGHT);
	staging_buffer.Unmap();

	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	VkBufferImageCopy Region = {};
	Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	Region.imageSubresource.mipLevel = 0;
	Region.imageSubresource.baseArrayLayer = 0;
	Region.imageSubresource.layerCount = 1;
	Region.imageExtent.width = STB_FONT_WIDTH;
	Region.imageExtent.height = STB_FONT_HEIGHT;
	Region.imageExtent.depth = 1;
	Region.bufferOffset = 0;

	//TransitionImageLayout(image, VK_FORMAT_R8_UNORM, , );
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkCmdCopyBufferToImage(command_buffer, staging_buffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	EndSingleTimeCommands(command_buffer);
	
	staging_buffer.Destroy();
}

void GuiTextOverlay::CreateViewAndSampler() {
	VkImageViewCreateInfo ViewInfo = {};
	ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ViewInfo.image = image;
	ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	ViewInfo.format = VK_FORMAT_R8_UNORM;
	ViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,	VK_COMPONENT_SWIZZLE_A };
	ViewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	if (vkCreateImageView(logical_device->device, &ViewInfo, nullptr, &image_view) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to create texture image view for status overlay!");
		std::exit(-1);
	}

	VkSamplerCreateInfo SamplerInfo = {};
	SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	SamplerInfo.magFilter = VK_FILTER_LINEAR;
	SamplerInfo.minFilter = VK_FILTER_LINEAR;
	SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	SamplerInfo.mipLodBias = 0.0f;
	SamplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	SamplerInfo.minLod = 0.0f;
	SamplerInfo.maxLod = 1.0f;
	SamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	if (vkCreateSampler(logical_device->device, &SamplerInfo, nullptr, &texture_sampler) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to create texture sampler for status overlay!");
		std::exit(-1);
	}
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

	ErrorCheck(vkCreateDescriptorSetLayout(logical_device->device, &SceneObjectsLayoutInfo, nullptr, &descriptor_set_layout));
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

	ErrorCheck(vkCreateDescriptorPool(logical_device->device, &PoolInfo, nullptr, &descriptor_pool));
}

void GuiTextOverlay::CreateDescriptorSet() {
		VkDescriptorSetAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfo.descriptorPool = descriptor_pool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &descriptor_set_layout;

		ErrorCheck(vkAllocateDescriptorSets(logical_device->device, &AllocInfo, &descriptor_set));

		VkDescriptorImageInfo ImageInfo = {};
		ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageInfo.imageView = image_view;
		ImageInfo.sampler = texture_sampler;

		std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptor_set;
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
	ErrorCheck(vkCreatePipelineCache(logical_device->device, &pipelineCacheCreateInfo, nullptr, &pipeline_cache));

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

	std::array<VkDescriptorSetLayout, 1> layouts = {descriptor_set_layout};

	VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
	PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	PipelineLayoutInfo.pSetLayouts = layouts.data();
	
	ErrorCheck(vkCreatePipelineLayout(logical_device->device, &PipelineLayoutInfo, nullptr, &pipeline_layout));

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
	PipelineInfo.layout = pipeline_layout;
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

	ErrorCheck(vkCreateGraphicsPipelines(logical_device->device, pipeline_cache, 1, &PipelineInfo, nullptr, &text_overlay_pipeline));

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
	
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, text_overlay_pipeline);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, offsets);
	vkCmdBindVertexBuffers(command_buffer, 1, 1, &buffer, offsets);

	for (uint32_t j = 0; j < num_letters; j++) {
		vkCmdDraw(command_buffer, 4, 1, j * 4, 0);
	}
}

void GuiTextOverlay::DeInit() {
	vkDestroyBuffer(logical_device->device, buffer, nullptr);
	vkDestroyImage(logical_device->device, image, nullptr);
	vkDestroyImageView(logical_device->device, image_view, nullptr);
	vkFreeMemory(logical_device->device, memory, nullptr);
	vkFreeMemory(logical_device->device, image_memory, nullptr);
	vkDestroySampler(logical_device->device, texture_sampler, nullptr);
	vkDestroyPipelineCache(logical_device->device, pipeline_cache, nullptr);
	vkDestroyPipeline(logical_device->device, text_overlay_pipeline, nullptr);
	vkDestroyPipelineLayout(logical_device->device, pipeline_layout, nullptr);
	vkDestroyCommandPool(logical_device->device, command_pool, nullptr);
	vkDestroyDescriptorPool(logical_device->device, descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(logical_device->device, descriptor_set_layout, nullptr);

	mapped = nullptr;
}
