#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "LoadFile.cpp"
#include "headers/GuiMainUi.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#endif


//---------- Constructors and dectructors ---------- //

GuiMainUi::GuiMainUi() {
#if DEBUG_VERSION
	std::cout << "Gui - main ui - created\n";
#endif 
}

GuiMainUi::~GuiMainUi() {	
#if DEBUG_VERSION
	std::cout << "Gui - main ui - destroyed\n";
#endif
}

void GuiMainUi::init(Device* device, VkCommandPool* commandPool) {
	p_LogicalDevice = device;
	p_CommandPool = commandPool; 

	m_DrawData.totalIndicesCount = 0;
	m_DrawData.totalVerticesCount = 0;

	m_VertexBuffer.setDevice(device);
	m_IndexBuffer.setDevice(device);

	setUp();
	loadImGuiImage();
	createDescriptorSetLayout();
	createDescriptorPool();
	createDescriptorSet();
	createGraphicsPipeline();
}

void GuiMainUi::setUp() {
	getDrawData();
}

void GuiMainUi::loadImGuiImage() {
    ImGuiIO& io = ImGui::GetIO();

	unsigned char* fontData;
	io.Fonts->GetTexDataAsRGBA32(&fontData, (int*)&m_Font.texWidth, (int*)&m_Font.texHeight);
	
	VkDeviceSize imageSize = m_Font.texWidth * m_Font.texHeight * 4 * sizeof(char);
	enginetool::Buffer stagingBuffer;
	stagingBuffer.setDevice(p_LogicalDevice);
	stagingBuffer.createStagedBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, fontData);
	
	m_Font.Init(p_LogicalDevice, *p_CommandPool, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 1);
	m_Font.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	m_Font.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	m_Font.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);
	
	m_Font.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_Font.CopyBufferToImage(stagingBuffer.getBuffer());
	stagingBuffer.destroy();
	m_Font.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GuiMainUi::createDescriptorSetLayout() {
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

	ErrorCheck(vkCreateDescriptorSetLayout(p_LogicalDevice->get(), &SceneObjectsLayoutInfo, nullptr, &m_DescriptorSetLayout));
}


void GuiMainUi::createDescriptorPool() {
	// Don't forget to rise this numbers when you add bindings
	std::array<VkDescriptorPoolSize, 1> PoolSizes = {};
	PoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	PoolSizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo PoolInfo = {};
	PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.maxSets = 2; // maximum number of descriptor sets that will be allocated

	ErrorCheck(vkCreateDescriptorPool(p_LogicalDevice->get(), &PoolInfo, nullptr, &m_DescriptorPool));
}

void GuiMainUi::createDescriptorSet() {
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

	std::array<VkWriteDescriptorSet, 1> WriteDescriptorSets = {};

	WriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	WriteDescriptorSets[0].dstSet = m_DescriptorSet;
	WriteDescriptorSets[0].dstBinding = 0;
	WriteDescriptorSets[0].dstArrayElement = 0;
	WriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	WriteDescriptorSets[0].descriptorCount = 1;
	WriteDescriptorSets[0].pImageInfo = &ImageInfo;

	vkUpdateDescriptorSets(p_LogicalDevice->get(), static_cast<uint32_t>(WriteDescriptorSets.size()), WriteDescriptorSets.data(), 0, nullptr);
}

