#include "..\headers\Renderpass.hpp"

rtf::Renderpass::Renderpass(VkInstance instance, vks::VulkanDevice* device)
	: Instance(instance), Device(device), Pipeline(nullptr), PipelineLayout(nullptr), RenderPass(nullptr)
{}

rtf::Renderpass::~Renderpass()
{}

void rtf::Renderpass::CleanUp()
{
	CleanUp();
}
