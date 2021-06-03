#include "../headers/Attachment_Manager.hpp"

namespace rtf
{
	Attachment_Manager::Attachment_Manager(VkDevice* device, vks::VulkanDevice* vulkanDevice, VkPhysicalDevice* physicalDevice, uint32_t width, uint32_t height)
		: m_size{ width, height }
	{
		this->m_device = device;
		this->vulkanDevice = vulkanDevice;
		this->physicalDevice = physicalDevice;

		//create neccessary Attachments
		createAllAttachments(width, height);
	}

	FrameBufferAttachment* Attachment_Manager::getAttachment(Attachment attachment)
	{
		return &m_attachments[(int)attachment];
	}


	void Attachment_Manager::createAllAttachments(uint32_t width, uint32_t height)
	{
		// Find a suitable depth format
		VkFormat attDepthFormat;
		VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(*physicalDevice, &attDepthFormat);
		assert(validDepthFormat);
		m_attachmentTypes[(int)Attachment::depth].m_Format = attDepthFormat;

		for (int idx = 0; idx < m_maxAttachmentSize; idx++)
		{
			if (m_attachmentTypes.at(idx).m_Size.width == 0 || m_attachmentTypes.at(idx).m_Size.height == 0)
			{
				m_attachmentTypes.at(idx).m_Size = VkExtent2D{ width, height };
			}
			this->createAttachment(m_attachmentTypes.at(idx), &m_attachments[idx]);
		}
	}

	void Attachment_Manager::destroyAllAttachments()
	{
		//Destroy & free all attachments
		for (int idx = 0; idx < m_maxAttachmentSize; idx++)
		{
			destroyAttachment(&m_attachments[idx]);
		}
	}

	// Create a frame buffer attachment
	void Attachment_Manager::createAttachment(const AttachmentInitInfo& initInfo, FrameBufferAttachment* attachment)
	{
		VkImageAspectFlags aspectMask = 0;
		VkImageLayout imageLayout;

		attachment->format = initInfo.m_Format;

		if (initInfo.m_UsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		if (initInfo.m_UsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

		assert(aspectMask > 0);

		VkImageCreateInfo image = vks::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = initInfo.m_Format;
		image.extent = VkExtent3D{ initInfo.m_Size.width, initInfo.m_Size.height, 1 };
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = initInfo.m_UsageFlags | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VK_CHECK_RESULT(vkCreateImage(*m_device, &image, nullptr, &attachment->image));
		vkGetImageMemoryRequirements(*m_device, attachment->image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(*m_device, &memAlloc, nullptr, &attachment->mem));
		VK_CHECK_RESULT(vkBindImageMemory(*m_device, attachment->image, attachment->mem, 0));

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
		VK_CHECK_RESULT(vkCreateImageView(*m_device, &imageView, nullptr, &attachment->view));

	}
	
	Attachment_Manager::~Attachment_Manager()
	{
		destroyAllAttachments();
	}

	void Attachment_Manager::destroyAttachment(FrameBufferAttachment* attachment)
	{
		vkDestroyImageView(*m_device, attachment->view, nullptr);
		vkDestroyImage(*m_device, attachment->image, nullptr);
		vkFreeMemory(*m_device, attachment->mem, nullptr);
	}
}
