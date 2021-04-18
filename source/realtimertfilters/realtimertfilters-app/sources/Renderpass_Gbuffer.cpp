#include "..\headers\Renderpass_Gbuffer.hpp"

rtf::RenderpassGbuffer::RenderpassGbuffer(VkInstance instance, vks::VulkanDevice* device)
	: Renderpass(instance, device)
{

}

// Heavily inspired from Sascha Willems' "deferred" vulkan example
void rtf::RenderpassGbuffer::Prepare()
{
	prepareAttachments();

	// Formatting attachment data into VkAttachmentDescription structs
	const uint32_t ATTACHMENT_COUNT_COLOR = 5;
	const uint32_t ATTACHMENT_COUNT_DEPTH = 1;
	const uint32_t ATTACHMENT_COUNT = ATTACHMENT_COUNT_COLOR + ATTACHMENT_COUNT_DEPTH;
	VkAttachmentDescription attachmentDescriptions[ATTACHMENT_COUNT] = {};
	FrameBufferAttachment* attachments[] =
	{
		&Position, &Normal, &Albedo, &Velocity, &ObjectId, &Depth
	};

	for (uint32_t i = 0; i < ATTACHMENT_COUNT; i++)
	{
		attachmentDescriptions[i].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[i].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[i].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptions[i].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[i].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[i].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[i].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachmentDescriptions[i].format = attachments[i]->format;
	}
	attachmentDescriptions[5].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // The depth attachment needs a different layout

	// Preparing attachment reference structs
	VkAttachmentReference attachmentReferences_Color[ATTACHMENT_COUNT_COLOR] = {};
	for (uint32_t i = 0; i < ATTACHMENT_COUNT_COLOR; i++)
	{
		attachmentReferences_Color[i] = VkAttachmentReference{ i, VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }; // Assign incremental ids
	}

	VkAttachmentReference attachmentReference_Depth = 
	{ ATTACHMENT_COUNT_COLOR, VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }; // the depth attachment gets the final id (one higher than the highest color attachment id)

	// Subpass description
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = ATTACHMENT_COUNT_COLOR;
	subpass.pColorAttachments = attachmentReferences_Color;
	subpass.pDepthStencilAttachment = &attachmentReference_Depth;

}

void rtf::RenderpassGbuffer::prepareAttachments()
{
	// (World space) Positions
	createAttachment(
		VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT,
		VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		&Position);

	// (World space) Normals
	createAttachment(
		VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT,
		VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		&Normal);

	// Albedo (color)
	createAttachment(
		VkFormat::VK_FORMAT_R8G8B8A8_UNORM,
		VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		&Albedo);

	// Depth
	createAttachment(
		VkFormat::VK_FORMAT_R32_SFLOAT,
		VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		&Depth
	);

	// Velocity
	createAttachment(
		VkFormat::VK_FORMAT_R32G32_SFLOAT,
		VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		&Velocity
	);

	// ObjectId
	createAttachment(
		VkFormat::VK_FORMAT_R32_UINT,
		VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		&ObjectId
	);
}

void rtf::RenderpassGbuffer::Draw()
{}

void rtf::RenderpassGbuffer::CleanUp()
{}

// Pretty much copied from Sascha Willems' "deferred" vulkan example
void rtf::RenderpassGbuffer::createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment * attachment)
{
	VkImageAspectFlags aspectMask = 0;
	VkImageLayout imageLayout;

	attachment->format = format;

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	assert(aspectMask > 0);

	VkImageCreateInfo image = vks::initializers::imageCreateInfo();
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = format;
	image.extent.width = Width;
	image.extent.height = Height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

	VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs;

	VK_CHECK_RESULT(vkCreateImage(Device->logicalDevice, &image, nullptr, &attachment->image));
	vkGetImageMemoryRequirements(Device->logicalDevice, attachment->image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = Device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(Device->logicalDevice, &memAlloc, nullptr, &attachment->mem));
	VK_CHECK_RESULT(vkBindImageMemory(Device->logicalDevice, attachment->image, attachment->mem, 0));

	VkImageViewCreateInfo imageView = vks::initializers::imageViewCreateInfo();
	imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageView.format = format;
	imageView.subresourceRange = {};
	imageView.subresourceRange.aspectMask = aspectMask;
	imageView.subresourceRange.baseMipLevel = 0;
	imageView.subresourceRange.levelCount = 1;
	imageView.subresourceRange.baseArrayLayer = 0;
	imageView.subresourceRange.layerCount = 1;
	imageView.image = attachment->image;
	VK_CHECK_RESULT(vkCreateImageView(Device->logicalDevice, &imageView, nullptr, &attachment->view));
}
