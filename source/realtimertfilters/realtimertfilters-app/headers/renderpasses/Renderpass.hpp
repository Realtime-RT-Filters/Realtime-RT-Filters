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
	class Renderpass
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
		vks::VulkanDevice* m_vulkanDevice;
		Attachment_Manager* m_attachmentManager;

		VkRenderPass m_renderpass{};

		// Pipeline
		VkPipeline m_pipeline{};
		VkPipelineLayout m_pipelineLayout{};

		//DescriptorSet
		VkDescriptorPool m_descriptorPool{};
		VkDescriptorSetLayout m_descriptorSetLayout{};
		VkDescriptorSet m_descriptorSet{};

		RTFilterDemo* m_rtFilterDemo{};

		inline VkDevice getLogicalDevice() { return m_vulkanDevice->logicalDevice; }
	};

	// SharedPtr<Renderpass>
	using RenderpassPtr = std::shared_ptr<Renderpass>;
	// A vector of renderpasses
	using QueueTemplate = std::vector<RenderpassPtr>;
	// SharedPtr to a vector of renderpasses
	using QueueTemplatePtr = std::shared_ptr<QueueTemplate>;
}

#endif //Renderpass_h
