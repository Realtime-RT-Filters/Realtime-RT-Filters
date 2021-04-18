#ifndef Renderpass_Gbuffer_h
#define Renderpass_Gbuffer_h

#include "disable_warnings.h"
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

	struct UBO_Offscreen
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
		glm::vec4 instancePos[3];
	};

	class RenderpassGbuffer : public Renderpass
	{
	public:
		int32_t
			m_Width,
			m_Height;
		VkFramebuffer m_FrameBuffer = nullptr;
		FrameBufferAttachment
			m_Position = {},
			m_Normal = {},
			m_Albedo = {},
			m_Velocity = {},
			m_ObjectId = {},
			m_Depth = {};
		VkSampler m_ColorSampler = {};

		vks::Buffer m_Buffer;
		UBO_Offscreen m_UBO_Offscreen;

		RenderpassGbuffer(VkInstance instance, vks::VulkanDevice* device, int32_t width, int32_t height);

		// Inherited via Renderpass
		virtual void prepare() override;
		void prepareRenderpass();
		void prepareAttachments();
		void prepareUBOs();
		void updateUniformBuffer();
		virtual void draw() override;
		virtual void cleanUp() override;

	protected:
		void createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment* attachment);
	};
}

#endif Renderpass_Gbuffer_h