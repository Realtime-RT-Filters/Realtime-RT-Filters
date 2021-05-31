#ifndef Attachment_Manager_h
#define Attachment_Manager_h

#include "disable_warnings.h"
#include <VulkanDevice.h>

#include <utility>

namespace rtf {

	//These are global types needed by other classes
	enum class Attachment
	{
		position,
		normal,
		albedo,
		depth,
		meshid,
		motionvector,
		rtoutput,
		filteroutput,
		history_color,

		// add your attachment before this one
		max_attachments,
	};

	// Framebuffer
	struct FrameBufferAttachment
	{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
		VkFormat format;
	};

	/// <summary>
	/// Class managing attachments for all render passes
	/// </summary>
	class Attachment_Manager
	{
		std::vector<std::pair<VkFormat, VkImageUsageFlags>> m_attachmentTypes = {
			{ VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT }, // position
			{ VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT }, // normal
			{ VK_FORMAT_R16G16B16A16_SFLOAT, (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT) }, // albedo
			{ VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT }, // depth
			{ VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT }, // meshid
			{ VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT }, // motionvector
			{ VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT }, // rtoutput
			{ VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT }, // filteroutput
			{ VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT }, // history_color
		};

	public:
		static const uint32_t DEFAULT_WIDTH = 2048, DEFAULT_HEIGHT = 2048;

		Attachment_Manager(VkDevice* device, vks::VulkanDevice* vulkanDevice, VkPhysicalDevice* physicalDevice, uint32_t width = DEFAULT_WIDTH, uint32_t height = DEFAULT_HEIGHT);
		~Attachment_Manager();

		inline VkExtent2D GetSize() const { return m_size; }

		void createAttachment(VkFormat format, VkImageUsageFlags usage, FrameBufferAttachment* attachment, int width, int height);
		FrameBufferAttachment* getAttachment(Attachment);

		void createAllAttachments(int width, int height);
		void destroyAllAttachments();

	private:

		void destroyAttachment(FrameBufferAttachment* attachment);

		//Attachment Manager needs to be aware of certain Vulkan components
		VkDevice* m_device;

		//vks::Vkdevice is a combined logical/physical vulkan device
		vks::VulkanDevice* vulkanDevice;
		VkPhysicalDevice* physicalDevice;

		// Keeping track of current attachment size
		VkExtent2D m_size;

		// List of managed Attachments
		static const int m_maxAttachmentSize = (int)Attachment::max_attachments;
		FrameBufferAttachment m_attachments[m_maxAttachmentSize];
	};
}

#endif //Attachment_Manager_h
