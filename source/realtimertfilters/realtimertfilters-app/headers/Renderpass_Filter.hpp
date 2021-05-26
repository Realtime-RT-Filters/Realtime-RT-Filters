#ifndef Renderpass_Filter_h
#define Renderpass_Filter_h

#include "disable_warnings.h"
#include <VulkanDevice.h>
#include "Attachment_Manager.hpp"
#include "Renderpass.hpp"

namespace rtf
{
	/// <summary>
	/// Class which acts as a example filter pass
	/// </summary>
    class Renderpass_Filter : public Renderpass
	{
	protected:


		struct FrameBuffer
		{
			int32_t width, height;
			VkFramebuffer frameBuffer;
			FrameBufferAttachment* position, * normal, * albedo;
			VkRenderPass renderPass;
		} offScreenFrameBuf;

		void prepare();

	public:
		Renderpass_Filter(VkInstance instance, vks::VulkanDevice* device, Attachment_Manager* attachmentManager, RTFilterDemo* demo);
		virtual ~Renderpass_Filter();


	};
}

#endif //Renderpass_Filter_h
