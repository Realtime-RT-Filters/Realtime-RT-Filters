#include "..\headers\Renderpass.hpp"

namespace rtf
{

	Renderpass::Renderpass(VkInstance instance, vks::VulkanDevice* device)
		: m_Instance(instance), m_Device(device), m_Pipeline(nullptr), m_PipelineLayout(nullptr), m_Renderpass(nullptr)
	{}

	Renderpass::~Renderpass()
	{
		cleanUp();
	}

	void Renderpass::cleanUp()
	{}
}
