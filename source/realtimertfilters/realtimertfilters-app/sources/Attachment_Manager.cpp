#include "../headers/Attachment_Manager.hpp"

namespace rtf
{
	Attachment_Manager::Attachment_Manager(vks::VulkanDevice* vulkanDevice, VkQueue graphicsQueue, uint32_t width, uint32_t height)
		: m_size{ width, height }, m_vulkanDevice(vulkanDevice), m_queue(graphicsQueue)
	{
		//create neccessary Attachments
		createAllAttachments();
	}

	FrameBufferAttachment* Attachment_Manager::getAttachment(Attachment attachment)
	{
		return &m_attachments[(int)attachment];
	}

	void Attachment_Manager::getAllAttachments(FrameBufferAttachment*& out_arr, size_t& out_count)
	{
		out_arr = m_attachments;
		out_count = (size_t)Attachment::max_attachments;
	}


	void Attachment_Manager::createAllAttachments()
	{
		VkCommandBuffer cmdBuffer = m_vulkanDevice->createCommandBuffer(VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		// Find a suitable depth format
		VkFormat attDepthFormat;
		VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(m_vulkanDevice->physicalDevice, &attDepthFormat);
		assert(validDepthFormat);
		//m_attachmentTypes[(int)Attachment::depth].m_Format = attDepthFormat;

		for (auto& initInfo : m_attachmentInitsUnsorted)
		{
			m_attachmentInits.at((size_t)initInfo.m_AttachmentId) = initInfo;
		}

		for (int idx = 0; idx < m_maxAttachmentSize; idx++)
		{
			this->createAttachment(cmdBuffer, m_attachmentInits.at(idx), &m_attachments[idx]);
		}
		m_vulkanDevice->flushCommandBuffer(cmdBuffer, m_queue);
	}

	void Attachment_Manager::destroyAllAttachments()
	{
		//Destroy & free all attachments
		for (int idx = 0; idx < m_maxAttachmentSize; idx++)
		{
			destroyAttachment(&m_attachments[idx]);
		}
	}

	void Attachment_Manager::resize(VkExtent2D newsize)
	{
		if (m_size.width == newsize.width && m_size.height == newsize.height)
		{
			return;
		}
		m_size = newsize;
		createAllAttachments();
	}

	// Create a frame buffer attachment
	void Attachment_Manager::createAttachment(VkCommandBuffer cmdBuffer, const AttachmentInitInfo& initInfo, FrameBufferAttachment* attachment)
	{
		if (attachment->image != nullptr)
		{
			destroyAttachment(attachment);
		}

		VkImageAspectFlags aspectMask = 0;
		attachment->format = initInfo.m_Format;

		if (initInfo.m_UsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (initInfo.m_UsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT /*| VK_IMAGE_ASPECT_STENCIL_BIT*/;
		}

		assert(aspectMask > 0);

		VkExtent3D extent;
		if (initInfo.m_Size.width == 0 || initInfo.m_Size.height == 0)
		{
			extent = VkExtent3D{ m_size.width, m_size.height, 1 };
		}
		else
		{
			extent = VkExtent3D{ initInfo.m_Size.width, initInfo.m_Size.height, 1 };
		}

		VkImageCreateInfo image = vks::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = initInfo.m_Format;
		image.extent = extent;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = initInfo.m_UsageFlags | VK_IMAGE_USAGE_SAMPLED_BIT;
		image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VK_CHECK_RESULT(vkCreateImage(m_vulkanDevice->logicalDevice, &image, nullptr, &attachment->image));
		vkGetImageMemoryRequirements(m_vulkanDevice->logicalDevice, attachment->image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = m_vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(m_vulkanDevice->logicalDevice, &memAlloc, nullptr, &attachment->mem));
		VK_CHECK_RESULT(vkBindImageMemory(m_vulkanDevice->logicalDevice, attachment->image, attachment->mem, 0));

		VkImageViewCreateInfo imageView = vks::initializers::imageViewCreateInfo();
		imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageView.format = initInfo.m_Format;
		imageView.subresourceRange = {};
		imageView.subresourceRange.aspectMask = aspectMask;
		imageView.subresourceRange.baseMipLevel = 0;
		imageView.subresourceRange.levelCount = 1;
		imageView.subresourceRange.baseArrayLayer = 0;
		imageView.subresourceRange.layerCount = 1;
		imageView.image = attachment->image;
		VK_CHECK_RESULT(vkCreateImageView(m_vulkanDevice->logicalDevice, &imageView, nullptr, &attachment->view));

		if (!(initInfo.m_UsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			vks::tools::setImageLayout(cmdBuffer, attachment->image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				initInfo.m_InitialLayout,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		}
	}
	
	Attachment_Manager::~Attachment_Manager()
	{
		destroyAllAttachments();
	}

	void Attachment_Manager::destroyAttachment(FrameBufferAttachment* attachment)
	{
		vkDestroyImageView(m_vulkanDevice->logicalDevice, attachment->view, nullptr);
		vkDestroyImage(m_vulkanDevice->logicalDevice, attachment->image, nullptr);
		vkFreeMemory(m_vulkanDevice->logicalDevice, attachment->mem, nullptr);
	}
}
