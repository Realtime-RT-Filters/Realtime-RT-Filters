#ifndef Renderpass_Gui_h
#define Renderpass_Gui_h

#include "disable_warnings.h"
#include <VulkanDevice.h>
#include "Attachment_Manager.hpp"
#include "Renderpass.hpp"

#include "vulkanexamplebase.h"

namespace rtf
{
	/// <summary>
	/// Class which acts as final render pass, showing output with UI
	/// </summary>
    class Renderpass_Gui : public Renderpass
	{
	protected:

		
		FrameBufferAttachment* m_position, * m_normal, * m_albedo, * m_motionvector, * m_rtoutput, * m_filteroutput;
		
		VkDescriptorSet descriptorSetInputAttachments;


		// One sampler for the frame buffer color attachments
		VkSampler colorSampler;

		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;


		void prepare();
		void setupDescriptorSetLayout();
		void setupDescriptorPool();
		void setupDescriptorSet();

	public:
		Renderpass_Gui(VkInstance instance, vks::VulkanDevice* device, Attachment_Manager* attachmentManager, RTFilterDemo* demo);
		~Renderpass_Gui();

		void buildCommandBuffer();


		virtual void draw(VkQueue queue) override;
	};
}

#endif //Renderpass_Gui_h
