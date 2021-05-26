#include "../headers/Renderpass_Gui.hpp"

namespace rtf
{


	Renderpass_Gui::Renderpass_Gui(VkInstance instance, vks::VulkanDevice* device, Attachment_Manager* attachment_manager, RTFilterDemo* demo)
		: Renderpass(instance, device, attachment_manager, demo)
	{
	}

	Renderpass_Gui::~Renderpass_Gui()
	{
	}

	void Renderpass_Gui::buildCommandBuffer()
	{
	}

	void Renderpass_Gui::draw(VkQueue queue)
	{
	}

	void Renderpass_Gui::prepare()
	{
		//Get all the needed attachments from the attachment manager
		m_position = m_AttachmentManager->getAttachment(Attachment::position);
		m_normal = m_AttachmentManager->getAttachment(Attachment::normal);
		m_albedo = m_AttachmentManager->getAttachment(Attachment::albedo);
		m_motionvector = m_AttachmentManager->getAttachment(Attachment::motionvector);
		m_rtoutput = m_AttachmentManager->getAttachment(Attachment::rtoutput);
		m_filteroutput = m_AttachmentManager->getAttachment(Attachment::filteroutput);



		// Create sampler to sample from the color attachments
		VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_NEAREST;
		sampler.minFilter = VK_FILTER_NEAREST;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 1.0f;
		sampler.minLod = 0.0f;
		sampler.maxLod = 1.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(m_Device->logicalDevice, &sampler, nullptr, &colorSampler));




	}

	void Renderpass_Gui::setupDescriptorSetLayout()
	{
	}




	void Renderpass_Gui::setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 3);
		VK_CHECK_RESULT(vkCreateDescriptorPool(m_Device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

	}

	void Renderpass_Gui::setupDescriptorSet()
	{

		// Image descriptors for the color attachments
		VkDescriptorImageInfo texDescriptorPosition =
			vks::initializers::descriptorImageInfo(
				colorSampler,
				m_position->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo texDescriptorNormal =
			vks::initializers::descriptorImageInfo(
				colorSampler,
				m_normal->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo texDescriptorAlbedo =
			vks::initializers::descriptorImageInfo(
				colorSampler,
				m_albedo->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo texDescriptorMotionVector =
			vks::initializers::descriptorImageInfo(
				colorSampler,
				m_motionvector->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo texDescriptorRTOutput =
			vks::initializers::descriptorImageInfo(
				colorSampler,
				m_rtoutput->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo texDescriptorFilterOutput =
			vks::initializers::descriptorImageInfo(
				colorSampler,
				m_filteroutput->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


		//VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayoutGBuffer, 1);

		//VK_CHECK_RESULT(vkAllocateDescriptorSets(*m_device, &allocInfo, &descriptorSetInputAttachments));
	}

}
