#ifndef Renderpass_Gui_h
#define Renderpass_Gui_h

#include "disable_warnings.h"
#include <VulkanDevice.h>
#include "Attachment_Manager.hpp"

namespace rtf
{
	/// <summary>
	/// Class which acts as final render pass, showing output with UI
	/// </summary>
    class Renderpass_Gui
	{
	protected:
		// Vulkan Environment
		VkInstance* m_Instance;
		vks::VulkanDevice* m_Device;


		// Pipeline
		VkPipelineLayout m_PipelineLayout;
		VkRenderPass m_Renderpass;
		
		VkPipeline m_Pipeline;
		VkDescriptorSetLayout m_DescriptorSetLayout;

		Attachment_Manager* m_attachment_manager;


		struct FrameBuffer
		{
			int32_t width, height;
			VkFramebuffer frameBuffer;
			FrameBufferAttachment* position, * normal, * albedo;
			VkRenderPass renderPass;
		} offScreenFrameBuf;

		void prepare();

	public:
		Renderpass_Gui(VkDevice* device, Attachment_Manager* attachment_manager);
		virtual ~Renderpass_Gui();




	};
}

#endif //Renderpass_Gui_h
