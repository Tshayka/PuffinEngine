#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>

#include "LoadFile.cpp"
#include "Ui.hpp"

//---------- Constructors and dectructors ---------- //

GuiElement::GuiElement() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui element created\n";
#endif 
}

GuiElement::~GuiElement() {	
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Gui element destroyed\n";
#endif
}

void GuiElement::InitMenu(Device* device) {
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

void GuiElement::SetUp() {
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)logical_device->swapchain_extent.width, (float)logical_device->swapchain_extent.height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 5.3f;
	style.GrabRounding = style.FrameRounding = 2.3f;
	style.ScrollbarRounding = 5.0f;
	style.FrameBorderSize = 1.0f;
	style.ItemSpacing.y = 6.5f;

	style.Colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	style.Colors[ImGuiCol_ChildBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
	style.Colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	style.Colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	style.Colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
	style.Colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
	style.Colors[ImGuiCol_FrameBgActive]          = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
	style.Colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive]          = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	style.Colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	style.Colors[ImGuiCol_CheckMark]              = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab]             = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	style.Colors[ImGuiCol_Button]                 = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
	style.Colors[ImGuiCol_ButtonHovered]          = ImVec4(0.46f, 0.47f, 0.48f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive]           = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
	style.Colors[ImGuiCol_Header]                 = ImVec4(0.70f, 0.70f, 0.70f, 0.31f);
	style.Colors[ImGuiCol_HeaderHovered]          = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
	style.Colors[ImGuiCol_HeaderActive]           = ImVec4(0.48f, 0.50f, 0.52f, 1.00f);
	style.Colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	style.Colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
	style.Colors[ImGuiCol_SeparatorActive]        = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip]             = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
	style.Colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
	style.Colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram]          = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
	style.Colors[ImGuiCol_ModalWindowDarkening]   = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	style.Colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	style.Colors[ImGuiCol_NavHighlight]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
}

void GuiElement::CreateCommandPool() {
	
	QueueFamilyIndices queueFamilyIndices = logical_device->FindQueueFamilies(logical_device->gpu);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow command buffers to be rerecorded individually, optional

	ErrorCheck(vkCreateCommandPool(logical_device->device, &poolInfo, nullptr, &command_pool));
}

