#include "../headers/TextureBinding.hpp"

namespace rtf
{
	TextureBinding::TextureBinding(
		Attachment				attachmentid,
		Type					type,
		FrameBufferAttachment* attachment,
		VkImageLayout			prelayout,
		VkImageLayout			worklayout,
		VkImageLayout			postlayout,
		VkImageAspectFlags		aspectflags
	)
		: m_AttachmentId(attachmentid), m_Type(type), m_Attachment(attachment), m_PreLayout(prelayout), m_WorkLayout(worklayout), m_PostLayout(postlayout), m_AspectMask(aspectflags), m_ImageView()
	{}

	void TextureBinding::resolveAttachment(vks::VulkanDevice* vulkanDevice, Attachment_Manager* manager)
	{
		m_Attachment = manager->getAttachment(m_AttachmentId);
		if (m_AspectMask != 0)
		{
			createImageview(vulkanDevice);
		}
		else
		{
			m_ImageView = m_Attachment->view;
		}
	}

	inline void rtf::TextureBinding::createImageview(vks::VulkanDevice* vulkanDevice)
	{
		VkImageViewCreateInfo viewCI = vks::initializers::imageViewCreateInfo();
		viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCI.format = m_Attachment->format;
		viewCI.subresourceRange = {};
		viewCI.subresourceRange.aspectMask = m_AspectMask;
		viewCI.subresourceRange.baseMipLevel = 0;
		viewCI.subresourceRange.levelCount = 1;
		viewCI.subresourceRange.baseArrayLayer = 0;
		viewCI.subresourceRange.layerCount = 1;
		viewCI.image = m_Attachment->image;
		VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &viewCI, nullptr, &m_ImageView));
	}

	void TextureBinding::destroyImageview(vks::VulkanDevice* vulkanDevice)
	{
		if (m_ImageView != nullptr)
		{
			vkDestroyImageView(vulkanDevice->logicalDevice, m_ImageView, nullptr);
		}
	}


	VkDescriptorSetLayoutBinding TextureBinding::makeDescriptorSetLayoutBinding(uint32_t binding) const
	{
		VkDescriptorSetLayoutBinding result{};
		result.binding = binding;
		result.descriptorType = getDescriptorType();
		result.descriptorCount = 1;
		result.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		return result;
	}

	VkDescriptorImageInfo TextureBinding::makeDescriptorImageInfo(VkSampler sampler) const
	{
		if (m_ImageView == nullptr)
		{
			return VkDescriptorImageInfo{ selectSampler(sampler), m_Attachment->view, m_WorkLayout };
		}
		else
		{
			return VkDescriptorImageInfo{ selectSampler(sampler), m_ImageView, m_WorkLayout };
		}
	}

	VkWriteDescriptorSet TextureBinding::makeWriteDescriptorSet(VkDescriptorSet descrSet, uint32_t binding, VkDescriptorImageInfo* imageInfo) const
	{
		VkWriteDescriptorSet result{};
		result.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		result.dstSet = descrSet;
		result.dstBinding = binding;
		result.descriptorCount = 1;
		result.descriptorType = getDescriptorType();
		result.pImageInfo = imageInfo;
		return result;
	}

	void TextureBinding::makeImageTransition(VkCommandBuffer commandBuffer) const
	{
		VkImageCreateFlags mask = m_AspectMask;
		if (mask == 0)
		{
			mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		}
		vks::tools::setImageLayout(commandBuffer, m_Attachment->image, mask, m_PreLayout, m_PostLayout);
	}

	void TextureBinding::FillLayoutBindingVector(std::vector<VkDescriptorSetLayoutBinding>& vector, const TextureBinding* data, uint32_t count, uint32_t baseBinding)
	{
		vector.reserve(count);
		uint32_t binding = baseBinding;
		for (uint32_t i = 0; i < count; i++)
		{
			const TextureBinding& texBinding = data[i];
			if (!texBinding.usesDescriptor())
			{
				continue;
			}
			vector.push_back(texBinding.makeDescriptorSetLayoutBinding(binding));
			binding++;
		}
	}

	void TextureBinding::CreateDescriptorSetLayout(VkDevice logicalDevice, VkDescriptorSetLayout& out, const TextureBinding* data, uint32_t count)
	{
		std::vector<VkDescriptorSetLayoutBinding> vector{};
		FillLayoutBindingVector(vector, data, count);
		VkDescriptorSetLayoutCreateInfo createinfo{};
		createinfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createinfo.bindingCount = vector.size();
		createinfo.pBindings = vector.data();
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(logicalDevice, &createinfo, nullptr, &out));
	}

	void TextureBinding::FillPoolSizesVector(std::vector<VkDescriptorPoolSize>& vector, const TextureBinding* data, uint32_t count)
	{
		const size_t INDEX_STORAGEIMAGE = 0;
		const size_t INDEX_SAMPLER = 1;
		uint32_t counts[2]{};

		for (uint32_t i = 0; i < count; i++)
		{
			const TextureBinding& texBinding = data[i];
			switch (texBinding.getDescriptorType())
			{
			case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				counts[INDEX_STORAGEIMAGE]++;
				break;
			case VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				counts[INDEX_SAMPLER]++;
				break;
			default:
				continue;
			}
		}

		if (counts[INDEX_STORAGEIMAGE] > 0)
		{
			vector.push_back(VkDescriptorPoolSize{ VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, counts[INDEX_STORAGEIMAGE] });
		}
		if (counts[INDEX_SAMPLER] > 0)
		{
			vector.push_back(VkDescriptorPoolSize{ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, counts[INDEX_SAMPLER] });
		}
	}

	void TextureBinding::CreateDescriptorPool(VkDevice logicalDevice, VkDescriptorPool& out, const TextureBinding* data, uint32_t count, uint32_t maxSets)
	{
		std::vector<VkDescriptorPoolSize> vector{};
		FillPoolSizesVector(vector, data, count);
		VkDescriptorPoolCreateInfo createinfo{};
		createinfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createinfo.maxSets = maxSets;
		createinfo.poolSizeCount = vector.size();
		createinfo.pPoolSizes = vector.data();
		VK_CHECK_RESULT(vkCreateDescriptorPool(logicalDevice, &createinfo, nullptr, &out));
	}

	void TextureBinding::FillWriteDescriptorSetStructures(std::vector<VkDescriptorImageInfo>& imageInfos, std::vector<VkWriteDescriptorSet>& writes, const TextureBinding* data, uint32_t count, VkDescriptorSet descrSet, VkSampler sampler, uint32_t baseBinding)
	{
		imageInfos.reserve(count);
		writes.reserve(count);

		uint32_t binding = baseBinding;
		for (uint32_t i = 0; i < count; i++)
		{
			const TextureBinding& texBinding = data[i];
			if (!texBinding.usesDescriptor())
			{
				continue;
			}
			imageInfos.push_back(texBinding.makeDescriptorImageInfo(sampler));
			writes.push_back(texBinding.makeWriteDescriptorSet(descrSet, binding, &imageInfos.back()));
			binding++;
		}
	}

	void TextureBinding::UpdateDescriptorSet(VkDevice logicalDevice, const TextureBinding* data, uint32_t count, VkDescriptorSet descrSet, VkSampler sampler, uint32_t baseBinding)
	{
		std::vector<VkDescriptorImageInfo> imageInfos{};
		std::vector<VkWriteDescriptorSet> writes{};
		FillWriteDescriptorSetStructures(imageInfos, writes, data, count, descrSet, sampler, baseBinding);
		vkUpdateDescriptorSets(logicalDevice, writes.size(), writes.data(), 0, nullptr);
	}

	void TextureBinding::FillAttachmentDescriptionStructures(std::vector<VkAttachmentDescription>& descriptions, std::vector<VkAttachmentReference>& inputrefs, std::vector<VkAttachmentReference>& outputrefs, const TextureBinding* data, uint32_t count)
	{
		VkAttachmentDescription attachmentDescription{};
		attachmentDescription.flags = 0;
		attachmentDescription.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		for (uint32_t i = 0; i < count; i++)
		{
			const TextureBinding& texBinding = data[i];
			if (!texBinding.usesAttachmentDescription())
			{
				continue; // StorageImages don't need texBinding descriptions
			}

			attachmentDescription.format = texBinding.m_Attachment->format;

			attachmentDescription.initialLayout = texBinding.m_WorkLayout;
			attachmentDescription.finalLayout = texBinding.m_PostLayout;
			if (texBinding.readAccess())
			{
				attachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
				attachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
				inputrefs.push_back(VkAttachmentReference{ static_cast<uint32_t>(descriptions.size()), texBinding.m_WorkLayout });
			}
			else if (texBinding.writeAccess())
			{
				attachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
				outputrefs.push_back(VkAttachmentReference{ static_cast<uint32_t>(descriptions.size()), texBinding.m_WorkLayout });
			}
			descriptions.push_back(attachmentDescription);
		};
	}
}