void GuiMainUi::createGraphicsPipeline() {
	VkPipelineCacheCreateInfo PipelineCacheCreateInfo = {};
	PipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	ErrorCheck(vkCreatePipelineCache(p_LogicalDevice->get(), &PipelineCacheCreateInfo, nullptr, &m_PipelineCache));

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
	
	std::array<VkDescriptorSetLayout, 1> layouts = { m_DescriptorSetLayout };

	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
	PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstantRange;
	PipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());;
	PipelineLayoutCreateInfo.pSetLayouts = layouts.data();
	
	ErrorCheck(vkCreatePipelineLayout(p_LogicalDevice->get(), &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

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
	std::filesystem::path vertModelsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "imgui_menu_shader.vert.spv";
	std::filesystem::path fragModelsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "imgui_menu_shader.frag.spv";

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

void GuiMainUi::newFrame() {
    
}

void GuiMainUi::getDrawData() {
     
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

    m_DrawData.componentsToDraw.push_back(rectangle);

	for (int32_t i = 0; i < m_DrawData.componentsToDraw.size(); i++)
    {
        m_DrawData.totalVerticesCount += (int)m_DrawData.componentsToDraw[i].vertices.size();
        m_DrawData.totalIndicesCount += (int)m_DrawData.componentsToDraw[i].indices.size();
    }
}

void GuiMainUi::updateDrawData() {
	if (m_DrawData.totalVerticesCount == 0) {
		return;
	}
	
	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = m_DrawData.totalVerticesCount * sizeof(Vertex);
	VkDeviceSize indexBufferSize = m_DrawData.totalIndicesCount * sizeof(uint16_t);

	// Update buffers only if vertex or index count has been changed compared to current buffer size

	// Vertex buffer
	if (m_VertexCount != m_DrawData.totalVerticesCount) {
		if (m_VertexBuffer.getBuffer() == VK_NULL_HANDLE) {
			m_VertexBuffer.unmap();
			m_VertexBuffer.destroy();
		}

		m_VertexBuffer.createUnstagedBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		m_VertexCount = m_DrawData.totalVerticesCount;
		m_VertexBuffer.unmap();
		m_VertexBuffer.map(vertexBufferSize);
	}

	// Index buffer
	//VkDeviceSize indexSize = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
	if (m_IndexCount < m_DrawData.totalIndicesCount) {
		if (m_IndexBuffer.getBuffer() == VK_NULL_HANDLE) {
			m_IndexBuffer.unmap();
			m_IndexBuffer.destroy();
		}

		m_IndexBuffer.createUnstagedBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		m_IndexCount = m_DrawData.totalIndicesCount;
		m_IndexBuffer.map(indexBufferSize);
	}

	 Vertex* vtxDst = (Vertex*)m_VertexBuffer.getMapped();
	 uint16_t* idxDst = (uint16_t*)m_IndexBuffer.getMapped();

	 for (int32_t i = 0; i < m_DrawData.componentsToDraw.size(); i++) {
	    memcpy(vtxDst, m_DrawData.componentsToDraw[i].vertices.data(), m_DrawData.componentsToDraw[i].vertices.size() * sizeof(Vertex));
	  	memcpy(idxDst, m_DrawData.componentsToDraw[i].indices.data(), m_DrawData.componentsToDraw[i].indices.size() * sizeof(uint16_t));
	 	vtxDst += m_DrawData.componentsToDraw[i].vertices.size() ;
	 	idxDst += m_DrawData.componentsToDraw[i].indices.size();
	}

	m_VertexBuffer.flush(VK_WHOLE_SIZE);
	m_IndexBuffer.flush(VK_WHOLE_SIZE);
}

void GuiMainUi::createUniformBuffer(const VkCommandBuffer& command_buffer) {
	m_Viewport.x = 0.0f;
	m_Viewport.y = 0.0f;
	m_Viewport.width = (float)p_LogicalDevice->swapchain_extent.width;
	m_Viewport.height = (float)p_LogicalDevice->swapchain_extent.height;
	m_Viewport.minDepth = 0.0f;
	m_Viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &m_Viewport);

	// UI scale and translate via push constants
	m_PushConstBlock.scale = glm::vec2(2.0f / m_Viewport.width, 2.0f / m_Viewport.height );
	m_PushConstBlock.translate = glm::vec2(-1.0f);
	vkCmdPushConstants(command_buffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &m_PushConstBlock);

	VkDeviceSize offsets[1] = { 0 };

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &m_VertexBuffer.getBuffer(), offsets);
	vkCmdBindIndexBuffer(command_buffer, m_IndexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

	// Render the command lists:
	int32_t vertexOffset = 0;
	int32_t indexOffset = 0;

	

	for (int32_t i = 0; i < m_DrawData.componentsToDraw.size(); i++) {
		m_Scissor.offset.x = 0;//std::max((int32_t)(drawData.componentsToDraw[i].clipExtent.x),0);
		m_Scissor.offset.y = 0;//std::max((int32_t)(drawData.componentsToDraw[i].clipExtent.y), 0);
		m_Scissor.extent.width = p_LogicalDevice->swapchain_extent.width;//(uint32_t)(drawData.componentsToDraw[i].clipExtent.z - drawData.componentsToDraw[i].clipExtent.x);
		m_Scissor.extent.height = p_LogicalDevice->swapchain_extent.height;//(uint32_t)(drawData.componentsToDraw[i].clipExtent.w - drawData.componentsToDraw[i].clipExtent.y);
		vkCmdSetScissor(command_buffer, 0, 1, &m_Scissor);
		vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(m_DrawData.componentsToDraw[i].indices.size()), 1, indexOffset, vertexOffset, 0);
		//vkCmdDrawIndexed(command_buffers[i], actors[j]->mesh.indexCount, 1, 0, actors[j]->mesh.indexBase, 0);
		indexOffset += static_cast<uint32_t>(m_DrawData.componentsToDraw[i].indices.size());
		vertexOffset += static_cast<uint32_t>(m_DrawData.componentsToDraw[i].vertices.size());
	}

	//std::cout << drawData.componentsToDraw[0].indices.size() << " " <<  drawData.componentsToDraw[0].vertices.size() <<  std::endl;
	//std::cout << scissor.offset.x << " " << scissor.offset.y << std::endl;
	//std::cout << scissor.extent.width << " " << scissor.extent.height << std::endl;
}

void GuiMainUi::cleanUpForSwapchain() {
	destroyPipeline();
}

void GuiMainUi::recreateForSwapchain() {
	createGraphicsPipeline();
}

void GuiMainUi::destroyPipeline() {
	vkDestroyPipelineCache(p_LogicalDevice->get(), m_PipelineCache, nullptr);
	vkDestroyPipeline(p_LogicalDevice->get(), m_Pipeline, nullptr);
	vkDestroyPipelineLayout(p_LogicalDevice->get(), m_PipelineLayout, nullptr);
}

void GuiMainUi::deInit() {
	m_IndexBuffer.destroy();
	m_VertexBuffer.destroy();
	m_Font.DeInit();
	vkDestroyDescriptorPool(p_LogicalDevice->get(), m_DescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(p_LogicalDevice->get(), m_DescriptorSetLayout, nullptr);

	p_LogicalDevice = nullptr;
	p_CommandPool = nullptr;
}
