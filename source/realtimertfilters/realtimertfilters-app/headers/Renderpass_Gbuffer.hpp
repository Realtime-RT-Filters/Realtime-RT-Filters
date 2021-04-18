#ifndef Renderpass_Gbuffer_h
#define Renderpass_Gbuffer_h

#include "Renderpass.hpp"

namespace rtf
{
	struct FrameBufferAttachment
	{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
		VkFormat format;
	};

	class RenderpassGbuffer : public Renderpass
	{
	public:
		int32_t
			Width = 0,
			Height = 0;
		VkFramebuffer FrameBuffer = nullptr;
		FrameBufferAttachment
			Position = {},
			Normal = {},
			Albedo = {},
			Velocity = {},
			ObjectId = {},
			Depth = {};


		RenderpassGbuffer(VkInstance instance, vks::VulkanDevice* device);

		// Inherited via Renderpass
		virtual void Prepare() override;
		void prepareAttachments();
		virtual void Draw() override;
		virtual void CleanUp() override;

	protected:
		void createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment* attachment);
	};
}

#endif Renderpass_Gbuffer_h