VkCommandBuffer GuiElement::BeginSingleTimeCommands() {
	
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

void GuiElement::EndSingleTimeCommands(VkCommandBuffer command_buffer) {
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &command_buffer;

	vkQueueSubmit(logical_device->queue, 1, &SubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(logical_device->queue);

	vkFreeCommandBuffers(logical_device->device, command_pool, 1, &command_buffer);
}

void GuiElement::InitResources() {
	ImGuiIO& io = ImGui::GetIO();

	// Create font texture
	unsigned char* fontData;
	int texWidth, texHeight;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
	VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	VkImageCreateInfo ImageInfo = {};
	ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageInfo.format = format;
	ImageInfo.extent.width = texWidth;
	ImageInfo.extent.height = texHeight;
	ImageInfo.extent.depth = 1;
	ImageInfo.mipLevels = 1;
	ImageInfo.arrayLayers = 1;
	ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	ErrorCheck(vkCreateImage(logical_device->device, &ImageInfo, nullptr, &font_image));

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(logical_device->device, font_image, &memory_requirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memory_requirements.size;
	allocInfo.memoryTypeIndex = logical_device->FindMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(logical_device->device, &allocInfo, nullptr, &font_memory) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to allocate image memory!");
		std::exit(-1);
	}

	ErrorCheck(vkBindImageMemory(logical_device->device, font_image, font_memory, 0));

	enginetool::Buffer staging_buffer;
	logical_device->CreateStagedBuffer(uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, fontData);
	
	vkGetBufferMemoryRequirements(logical_device->device, staging_buffer.buffer, &memory_requirements);

	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	VkBufferImageCopy Region = {};
	Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//Region.imageSubresource.mipLevel = 0;
	//Region.imageSubresource.baseArrayLayer = 0;
	Region.imageSubresource.layerCount = 1;
	Region.imageExtent.width = texWidth;
	Region.imageExtent.height = texHeight;
	Region.imageExtent.depth = 1;
	//Region.bufferOffset = 0;

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = font_image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkCmdCopyBufferToImage(command_buffer, staging_buffer.buffer, font_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = font_image;
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

	// Store our identifier
	io.Fonts->TexID = (void *)(intptr_t)font_image;
}

void GuiElement::CreateViewAndSampler() {
	VkImageViewCreateInfo ViewInfo = {};
	ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ViewInfo.image = font_image;
	ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	ViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM; //TODO
	ViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	ViewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	if (vkCreateImageView(logical_device->device, &ViewInfo, nullptr, &ui_image_view) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: failed to create menu texture image view!");
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
	SamplerInfo.minLod = -1000.0f;
	SamplerInfo.maxLod = 1000.0f;
	SamplerInfo.maxAnisotropy = 1.0f;
	
	if (vkCreateSampler(logical_device->device, &SamplerInfo, nullptr, &ui_texture_sampler) != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: Failed to create a sampler for font texture!");
		std::exit(-1);
	}
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

	ErrorCheck(vkCreateDescriptorSetLayout(logical_device->device, &SceneObjectsLayoutInfo, nullptr, &descriptor_set_layout));
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

	ErrorCheck(vkCreateDescriptorPool(logical_device->device, &PoolInfo, nullptr, &descriptor_pool));
}

void GuiElement::CreateDescriptorSet() {
	VkDescriptorSetAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AllocInfo.descriptorPool = descriptor_pool;
	AllocInfo.descriptorSetCount = 1;
	AllocInfo.pSetLayouts = &descriptor_set_layout;

	ErrorCheck(vkAllocateDescriptorSets(logical_device->device, &AllocInfo, &gui_descriptor_set));

	VkDescriptorImageInfo ImageInfo = {};
	ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ImageInfo.imageView = ui_image_view;
	ImageInfo.sampler = ui_texture_sampler;

	std::array<VkWriteDescriptorSet, 1> WriteDescriptorSets = {};

	WriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	WriteDescriptorSets[0].dstSet = gui_descriptor_set;
	WriteDescriptorSets[0].dstBinding = 0;
	WriteDescriptorSets[0].dstArrayElement = 0;
	WriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	WriteDescriptorSets[0].descriptorCount = 1;
	WriteDescriptorSets[0].pImageInfo = &ImageInfo;

	vkUpdateDescriptorSets(logical_device->device, static_cast<uint32_t>(WriteDescriptorSets.size()), WriteDescriptorSets.data(), 0, nullptr);
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

VkShaderModule GuiElement::CreateVertShaderModule()
{
	VkShaderModuleCreateInfo vert_info = {};
	vert_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vert_info.codeSize = sizeof(__glsl_shader_vert_spv);
	vert_info.pCode = (uint32_t*)__glsl_shader_vert_spv;

	VkShaderModule shaderModule;
	ErrorCheck(vkCreateShaderModule(logical_device->device, &vert_info, nullptr, &shaderModule));

	return shaderModule;
}

VkShaderModule GuiElement::CreateFragShaderModule()
{
	VkShaderModuleCreateInfo frag_info = {};
	frag_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	frag_info.codeSize = sizeof(__glsl_shader_frag_spv);
	frag_info.pCode = (uint32_t*)__glsl_shader_frag_spv;

	VkShaderModule shaderModule;
	ErrorCheck(vkCreateShaderModule(logical_device->device, &frag_info, nullptr, &shaderModule));

	return shaderModule;
}

void GuiElement::CreateGraphicsPipeline() {
	VkPipelineCacheCreateInfo PipelineCacheCreateInfo = {};
	PipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	ErrorCheck(vkCreatePipelineCache(logical_device->device, &PipelineCacheCreateInfo, nullptr, &pipeline_cache));

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
	
	std::array<VkDescriptorSetLayout, 1> layouts = { descriptor_set_layout };

	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
	PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstantRange;
	PipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());;
	PipelineLayoutCreateInfo.pSetLayouts = layouts.data();
	
	ErrorCheck(vkCreatePipelineLayout(logical_device->device, &PipelineLayoutCreateInfo, nullptr, &gui_pipeline_layout));

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
	PipelineInfo.layout = gui_pipeline_layout;
	PipelineInfo.renderPass = logical_device->renderPass;
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	auto vertModelsShaderCode = enginetool::readFile("puffinEngine/shaders/imgui_menu_shader.vert.spv"); // DRY
	auto fragModelsShaderCode = enginetool::readFile("puffinEngine/shaders/imgui_menu_shader.frag.spv"); // DRY 

	VkShaderModule vertModelsShaderModule = CreateVertShaderModule();
	VkShaderModule fragModelsShaderModule = CreateFragShaderModule();

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

	ErrorCheck(vkCreateGraphicsPipelines(logical_device->device, pipeline_cache, 1, &PipelineInfo, nullptr, &pipeline));

	vkDestroyShaderModule(logical_device->device, fragModelsShaderModule, nullptr);
	vkDestroyShaderModule(logical_device->device, vertModelsShaderModule, nullptr); 
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

	// Vertex buffer
	if ((vertex_buffer.buffer == VK_NULL_HANDLE) || (vertex_count != draw_data->TotalVtxCount)) {
		vertex_buffer.Unmap();
		vertex_buffer.Destroy();

		logical_device->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertex_buffer.buffer, vertex_buffer.memory);
		vertex_buffer.device = logical_device->device;
		vertex_count = draw_data->TotalVtxCount;
		vertex_buffer.Unmap();
		vertex_buffer.Map();
	}

	// Index buffer
	VkDeviceSize indexSize = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
	if ((index_buffer.buffer == VK_NULL_HANDLE) || (index_count < draw_data->TotalIdxCount)) {
		index_buffer.Unmap();
		index_buffer.Destroy();

		logical_device->CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, index_buffer.buffer, index_buffer.memory);
		index_buffer.device = logical_device->device;
		index_count = draw_data->TotalIdxCount;
		index_buffer.Map();
	}

	// Upload data
	ImDrawVert* vtxDst = (ImDrawVert*)vertex_buffer.mapped;
	ImDrawIdx* idxDst = (ImDrawIdx*)index_buffer.mapped;

	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	vertex_buffer.Flush();
	index_buffer.Flush();
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
	vkCmdPushConstants(command_buffer, gui_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

	VkDeviceSize offsets[1] = { 0 };

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer.buffer, offsets);
	vkCmdBindIndexBuffer(command_buffer, index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gui_pipeline_layout, 0, 1, &gui_descriptor_set, 0, nullptr);

	// Render the command lists:
	int32_t vertex_offset = 0;
	int32_t index_offset = 0;

	ImVec2 display_pos = draw_data->DisplayPos;

	for (int32_t i = 0; i < draw_data->CmdListsCount; i++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[i];
		for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
			if (pcmd->UserCallback)	{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else {
				scissor.offset.x = (int32_t)(pcmd->ClipRect.x - display_pos.x) > 0 ? (int32_t)(pcmd->ClipRect.x - display_pos.x) : 0;
				scissor.offset.y = (int32_t)(pcmd->ClipRect.y - display_pos.y) > 0 ? (int32_t)(pcmd->ClipRect.y - display_pos.y) : 0;
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

void GuiElement::DeInitMenu() {
	index_buffer.Destroy();
	vertex_buffer.Destroy();
	vkDestroyImage(logical_device->device, font_image, nullptr);
	vkDestroyImageView(logical_device->device, ui_image_view, nullptr);
	vkFreeMemory(logical_device->device, font_memory, nullptr);
	vkDestroySampler(logical_device->device, ui_texture_sampler, nullptr);
	vkDestroyPipelineCache(logical_device->device, pipeline_cache, nullptr);
	vkDestroyPipeline(logical_device->device, pipeline, nullptr);
	vkDestroyPipelineLayout(logical_device->device, gui_pipeline_layout, nullptr);
	vkDestroyCommandPool(logical_device->device, command_pool, nullptr);
	vkDestroyDescriptorPool(logical_device->device, descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(logical_device->device, descriptor_set_layout, nullptr);
}