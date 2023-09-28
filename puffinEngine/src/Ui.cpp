#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "LoadFile.cpp"
#include "headers/Ui.hpp"

using namespace puffinengine::tool;

//---------- Constructors and dectructors ---------- //

GuiElement::GuiElement() {
#if DEBUG_VERSION
	std::cout << "Gui - imgui element - created\n";
#endif 
}

GuiElement::~GuiElement() {	
#if DEBUG_VERSION
	std::cout << "Gui - imgui element - destroyed\n";
#endif
}

void GuiElement::init(Device* device, RenderPass* renderPass, VkCommandPool* commandPool) {
	p_LogicalDevice = device;
	p_RenderPass = renderPass;
	p_CommandPool = commandPool;

	m_VertexBuffer.setDevice(device);
	m_IndexBuffer.setDevice(device);

	setUp();
	loadFontImage();
	createDescriptorSetLayout();
	createDescriptorPool();
	createDescriptorSet();
	createGraphicsPipeline();
}

void GuiElement::setUp() {
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)p_LogicalDevice->swapchain_extent.width, (float)p_LogicalDevice->swapchain_extent.height);
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

void GuiElement::loadFontImage() {
	ImGuiIO& io = ImGui::GetIO();

	unsigned char* fontData;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &m_Font.texWidth, &m_Font.texHeight);
	VkDeviceSize uploadSize = static_cast<uint64_t>(m_Font.texWidth) * static_cast<uint64_t>(m_Font.texHeight) * 4 * sizeof(char);
	
	enginetool::Buffer stagingBuffer;
	stagingBuffer.setDevice(p_LogicalDevice);
	stagingBuffer.createStagedBuffer(uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, fontData);
	
	m_Font.Init(p_LogicalDevice, *p_CommandPool, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 1);
	m_Font.CreateImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	m_Font.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	m_Font.CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);

	m_Font.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_Font.CopyBufferToImage(stagingBuffer.getBuffer());
	stagingBuffer.destroy();
	m_Font.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Store our identifier
	io.Fonts->TexID = (void *)(intptr_t)m_Font.m_FontImage;	
}

