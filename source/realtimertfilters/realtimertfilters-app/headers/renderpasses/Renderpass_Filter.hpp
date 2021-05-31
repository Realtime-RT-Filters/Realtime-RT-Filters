#ifndef Renderpass_Filter_h
#define Renderpass_Filter_h

#include "../disable_warnings.h"
#include <VulkanDevice.h>
#include "../Attachment_Manager.hpp"
#include "Renderpass.hpp"

namespace rtf
{
	/// <summary>
	/// Class which acts as a example filter pass
	/// </summary>
    class Renderpass_Filter : public Renderpass
	{

	public:
		Renderpass_Filter();
		virtual ~Renderpass_Filter();

		virtual void prepare() override;
		virtual void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount);
		virtual void cleanUp() override;

	protected:
		struct FrameBuffer
		{
			int32_t width, height;
			VkFramebuffer frameBuffer;
			FrameBufferAttachment* position, * normal, * albedo;
			VkRenderPass renderPass;
		} offScreenFrameBuf;


	};
}

#endif //Renderpass_Filter_h
