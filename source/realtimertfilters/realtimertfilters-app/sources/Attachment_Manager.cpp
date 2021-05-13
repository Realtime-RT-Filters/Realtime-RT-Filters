#include "..\headers\Attachment_Manager.hpp"

namespace rtf
{
	Attachment_Manager::Attachment_Manager(VkDevice* device, vks::VulkanDevice* vulkanDevice, VkPhysicalDevice* physicalDevice)
	{
		this->device = device;
		this->vulkanDevice = vulkanDevice;
		this->physicalDevice = physicalDevice;

		//create neccessary Attachments
		createAllAttachments();
	}


	FrameBufferAttachment* Attachment_Manager::getAttachment(Attachment attachment)
	{

		//Return requested attachment depending on request
		switch (attachment)
		{
		case rtf::position:
			return &m_position;
		case rtf::normal:
			return &m_normal;
		case rtf::albedo:
			return &m_albedo;
		case rtf::depth:
			return &m_depth;
		case rtf::output_rt:
			break;
		case rtf::output_filter:
			break;
		default:
			break;
		}

		return &FrameBufferAttachment();
	}


	void Attachment_Manager::createAllAttachments()
	{

		int width = 2048;
		int height = 2048;


		//create all the required attachments here

		//Prepass Outputs
		// (World space) Positions
		this->createAttachment(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			&m_position,
			width,
			height);


		// (World space) Normals
		this->createAttachment(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			&m_normal,
			width,
			height);


		// Albedo (color)
		this->createAttachment(
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			&m_albedo,
			width,
			height);


		// Depth attachment

		// Find a suitable depth format
		VkFormat attDepthFormat;
		VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(*physicalDevice, &attDepthFormat);
		assert(validDepthFormat);

		this->createAttachment(
			attDepthFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			&m_depth,
			width,
			height);



		//Ray Tracing Outputs





	}



	// Create a frame buffer attachment
	void Attachment_Manager::createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment* attachment, int width, int height)
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
		image.extent.width = width;
		image.extent.height = height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VK_CHECK_RESULT(vkCreateImage(*device, &image, nullptr, &attachment->image));
		vkGetImageMemoryRequirements(*device, attachment->image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(*device, &memAlloc, nullptr, &attachment->mem));
		VK_CHECK_RESULT(vkBindImageMemory(*device, attachment->image, attachment->mem, 0));

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
		VK_CHECK_RESULT(vkCreateImageView(*device, &imageView, nullptr, &attachment->view));

	}
	
	Attachment_Manager::~Attachment_Manager()
	{
		//Destroy & free all attachments

		vkDestroyImageView(*device, m_position.view, nullptr);
		vkDestroyImage(*device, m_position.image, nullptr);
		vkFreeMemory(*device, m_position.mem, nullptr);

		vkDestroyImageView(*device, m_normal.view, nullptr);
		vkDestroyImage(*device, m_normal.image, nullptr);
		vkFreeMemory(*device, m_normal.mem, nullptr);

		vkDestroyImageView(*device, m_albedo.view, nullptr);
		vkDestroyImage(*device, m_albedo.image, nullptr);
		vkFreeMemory(*device, m_albedo.mem, nullptr);

		vkDestroyImageView(*device, m_depth.view, nullptr);
		vkDestroyImage(*device, m_depth.image, nullptr);
		vkFreeMemory(*device, m_depth.mem, nullptr);


	}

}
