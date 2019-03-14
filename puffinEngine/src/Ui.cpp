#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>

#include "LoadFile.cpp"
#include "Ui.hpp"

//---------- Constructors and dectructors ---------- //

GuiElement::GuiElement() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui - imgui element - created\n";
#endif 
}

GuiElement::~GuiElement() {	
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui - imgui element - destroyed\n";
#endif
}

void GuiElement::Init(Device* device, VkCommandPool& commandPool) {
	logicalDevice = device;
	this->commandPool = &commandPool; 

	SetUp();
	LoadImage();
	CreateDescriptorSetLayout();
	CreateDescriptorPool();
	CreateDescriptorSet();
	CreateGraphicsPipeline();
}

void GuiElement::SetUp() {
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)logicalDevice->swapchain_extent.width, (float)logicalDevice->swapchain_extent.height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding            = ImVec2(6, 4);
    style.WindowRounding           = 0.0f;
    style.FramePadding             = ImVec2(5, 2);
    style.FrameRounding            = 0.0f;
    style.ItemSpacing              = ImVec2(7, 1);
    style.ItemInnerSpacing         = ImVec2(1, 1);
    style.TouchExtraPadding        = ImVec2(0, 0);
    style.IndentSpacing            = 6.0f;
    style.ScrollbarSize            = 12.0f;
    style.ScrollbarRounding        = 16.0f;
    style.GrabMinSize              = 20.0f;
    style.GrabRounding             = 0.0f;
    style.WindowTitleAlign.x = 0.50f;
    style.Colors[ImGuiCol_Border] = ImVec4(0.539f, 0.479f, 0.255f, 0.162f);
    style.FrameBorderSize = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.200f, 0.220f, 0.270f, 0.58f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.200f, 0.220f, 0.270f, 1.0f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.200f, 0.220f, 0.270f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.455f, 0.198f, 0.301f, 0.78f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.455f, 0.198f, 0.301f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.232f, 0.201f, 0.271f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.502f, 0.075f, 0.256f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.200f, 0.220f, 0.270f, 0.75f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.200f, 0.220f, 0.270f, 0.47f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.200f, 0.220f, 0.270f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.455f, 0.198f,0.301f, 0.78f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.455f, 0.198f,0.301f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.455f, 0.198f, 0.301f, 0.86f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.455f, 0.198f, 0.301f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.455f, 0.198f, 0.301f, 0.76f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.455f, 0.198f, 0.301f, 0.86f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.502f, 0.075f, 0.256f, 1.00f);
    style.Colors[ImGuiCol_Column]                = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.455f, 0.198f, 0.301f, 0.78f);
    style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.455f, 0.198f, 0.301f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.455f, 0.198f, 0.301f, 0.78f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.455f, 0.198f, 0.301f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.455f, 0.198f, 0.301f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.455f, 0.198f, 0.301f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.455f, 0.198f, 0.301f, 0.43f);
    style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.200f, 0.220f, 0.270f, 0.73f);
}

void GuiElement::LoadImage() {
	ImGuiIO& io = ImGui::GetIO();

	unsigned char* fontData;
	io.Fonts->GetTexDataAsRGBA32(&fontData, (int*)&font.texWidth, (int*)&font.texHeight);
	
	VkDeviceSize uploadSize = font.texWidth * font.texHeight * 4 * sizeof(char);
	enginetool::Buffer<void> stagingBuffer;
	stagingBuffer.CreateBuffer(logicalDevice, uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, fontData);
	
	font.Init(logicalDevice, *commandPool, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 1);
	font.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	font.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	font.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);

	font.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	font.CopyBufferToImage(stagingBuffer.buffer);
	stagingBuffer.Destroy();
	font.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Store our identifier
	io.Fonts->TexID = (void *)(intptr_t)font.texture;	
}

void GuiElement::CreateDescriptorSetLayout() {
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


void GuiElement::CreateDescriptorPool() {
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

void GuiElement::CreateDescriptorSet() {
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

void GuiElement::CreateGraphicsPipeline() {
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
	Rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
	vertex_bindings[0].stride = sizeof(ImDrawVert);
	vertex_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 3> vertex_attributes = {};
	vertex_attributes[0].binding = 0;
	vertex_attributes[0].location = 0;
	vertex_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_attributes[0].offset = offsetof(ImDrawVert, pos);

	vertex_attributes[1].binding = 0;
	vertex_attributes[1].location = 1;
	vertex_attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_attributes[1].offset = offsetof(ImDrawVert, uv);

	vertex_attributes[2].binding = 0;
	vertex_attributes[2].location = 2;
	vertex_attributes[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	vertex_attributes[2].offset = offsetof(ImDrawVert, col); 

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

void GuiElement::NewFrame() {
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	ImGui::Render();
}

void GuiElement::RenderDrawData() {
	ImDrawData* draw_data = ImGui::GetDrawData();

	if (draw_data->TotalVtxCount == 0)
		return;

	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = draw_data->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

	// Update buffers only if vertex or index count has been changed compared to current buffer size
	if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != draw_data->TotalVtxCount)) {
		vertexBuffer.Unmap();
		vertexBuffer.Destroy();

		vertexBuffer.CreateBuffer(logicalDevice, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr);
		vertexCount = draw_data->TotalVtxCount;
		vertexBuffer.Unmap();
		vertexBuffer.Map();
	}

	if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < draw_data->TotalIdxCount)) {
		indexBuffer.Unmap();
		indexBuffer.Destroy();

		indexBuffer.CreateBuffer(logicalDevice, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr);
		indexCount = draw_data->TotalIdxCount;
		indexBuffer.Unmap();
		indexBuffer.Map();
	}

	// Upload data
	ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffer.mapped;
	ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffer.mapped;

	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	vertexBuffer.Flush();
	indexBuffer.Flush();
}

void GuiElement::CreateUniformBuffer(VkCommandBuffer command_buffer) {
	ImDrawData* draw_data = ImGui::GetDrawData();

	if (draw_data->TotalVtxCount == 0)
		return;

	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = draw_data->DisplaySize.x;
	viewport.height = draw_data->DisplaySize.y;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	// UI scale and translate via push constants
	pushConstBlock.scale = glm::vec2(2.0f / draw_data->DisplaySize.x, 2.0f / draw_data->DisplaySize.y);
	pushConstBlock.translate = glm::vec2(-1.0f - draw_data->DisplayPos.x * pushConstBlock.scale.x, -1.0f - draw_data->DisplayPos.y * pushConstBlock.scale.y);
	vkCmdPushConstants(command_buffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

	VkDeviceSize offsets[1] = { 0 };

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertexBuffer.buffer, offsets);
	vkCmdBindIndexBuffer(command_buffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

	// Render the command lists:
	int32_t vertex_offset = 0;
	int32_t index_offset = 0;

	for (int32_t i = 0; i < draw_data->CmdListsCount; i++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[i];
		for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
			if (pcmd->UserCallback)	{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else {
				scissor.offset.x = std::max((int32_t)(pcmd->ClipRect.x - draw_data->DisplayPos.x), 0);
				scissor.offset.y = std::max((int32_t)(pcmd->ClipRect.y - draw_data->DisplayPos.y), 0);
				scissor.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissor.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
				vkCmdSetScissor(command_buffer, 0, 1, &scissor);

				// Draw
				vkCmdDrawIndexed(command_buffer, pcmd->ElemCount, 1, index_offset, vertex_offset, 0);
			}
			index_offset += pcmd->ElemCount;
		}
		vertex_offset += cmd_list->VtxBuffer.Size;
	}
}

void GuiElement::DeInit() {
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