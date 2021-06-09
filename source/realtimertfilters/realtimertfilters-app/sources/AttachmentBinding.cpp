#include "../headers/AttachmentBinding.hpp"

namespace rtf
{
	AttachmentBinding::AttachmentBinding(Attachment attachmentid, AccessMode usetype, BindType bindtype, FrameBufferAttachment* attachment, SamplerType samplertype, VkImageLayout prelayout, VkImageLayout worklayout, VkImageLayout postlayout, VkImageAspectFlags aspectflags)
		: m_AttachmentId(attachmentid), m_Attachment(attachment), m_AccessMode(usetype), m_Bind(bindtype), m_PreLayout(prelayout), m_Sampler(samplertype), m_WorkLayout(worklayout), m_PostLayout(postlayout), m_AspectMask(aspectflags)
	{
	}

	void AttachmentBinding::resolveAttachment(vks::VulkanDevice* vulkanDevice, Attachment_Manager* manager)
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

	inline void rtf::AttachmentBinding::createImageview(vks::VulkanDevice* vulkanDevice)
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


	VkDescriptorSetLayoutBinding AttachmentBinding::makeDescriptorSetLayoutBinding(uint32_t binding) const
	{
		VkDescriptorSetLayoutBinding result{};
		result.binding = binding;
		result.descriptorType = getDescriptorType();
		result.descriptorCount = 1;
		result.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		return result;
	}

	VkDescriptorImageInfo AttachmentBinding::makeDescriptorImageInfo(VkSampler direct, VkSampler normalized) const
	{
		if (m_ImageView == nullptr)
		{
			return VkDescriptorImageInfo{ selectSampler(direct, normalized), m_Attachment->view, m_WorkLayout };
		}
		else
		{
			return VkDescriptorImageInfo{ selectSampler(direct, normalized), m_ImageView, m_WorkLayout };
		}
	}

	VkWriteDescriptorSet AttachmentBinding::makeWriteDescriptorSet(VkDescriptorSet descrSet, uint32_t binding, VkDescriptorImageInfo* imageInfo) const
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

	void AttachmentBinding::makeImageTransition(VkCommandBuffer commandBuffer) const
	{
		VkImageCreateFlags mask = m_AspectMask;
		if (mask == 0)
		{
			mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		}
		vks::tools::setImageLayout(commandBuffer, m_Attachment->image, mask, m_PreLayout, m_PostLayout);
	}

	void AttachmentBinding::FillLayoutBindingVector(std::vector<VkDescriptorSetLayoutBinding>& vector, const AttachmentBinding* data, uint32_t count, uint32_t baseBinding)
	{
		vector.reserve(count);
		uint32_t binding = baseBinding;
		for (uint32_t i = 0; i < count; i++)
		{
			const AttachmentBinding& attachment = data[i];
			if (!attachment.usesDescriptor())
			{
				continue;
			}
			vector.push_back(attachment.makeDescriptorSetLayoutBinding(binding));
			binding++;
		}
	}

	void AttachmentBinding::CreateDescriptorSetLayout(VkDevice logicalDevice, VkDescriptorSetLayout& out, const AttachmentBinding* data, uint32_t count)
	{
		std::vector<VkDescriptorSetLayoutBinding> vector{};
		FillLayoutBindingVector(vector, data, count);
		VkDescriptorSetLayoutCreateInfo createinfo{};
		createinfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createinfo.bindingCount = vector.size();
		createinfo.pBindings = vector.data();
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(logicalDevice, &createinfo, nullptr, &out));
	}

	void AttachmentBinding::FillPoolSizesVector(std::vector<VkDescriptorPoolSize>& vector, const AttachmentBinding* data, uint32_t count)
	{
		uint32_t counts[2]{};

		for (uint32_t i = 0; i < count; i++)
		{
			const AttachmentBinding& attachment = data[i];
			counts[(size_t)attachment.m_Bind]++;
		}

		if (counts[(size_t)BindType::StorageImage] > 0)
		{
			vector.push_back(VkDescriptorPoolSize{ VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, counts[(size_t)BindType::StorageImage] });
		}
		if (counts[(size_t)BindType::Sampled] > 0)
		{
			vector.push_back(VkDescriptorPoolSize{ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, counts[(size_t)BindType::Sampled] });
		}
	}

	void AttachmentBinding::CreateDescriptorPool(VkDevice logicalDevice, VkDescriptorPool& out, const AttachmentBinding* data, uint32_t count, uint32_t maxSets)
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

	void AttachmentBinding::FillWriteDescriptorSetStructures(std::vector<VkDescriptorImageInfo>& imageInfos, std::vector<VkWriteDescriptorSet>& writes, const AttachmentBinding* data, uint32_t count, VkDescriptorSet descrSet, VkSampler direct, VkSampler normalized, uint32_t baseBinding)
	{
		imageInfos.reserve(count);
		writes.reserve(count);

		uint32_t binding = baseBinding;
		for (uint32_t i = 0; i < count; i++)
		{
			const AttachmentBinding& attachment = data[i];
			if (!attachment.usesDescriptor())
			{
				continue;
			}
			imageInfos.push_back(attachment.makeDescriptorImageInfo(direct, normalized));
			writes.push_back(attachment.makeWriteDescriptorSet(descrSet, binding, &imageInfos.back()));
			binding++;
		}
	}

	void AttachmentBinding::UpdateDescriptorSet(VkDevice logicalDevice, const AttachmentBinding* data, uint32_t count, VkDescriptorSet descrSet, VkSampler direct, VkSampler normalized, uint32_t baseBinding)
	{
		std::vector<VkDescriptorImageInfo> imageInfos{};
		std::vector<VkWriteDescriptorSet> writes{};
		FillWriteDescriptorSetStructures(imageInfos, writes, data, count, descrSet, direct, normalized, baseBinding);
		vkUpdateDescriptorSets(logicalDevice, writes.size(), writes.data(), 0, nullptr);
	}

	void AttachmentBinding::FillAttachmentDescriptionStructures(std::vector<VkAttachmentDescription>& descriptions, std::vector<VkAttachmentReference>& inputrefs, std::vector<VkAttachmentReference>& outputrefs, const AttachmentBinding* data, uint32_t count)
	{
		VkAttachmentDescription attachmentDescription{};
		attachmentDescription.flags = 0;
		attachmentDescription.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		for (uint32_t i = 0; i < count; i++)
		{
			const AttachmentBinding& attachment = data[i];
			if (!attachment.usesAttachmentDescription())
			{
				continue; // StorageImages don't need attachment descriptions
			}

			attachmentDescription.format = attachment.m_Attachment->format;

			attachmentDescription.initialLayout = attachment.m_WorkLayout;
			attachmentDescription.finalLayout = attachment.m_PostLayout;
			if (attachment.readAccess())
			{
				attachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
				attachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
				inputrefs.push_back(VkAttachmentReference{ static_cast<uint32_t>(descriptions.size()), attachment.m_WorkLayout });
			}
			else if (attachment.writeAccess())
			{
				attachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
				outputrefs.push_back(VkAttachmentReference{ static_cast<uint32_t>(descriptions.size()), attachment.m_WorkLayout });
			}
			descriptions.push_back(attachmentDescription);
		};
	}
}