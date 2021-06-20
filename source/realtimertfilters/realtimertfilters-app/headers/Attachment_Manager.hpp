#ifndef Attachment_Manager_h
#define Attachment_Manager_h

#include "disable_warnings.h"
#include <VulkanDevice.h>

#include <array>
#include <utility>

namespace rtf
{

	//These are global types needed by other classes
	enum class Attachment
	{
		// GBUFFER
		position,
		normal,
		albedo,
		depth,
		meshid,
		motionvector,
		// PATHTRACER
		rtoutput,
		rtdirect,
		rtindirect,
		// TEMPORAL ACCUMULATION
		prev_position,
		prev_normal,
		prev_accumulatedcolor,
		direct_color_history,
		indirect_color_history,
		moments_history,
		new_moments,
		prev_historylength,
		new_historylength,
		atrous_integratedDirectColor_A,
		atrous_integratedDirectColor_B,
		atrous_integratedIndirectColor_A,
		atrous_integratedIndirectColor_B,
		svgf_output,

		// MISC
		intermediate,
		filteroutput,
		// BMFR
		scratch_buffer,
		prev_accumulatedregression,
		compute_output,
		// SVGF
		atrous_output,
		atrous_intermediate,

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
		Attachment m_AttachmentId{};
		VkFormat m_Format{};
		VkImageUsageFlags m_UsageFlags{};
		VkExtent2D m_Size{};
		VkImageLayout m_InitialLayout;

		AttachmentInitInfo() = default;
		AttachmentInitInfo(Attachment attachmentid, VkFormat format, VkImageUsageFlags flags, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_GENERAL, VkExtent2D size = VkExtent2D{ 0, 0 }) : m_AttachmentId(attachmentid), m_Format(format), m_UsageFlags(flags), m_Size(size), m_InitialLayout(initialLayout) {};
	};

	/// <summary>
	/// Class managing attachments for all render passes
	/// </summary>
	class Attachment_Manager
	{

		static const VkFormat DEFAULT_COLOR_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
		static const VkFormat DEFAULT_GEOMETRY_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;

		static const VkImageUsageFlags DEFAULTFLAGS =
			VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |		// Load into GUI & GBuffer shader with this
			VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT |				// Use as StorageImage, for RT and Postprocessing Shader
			VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		std::vector<AttachmentInitInfo> m_attachmentInitsUnsorted = {
			// GBuffer
			AttachmentInitInfo(Attachment::position, DEFAULT_GEOMETRY_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::normal,  DEFAULT_GEOMETRY_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::albedo,  DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::depth,  VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL),
			AttachmentInitInfo(Attachment::meshid,  VK_FORMAT_R32_SINT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::motionvector,  VK_FORMAT_R32G32_SFLOAT, DEFAULTFLAGS),
			// Pathtracer
			AttachmentInitInfo(Attachment::rtoutput,  DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::rtdirect,  DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::rtindirect,  DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			// Temporal Accumulation
			AttachmentInitInfo(Attachment::prev_position, DEFAULT_GEOMETRY_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::prev_normal, DEFAULT_GEOMETRY_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::prev_accumulatedcolor, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::direct_color_history, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::indirect_color_history, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::moments_history, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::new_moments, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::prev_historylength, VK_FORMAT_R16_SINT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::new_historylength, VK_FORMAT_R16_SINT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::atrous_integratedDirectColor_A, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::atrous_integratedDirectColor_B, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::atrous_integratedIndirectColor_A, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::atrous_integratedIndirectColor_B, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::svgf_output,  DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			// Misc
			AttachmentInitInfo(Attachment::filteroutput,  DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::intermediate,  DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			// BMFR
			AttachmentInitInfo(Attachment::scratch_buffer, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::prev_accumulatedregression, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::compute_output, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			// SVGF
			AttachmentInitInfo(Attachment::atrous_output, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			AttachmentInitInfo(Attachment::atrous_intermediate, DEFAULT_COLOR_FORMAT, DEFAULTFLAGS),
			
		};
		std::array<AttachmentInitInfo, (size_t)Attachment::max_attachments> m_attachmentInits{};

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
