#ifndef Renderpass_Filter_h
#define Renderpass_Filter_h

#include "disable_warnings.h"
#include <VulkanDevice.h>
#include "Attachment_Manager.hpp"

namespace rtf
{
	/// <summary>
	/// Class which acts as final render pass, showing output with UI
	/// </summary>
    class Renderpass_Filter
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
		Renderpass_Filter(VkDevice* device, Attachment_Manager* attachment_manager);
		virtual ~Renderpass_Filter();




	};
}

#endif //Renderpass_Gui_h
