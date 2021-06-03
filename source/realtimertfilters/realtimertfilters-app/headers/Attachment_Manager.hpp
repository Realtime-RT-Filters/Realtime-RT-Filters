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
		scratch_buffer,
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

	class AttachmentInitInfo
	{
	public:
		VkFormat m_Format{};
		VkImageUsageFlags m_UsageFlags{};
		VkExtent2D m_Size{};

		AttachmentInitInfo() = default;
		AttachmentInitInfo(VkFormat format, VkImageUsageFlags flags, VkExtent2D size = VkExtent2D{0, 0}) : m_Format(format), m_UsageFlags(flags), m_Size(size) {};
	};

	/// <summary>
	/// Class managing attachments for all render passes
	/// </summary>
	class Attachment_Manager
	{
		std::vector<AttachmentInitInfo> m_attachmentTypes = {
			AttachmentInitInfo(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT), // position
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ), // normal
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT) ), // albedo
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ), // depth
			AttachmentInitInfo( VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ), // meshid
			AttachmentInitInfo( VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ), // motionvector
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT ), // rtoutput
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ), // filteroutput
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ), // history_color
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT ), // scratch_buffer
		};

	public:
		static const uint32_t DEFAULT_WIDTH = 2048, DEFAULT_HEIGHT = 2048;

		Attachment_Manager(VkDevice* device, vks::VulkanDevice* vulkanDevice, VkPhysicalDevice* physicalDevice, uint32_t width = DEFAULT_WIDTH, uint32_t height = DEFAULT_HEIGHT);
		~Attachment_Manager();

		inline VkExtent2D GetSize() const { return m_size; }

		void createAttachment(const AttachmentInitInfo& initInfo, FrameBufferAttachment* attachment);
		FrameBufferAttachment* getAttachment(Attachment);

		void createAllAttachments(uint32_t width, uint32_t height);
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
