#ifndef Renderpass_h
#define Renderpass_h

#include "disable_warnings.h"
#include <VulkanDevice.h>
//#include "Attachment_Manager.hpp"


namespace rtf
{
	class Attachment_Manager;
	class RTFilterDemo;

	/// <summary>
	/// Abstract class to serve as a base for all renderpasses
	/// </summary>
	class Renderpass // abstract is MS specific
	{
	protected:
		// Vulkan Environment
		//VkInstance m_Instance;
		vks::VulkanDevice* m_Device;
		Attachment_Manager* m_AttachmentManager;
		RTFilterDemo* m_Main;

		// Pipeline
		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;

		VkRenderPass m_Renderpass;


		//DescriptorSet
		VkDescriptorPool m_DescriptorPool = nullptr;
		VkDescriptorSetLayout m_DescriptorSetLayout;

	public:
		
		Renderpass(VkInstance instance, vks::VulkanDevice* device, Attachment_Manager* attachmentManager, RTFilterDemo* rtfilterdemo);
		Renderpass(RTFilterDemo* demo);
		virtual ~Renderpass();

		// Member level copy of a renderpass object makes no sense, so we delete auto-generated constructors and operators associated with it
		Renderpass(Renderpass& other) = delete;
		Renderpass(Renderpass&& other) = delete;
		void operator=(Renderpass& other) = delete;


		virtual void prepare() = 0; // Setup pipelines, passes, descriptorsets, etc.
		virtual void draw(VkQueue queue); // Push your shit onto the queue
		virtual void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount);
		virtual void cleanUp(); // Cleanup any mess you made (is called from the destructor)

		inline VkDevice LogicalDevice() { return m_Device->logicalDevice; }
	};
}

#endif //Renderpass_h
