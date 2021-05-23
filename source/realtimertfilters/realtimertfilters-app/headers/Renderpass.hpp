#ifndef Renderpass_h
#define Renderpass_h

#include "disable_warnings.h"
#include <VulkanDevice.h>

namespace rtf
{
	/// <summary>
	/// Abstract class to serve as a base for all renderpasses
	/// </summary>
        class Renderpass // abstract is MS specific
	{
	protected:
		// Vulkan Environment
		VkInstance m_Instance;
		vks::VulkanDevice* m_Device;

		// Pipeline
		VkPipelineLayout m_PipelineLayout;
		VkRenderPass m_Renderpass;
		
		VkPipeline m_Pipeline;
		VkDescriptorSetLayout m_DescriptorSetLayout;

	public:
		Renderpass(VkInstance instance, vks::VulkanDevice* device);
		virtual ~Renderpass();

		Renderpass(Renderpass& other) = delete;
		Renderpass(Renderpass&& other) = delete;
		void operator=(Renderpass& other) = delete;


		virtual void prepare() = 0;
		virtual void draw(VkCommandBuffer& commandBuffer) = 0;
		virtual void cleanUp();
	};
}

#endif //Renderpass_h
