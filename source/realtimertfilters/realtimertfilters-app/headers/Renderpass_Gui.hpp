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
		vks::VulkanDevice* m_Device;

		Attachment_Manager* m_attachment_manager;


		// Pipeline
		VkPipelineLayout m_PipelineLayout;
		
		VkPipeline m_Pipeline;
		VkDescriptorSetLayout m_DescriptorSetLayout;

		VkRenderPass m_renderPass;

		VkFramebuffer m_frameBuffer;
		
		FrameBufferAttachment* m_position, * m_normal, * m_albedo, * m_motionvector, * m_rtouput, * m_filteroutput;
		
		// One sampler for the frame buffer color attachments
		VkSampler colorSampler;


		void prepare();

	public:
		Renderpass_Gui(VkDevice* device, Attachment_Manager* attachment_manager);
		virtual ~Renderpass_Gui();




	};
}

#endif //Renderpass_Gui_h
