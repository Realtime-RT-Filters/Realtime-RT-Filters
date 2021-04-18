#include "..\headers\Renderpass_Gbuffer.hpp"

namespace rtf
{

	RenderpassGbuffer::RenderpassGbuffer(VkInstance instance, vks::VulkanDevice* device, int32_t width, int32_t height)
		: Renderpass(instance, device), m_Width(width), m_Height(height)
	{

	}

	// Heavily inspired from Sascha Willems' "deferred" vulkan example
	void RenderpassGbuffer::prepare()
	{
		prepareAttachments();
		prepareRenderpass();
	}

	void RenderpassGbuffer::prepareAttachments()
	{
		// (World space) Positions
		createAttachment(
			VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT,
			VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			&m_Position);

		// (World space) Normals
		createAttachment(
			VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT,
			VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			&m_Normal);

		// Albedo (color)
		createAttachment(
			VkFormat::VK_FORMAT_R8G8B8A8_UNORM,
			VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			&m_Albedo);

		// Depth
		createAttachment(
			VkFormat::VK_FORMAT_R32_SFLOAT,
			VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			&m_Depth
		);

		// Velocity
		createAttachment(
			VkFormat::VK_FORMAT_R32G32_SFLOAT,
			VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			&m_Velocity
		);

		// ObjectId
		createAttachment(
			VkFormat::VK_FORMAT_R32_UINT,
			VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			&m_ObjectId
		);
	}

	void RenderpassGbuffer::prepareRenderpass()
	{
		// Formatting attachment data into VkAttachmentDescription structs
		const uint32_t ATTACHMENT_COUNT_COLOR = 5;
		const uint32_t ATTACHMENT_COUNT_DEPTH = 1;
		const uint32_t ATTACHMENT_COUNT = ATTACHMENT_COUNT_COLOR + ATTACHMENT_COUNT_DEPTH;
		VkAttachmentDescription attachmentDescriptions[ATTACHMENT_COUNT] = {};
		FrameBufferAttachment* attachments[] =
		{
			&m_Position, &m_Normal, &m_Albedo, &m_Velocity, &m_ObjectId, &m_Depth
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

		VkSubpassDependency subPassDependencies[2] = {};
		subPassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subPassDependencies[0].dstSubpass = 0;
		subPassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subPassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subPassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subPassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subPassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subPassDependencies[1].srcSubpass = 0;
		subPassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subPassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subPassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subPassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subPassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subPassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pAttachments = attachmentDescriptions;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(ATTACHMENT_COUNT);
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = subPassDependencies;

		VK_CHECK_RESULT(vkCreateRenderPass(m_Device->logicalDevice, &renderPassInfo, nullptr, &m_Renderpass));

		VkImageView attachmentViews[6] = { m_Position.view, m_Normal.view, m_Albedo.view, m_Velocity.view, m_ObjectId.view, m_Depth.view };

		VkFramebufferCreateInfo fbufCreateInfo = {};
		fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbufCreateInfo.pNext = NULL;
		fbufCreateInfo.renderPass = m_Renderpass;
		fbufCreateInfo.pAttachments = attachmentViews;
		fbufCreateInfo.attachmentCount = static_cast<uint32_t>(ATTACHMENT_COUNT);
		fbufCreateInfo.width = m_Width;
		fbufCreateInfo.height = m_Height;
		fbufCreateInfo.layers = 1;
		VK_CHECK_RESULT(vkCreateFramebuffer(m_Device->logicalDevice, &fbufCreateInfo, nullptr, &m_FrameBuffer));

		// Create sampler to sample from the color attachments
		VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_NEAREST;
		sampler.minFilter = VK_FILTER_NEAREST;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 1.0f;
		sampler.minLod = 0.0f;
		sampler.maxLod = 1.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(m_Device->logicalDevice, &sampler, nullptr, &m_ColorSampler));
	}

	void RenderpassGbuffer::draw()
	{}

	void RenderpassGbuffer::cleanUp()
	{}


	// Pretty much copied from Sascha Willems' "deferred" vulkan example
	void RenderpassGbuffer::createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment* attachment)
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
		image.extent.width = m_Width;
		image.extent.height = m_Height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VK_CHECK_RESULT(vkCreateImage(m_Device->logicalDevice, &image, nullptr, &attachment->image));
		vkGetImageMemoryRequirements(m_Device->logicalDevice, attachment->image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = m_Device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(m_Device->logicalDevice, &memAlloc, nullptr, &attachment->mem));
		VK_CHECK_RESULT(vkBindImageMemory(m_Device->logicalDevice, attachment->image, attachment->mem, 0));

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
		VK_CHECK_RESULT(vkCreateImageView(m_Device->logicalDevice, &imageView, nullptr, &attachment->view));
	}

	void RenderpassGbuffer::prepareUBOs()
	{
		// Offscreen vertex shader
		VK_CHECK_RESULT(m_Device->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_Buffer,
			sizeof(m_UBO_Offscreen)));

		// Map persistent
		VK_CHECK_RESULT(m_Buffer.map());

		// Setup instanced model positions
		m_UBO_Offscreen.instancePos[0] = glm::vec4(0.0f);
		m_UBO_Offscreen.instancePos[1] = glm::vec4(-4.0f, 0.0, -4.0f, 0.0f);
		m_UBO_Offscreen.instancePos[2] = glm::vec4(4.0f, 0.0, -4.0f, 0.0f);

		// Update
		updateUniformBuffer();
	}
	void RenderpassGbuffer::updateUniformBuffer()
	{
		//m_UBO_Offscreen.projection = camera.matrices.perspective;
		//m_UBO_Offscreen.view = camera.matrices.view;
		//m_UBO_Offscreen.model = glm::mat4(1.0f);
		//memcpy(uniformBuffers.offscreen.mapped, &uboOffscreenVS, sizeof(uboOffscreenVS));
	}
}