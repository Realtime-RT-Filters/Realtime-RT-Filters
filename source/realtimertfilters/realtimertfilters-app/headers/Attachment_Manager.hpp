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
		VkImage image{};
		VkDeviceMemory mem{};
		VkImageView view{};
		VkFormat format{};
	};

	class AttachmentInitInfo
	{
	public:
		VkFormat m_Format{};
		VkImageUsageFlags m_UsageFlags{};
		VkExtent2D m_Size{};
		VkImageLayout m_InitialLayout;

		AttachmentInitInfo() = default;
		AttachmentInitInfo(VkFormat format, VkImageUsageFlags flags, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_GENERAL, VkExtent2D size = VkExtent2D{0, 0}) : m_Format(format), m_UsageFlags(flags), m_Size(size), m_InitialLayout(initialLayout) {};
	};

	/// <summary>
	/// Class managing attachments for all render passes
	/// </summary>
	class Attachment_Manager
	{
		static const VkImageUsageFlags DEFAULTFLAGS =
			VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |		// Load into GUI & GBuffer shader with this
			VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT;				// Use as StorageImage, for RT and Postprocessing Shader

		std::vector<AttachmentInitInfo> m_attachmentTypes = {
			AttachmentInitInfo(VK_FORMAT_R16G16B16A16_SFLOAT, DEFAULTFLAGS), // position
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, DEFAULTFLAGS), // normal
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, DEFAULTFLAGS), // albedo
			AttachmentInitInfo( VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ), // depth
			AttachmentInitInfo( VK_FORMAT_R32_SINT, DEFAULTFLAGS), // meshid
			AttachmentInitInfo( VK_FORMAT_R32G32_SFLOAT, DEFAULTFLAGS), // motionvector
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, DEFAULTFLAGS), // rtoutput
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, DEFAULTFLAGS), // filteroutput
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, DEFAULTFLAGS), // history_color
			AttachmentInitInfo( VK_FORMAT_R16G16B16A16_SFLOAT, DEFAULTFLAGS), // scratch_buffer
		};

	public:
		static const uint32_t DEFAULT_WIDTH = 2048, DEFAULT_HEIGHT = 2048;

		Attachment_Manager(vks::VulkanDevice* vulkanDevice, VkQueue graphicsQueue, uint32_t width = DEFAULT_WIDTH, uint32_t height = DEFAULT_HEIGHT);
		~Attachment_Manager();

		inline VkExtent2D GetSize() const { return m_size; }

		void createAttachment(VkCommandBuffer cmdBuffer, const AttachmentInitInfo& initInfo, FrameBufferAttachment* attachment);
		FrameBufferAttachment* getAttachment(Attachment);
		void getAllAttachments(FrameBufferAttachment*& out_arr, size_t& out_count);

		void createAllAttachments();
		void destroyAllAttachments();

		void resize(VkExtent2D newsize);

	private:

		void destroyAttachment(FrameBufferAttachment* attachment);

		//vks::Vkdevice is a combined logical/physical vulkan device
		vks::VulkanDevice* m_vulkanDevice;
		VkQueue m_queue;

		// Keeping track of current attachment size
		VkExtent2D m_size;

		// List of managed Attachments
		static const int m_maxAttachmentSize = (int)Attachment::max_attachments;
		FrameBufferAttachment m_attachments[m_maxAttachmentSize]{};
	};
}

#endif //Attachment_Manager_h
