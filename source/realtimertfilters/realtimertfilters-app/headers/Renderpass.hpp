#ifndef Renderpass_h
#define Renderpass_h

#include <VulkanRaytracingSample.h>

namespace rtf
{
	/// <summary>
	/// Abstract class to serve as a base for all renderpasses
	/// </summary>
	class Renderpass abstract
	{
	protected:
		// Vulkan Environment
		VkInstance Instance;
		vks::VulkanDevice* Device;

		// Pipeline
		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;
		VkRenderPass RenderPass;

	public:
		Renderpass(VkInstance instance, vks::VulkanDevice* device);
		~Renderpass();

		Renderpass(Renderpass& other) = delete;
		Renderpass(Renderpass&& other) = delete;
		void operator=(Renderpass& other) = delete;


		virtual void Prepare() = 0;
		virtual void Draw() = 0;
		virtual void CleanUp();
	};
}

enum UnscopedEnum
{
	E,
	F
};

enum class ScopedEnum
{
	T, 
	U
};

void Test()
{
}

#endif Renderpass_h