void GuiElement::createDescriptorSetLayout() {
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


void GuiElement::createDescriptorPool() {
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

void GuiElement::createDescriptorSet() {
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

static uint32_t __glsl_shader_vert_spv[] =
{
	0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
	0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
	0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
	0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
	0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
	0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
	0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
	0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
	0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
	0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
	0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
	0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
	0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
	0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
	0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
	0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
	0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
	0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
	0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
	0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
	0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
	0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
	0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
	0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
	0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
	0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
	0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
	0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
	0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
	0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
	0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
	0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
	0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
	0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
	0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
	0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
	0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
	0x0000002d,0x0000002c,0x000100fd,0x00010038
};

static uint32_t __glsl_shader_frag_spv[] =
{
	0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
	0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
	0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
	0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
	0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
	0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
	0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
	0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
	0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
	0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
	0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
	0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
	0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
	0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
	0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
	0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
	0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
	0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
	0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
	0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
	0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
	0x00010038
};

VkShaderModule GuiElement::createVertShaderModule() {
	VkShaderModuleCreateInfo vert_info = {};
	vert_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vert_info.codeSize = sizeof(__glsl_shader_vert_spv);
	vert_info.pCode = (uint32_t*)__glsl_shader_vert_spv;

	VkShaderModule shaderModule;
	ErrorCheck(vkCreateShaderModule(p_LogicalDevice->get(), &vert_info, nullptr, &shaderModule));

	return shaderModule;
}

VkShaderModule GuiElement::createFragShaderModule() {
	VkShaderModuleCreateInfo frag_info = {};
	frag_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	frag_info.codeSize = sizeof(__glsl_shader_frag_spv);
	frag_info.pCode = (uint32_t*)__glsl_shader_frag_spv;

	VkShaderModule shaderModule;
	ErrorCheck(vkCreateShaderModule(p_LogicalDevice->get(), &frag_info, nullptr, &shaderModule));

	return shaderModule;
}

void GuiElement::createGraphicsPipeline() {
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
	PipelineInfo.renderPass = p_RenderPass->get();
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	std::filesystem::path p = std::filesystem::current_path().parent_path();
	std::filesystem::path vertModelsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "imgui_menu_shader.vert.spv";
	std::filesystem::path fragModelsShaderCodePath = p / std::filesystem::path("puffinEngine") / "shaders" / "imgui_menu_shader.frag.spv";

	auto vertModelsShaderCode = enginetool::readFile(vertModelsShaderCodePath.string());
	auto fragModelsShaderCode = enginetool::readFile(fragModelsShaderCodePath.string());

	VkShaderModule vertModelsShaderModule = createVertShaderModule();
	VkShaderModule fragModelsShaderModule = createFragShaderModule();

	//logical_device->CreateShaderModule(vertCloudsShaderCode);

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

void GuiElement::newFrame() {
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	ImGui::Render();
}

void GuiElement::updateDrawData() {
	ImDrawData* draw_data = ImGui::GetDrawData();

	if (draw_data->TotalVtxCount == 0)
		return;

	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = draw_data->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

	// Update buffers only if vertex or index count has been changed compared to current buffer size

	// Vertex buffer
	if (m_VertexCount != draw_data->TotalVtxCount) {
		if (m_VertexBuffer.getBuffer() == VK_NULL_HANDLE) {
			m_VertexBuffer.unmap();
			m_VertexBuffer.destroy();
		}

		m_VertexBuffer.createUnstagedBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		m_VertexCount = draw_data->TotalVtxCount;
		m_VertexBuffer.unmap();
		m_VertexBuffer.map(vertexBufferSize);
	}

	// Index buffer
	//VkDeviceSize indexSize = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
	if (m_IndexCount < draw_data->TotalIdxCount) {
		if (m_IndexBuffer.getBuffer() == VK_NULL_HANDLE) {
			m_IndexBuffer.unmap();
			m_IndexBuffer.destroy();
		}

		m_IndexBuffer.createUnstagedBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		m_IndexCount = draw_data->TotalIdxCount;
		m_IndexBuffer.unmap();
		m_IndexBuffer.map(indexBufferSize);
	}

	// Upload data
	ImDrawVert* vtxDst = (ImDrawVert*)m_VertexBuffer.getMapped();
	ImDrawIdx* idxDst = (ImDrawIdx*)m_IndexBuffer.getMapped();

	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	m_VertexBuffer.flush(VK_WHOLE_SIZE);
	m_IndexBuffer.flush(VK_WHOLE_SIZE);
}

void GuiElement::createUniformBuffer(const VkCommandBuffer& command_buffer) {
	ImDrawData* draw_data = ImGui::GetDrawData();

	if (draw_data->TotalVtxCount == 0) {
		return;
	}

	m_Viewport.x = 0.0f;
	m_Viewport.y = 0.0f;
	m_Viewport.width = draw_data->DisplaySize.x;
	m_Viewport.height = draw_data->DisplaySize.y;
	m_Viewport.minDepth = 0.0f;
	m_Viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &m_Viewport);

	// UI scale and translate via push constants
	m_PushConstBlock.scale = glm::vec2(2.0f / draw_data->DisplaySize.x, 2.0f / draw_data->DisplaySize.y);
	m_PushConstBlock.translate = glm::vec2(-1.0f - draw_data->DisplayPos.x * m_PushConstBlock.scale.x, -1.0f - draw_data->DisplayPos.y * m_PushConstBlock.scale.y);
	vkCmdPushConstants(command_buffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &m_PushConstBlock);

	VkDeviceSize offsets[1] = { 0 };

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &m_VertexBuffer.getBuffer(), offsets);
	vkCmdBindIndexBuffer(command_buffer, m_IndexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

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
				m_Scissor.offset.x = std::max((int32_t)(pcmd->ClipRect.x - draw_data->DisplayPos.x), 0);
				m_Scissor.offset.y = std::max((int32_t)(pcmd->ClipRect.y - draw_data->DisplayPos.y), 0);
				m_Scissor.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
				m_Scissor.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
				vkCmdSetScissor(command_buffer, 0, 1, &m_Scissor);

				// Draw
				vkCmdDrawIndexed(command_buffer, pcmd->ElemCount, 1, index_offset, vertex_offset, 0);
			}
			index_offset += pcmd->ElemCount;
		}
		vertex_offset += cmd_list->VtxBuffer.Size;
	}
}

void GuiElement::cleanUpForSwapchain() {
	destroyPipeline();
}

void GuiElement::recreateForSwapchain() {
	createGraphicsPipeline();
}

void GuiElement::destroyPipeline() {
	vkDestroyPipelineCache(p_LogicalDevice->get(), m_PipelineCache, nullptr);
	vkDestroyPipeline(p_LogicalDevice->get(), m_Pipeline, nullptr);
	vkDestroyPipelineLayout(p_LogicalDevice->get(), m_PipelineLayout, nullptr);
}

void GuiElement::deInit() {
	m_IndexBuffer.destroy();
	m_VertexBuffer.destroy();
	m_Font.DeInit();
	vkDestroyDescriptorPool(p_LogicalDevice->get(), m_DescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(p_LogicalDevice->get(), m_DescriptorSetLayout, nullptr);

	p_LogicalDevice = nullptr;
	p_CommandPool = nullptr; 
}