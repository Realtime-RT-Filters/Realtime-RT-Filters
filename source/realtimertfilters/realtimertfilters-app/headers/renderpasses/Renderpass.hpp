#ifndef Renderpass_h
#define Renderpass_h

#include "../disable_warnings.h"
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
	public:
		
		Renderpass() = default;
		virtual ~Renderpass();

		// Member level copy of a renderpass object makes no sense, so we delete auto-generated constructors and operators associated with it
		Renderpass(Renderpass& other) = delete;
		Renderpass(Renderpass&& other) = delete;
		void operator=(Renderpass& other) = delete;

		void setRtFilterDemo(RTFilterDemo* rtFilterDemo);
		
		virtual void prepare() = 0; // Setup pipelines, passes, descriptorsets, etc.
		virtual void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) = 0;
		virtual void cleanUp() {}; // Cleanup any mess you made (is called from the destructor)
		virtual void updateUniformBuffer() {};

	protected:
		// Vulkan Environment
		//VkInstance m_Instance;
		vks::VulkanDevice* m_vulkanDevice;
		Attachment_Manager* m_attachmentManager;

		// Pipeline
		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;

		VkRenderPass m_Renderpass;


		//DescriptorSet
		VkDescriptorPool m_DescriptorPool = nullptr;
		VkDescriptorSetLayout m_DescriptorSetLayout;

		RTFilterDemo* m_rtFilterDemo{};
	};
}

#endif //Renderpass_h