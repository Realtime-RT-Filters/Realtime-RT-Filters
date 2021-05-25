#include "../headers/Renderpass.hpp"

namespace rtf
{

	Renderpass::Renderpass(VkInstance instance, vks::VulkanDevice* device, Attachment_Manager* attachmentManager, RTFilterDemo* rtfilterdemo)
		: m_Instance(instance), m_Device(device), m_AttachmentManager(attachmentManager), m_Main(rtfilterdemo), m_Pipeline(nullptr), m_PipelineLayout(nullptr), m_Renderpass(nullptr), m_DescriptorSetLayout(nullptr)
	{}

	Renderpass::~Renderpass()
	{
		cleanUp();
	}

	void Renderpass::draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount)
	{
		out_commandBuffers = nullptr;
		out_commandBufferCount = 0;
	}

	void Renderpass::cleanUp()
	{}
}
