#include "../headers/Renderpass_PostProcess.hpp"
#include "../headers/VulkanglTFModel.h"
#include "../headers/RTFilterDemo.hpp"

namespace rtf
{
	RenderpassPostProcess::StaticsContainer* GlobalStatics;
	uint32_t InstanceCount;

	RenderpassPostProcess::RenderpassPostProcess(RTFilterDemo* demo)
		: Renderpass(demo)
	{}
	void RenderpassPostProcess::ConfigureShader(const std::string& shadername)
	{
		m_Shadername = shadername;

	}
	void RenderpassPostProcess::PushAttachment(FrameBufferAttachment* attachment, AttachmentUse use)
	{
		m_Attachments.push_back(AttachmentContainer{ attachment, use });
	}
	void RenderpassPostProcess::prepare()
	{
		// Assure the object has been properly configured
		assert(!m_Shadername.empty());
		m_CombinedAttachmentCount = m_Attachments.size();

		bool hasInput = false;
		bool hasOutput = false;
		for (int i = 0; i < m_CombinedAttachmentCount; i++)
		{
			switch (m_Attachments[i].m_Use)
			{
			case AttachmentUse::ReadOnly:
				hasInput = true;
				break;
			case AttachmentUse::WriteOnly:
				hasOutput = true;
				break;
			case AttachmentUse::ReadWrite:
				hasInput = true;
				hasOutput = true;
				break;
			}
		}
		assert(hasInput);
		assert(hasOutput);

		// Fetch our statics
		if (GlobalStatics == nullptr)
		{
			GlobalStatics = new StaticsContainer(m_Main);
			InstanceCount = 0;
		}
		Statics = GlobalStatics;
		InstanceCount++;

		createRenderPass();
		setupFramebuffer();
		setupDescriptorSetLayout();
		setupDescriptorSet();
		setupPipeline();
		buildCommandBuffer();
	}

	void RenderpassPostProcess::draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount)
	{
		out_commandBufferCount = 1;
		out_commandBuffers = &m_CmdBuffer;
	}

	void RenderpassPostProcess::cleanUp()
	{
		vkDestroyRenderPass(LogicalDevice(), m_Renderpass, nullptr);
		vkDestroyPipeline(LogicalDevice(), m_Pipeline, nullptr);
		vkDestroyPipelineLayout(LogicalDevice(), m_PipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(LogicalDevice(), m_DescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(LogicalDevice(), m_DescriptorPool, nullptr);
		vkDestroyFramebuffer(LogicalDevice(), m_Framebuffer, nullptr);

		// Clean up statics
		InstanceCount--;
		Statics = nullptr;
		if (InstanceCount == 0)
		{
			delete GlobalStatics;
		}
	}
	void RenderpassPostProcess::createRenderPass()
	{
		std::vector<VkAttachmentDescription> attachmentDescriptions;

		VkAttachmentDescription attachmentDescription{};
		attachmentDescription.flags = 0;
		attachmentDescription.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		for (size_t i = 0; i < m_Attachments.size(); i++)
		{
			attachmentDescription.format = m_Attachments[i].m_Attachment->format;

			switch (m_Attachments[i].m_Use)
			{
			case AttachmentUse::ReadOnly:
				attachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
				attachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
				//attachmentDescription.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				break;
			case AttachmentUse::WriteOnly:
				attachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
				//attachmentDescription.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				break;
			case AttachmentUse::ReadWrite:
				attachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
				attachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
				//attachmentDescription.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				break;
			}
			attachmentDescription.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
			attachmentDescription.finalLayout = attachmentDescription.initialLayout;
			attachmentDescriptions.push_back(attachmentDescription);
		};

		// Collect attachment references
		//std::vector<VkAttachmentReference> colorReferences;

		//for (size_t i = 0; i < m_CombinedAttachmentCount; i++)
		//{
		//	colorReferences.push_back({ static_cast<uint32_t>(i), attachmentDescriptions[i].initialLayout });
		//};

		// Default render pass setup uses only one subpass
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments = nullptr;
		subpass.colorAttachmentCount = 0;
		//subpass.pColorAttachments = colorReferences.data();
		//subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());

		// Use subpass dependencies for attachment layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		//dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		//dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		//dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		//dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Create render pass
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pAttachments = attachmentDescriptions.data();
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = dependencies.data();
		VK_CHECK_RESULT(vkCreateRenderPass(LogicalDevice(), &renderPassInfo, nullptr, &m_Renderpass));
	}
	void RenderpassPostProcess::setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		setLayoutBindings.reserve(m_CombinedAttachmentCount);
		for (uint32_t i = 0; i < m_CombinedAttachmentCount; i++)
		{
			setLayoutBindings.push_back(vks::initializers::descriptorSetLayoutBinding(
				//VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
				i
			));
		}

		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(LogicalDevice(), &descriptorLayout, nullptr, &m_DescriptorSetLayout));

		// Shared pipeline layout used by composition
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&m_DescriptorSetLayout, 1);

		//setup push constants
		VkPushConstantRange push_constant;
		//this push constant range starts at the beginning
		push_constant.offset = 0;
		//this push constant range takes up the size of a MeshPushConstants struct
		push_constant.size = sizeof(PushConstantsContainer);
		//this push constant range is accessible only in the vertex shader
		push_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		pPipelineLayoutCreateInfo.pPushConstantRanges = &push_constant;
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;

		VK_CHECK_RESULT(vkCreatePipelineLayout(LogicalDevice(), &pPipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.reserve(1);
		poolSizes.push_back(VkDescriptorPoolSize
			{
				VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				static_cast<uint32_t>(m_CombinedAttachmentCount)
			});

		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(
			//{ vks::initializers::descriptorPoolSize(VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_CombinedAttachmentCount) }, 1);
			poolSizes, 1);
		VK_CHECK_RESULT(vkCreateDescriptorPool(LogicalDevice(), &descriptorPoolInfo, nullptr, &m_DescriptorPool));
	}
	void RenderpassPostProcess::setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(m_DescriptorPool, &m_DescriptorSetLayout, 1);
		vkAllocateDescriptorSets(LogicalDevice(), &allocInfo, &m_AttachmentDescriptorSet);

		std::vector<VkDescriptorImageInfo> imageInfos;
		imageInfos.reserve(m_CombinedAttachmentCount);
		for (size_t i = 0; i < m_CombinedAttachmentCount; i++)
		{
			VkImageLayout layout;
			//switch (m_Attachments[i].m_Use)
			//{
			//case AttachmentUse::ReadOnly:
			//	layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			//	break;
			//case AttachmentUse::WriteOnly:
			//case AttachmentUse::ReadWrite:
			//	layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			//	break;
			//}
			layout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
			imageInfos.push_back(vks::initializers::descriptorImageInfo(
			//	m_Main->m_DefaultColorSampler,
				nullptr,
				m_Attachments[i].m_Attachment->view,
				layout
			));
		}

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		writeDescriptorSets.reserve(m_CombinedAttachmentCount);
		for (size_t i = 0; i < m_CombinedAttachmentCount; i++)
		{
			writeDescriptorSets.push_back(vks::initializers::writeDescriptorSet(
				m_AttachmentDescriptorSet,
				//VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				static_cast<uint32_t>(i),
				&imageInfos[i]
			));
		}

		vkUpdateDescriptorSets(LogicalDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}
	void RenderpassPostProcess::setupPipeline()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
		blendAttachmentStates.reserve(m_CombinedAttachmentCount);
		for (size_t i = 0; i < m_CombinedAttachmentCount; i++)
		{
			blendAttachmentStates.push_back(vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE));
		}
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(m_CombinedAttachmentCount, blendAttachmentStates.data());
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(m_PipelineLayout, m_Renderpass);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		// Final fullscreen composition pass pipeline
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		shaderStages[0] = Statics->m_VertexPassthroughShader;
		shaderStages[1] = m_Main->LoadShader(m_Shadername, VK_SHADER_STAGE_FRAGMENT_BIT);
		// Empty vertex input state, vertices are generated by the vertex shader
		VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		pipelineCI.pVertexInputState = &emptyInputState;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(LogicalDevice(), Statics->m_PipelineCache, 1, &pipelineCI, nullptr, &m_Pipeline));
	}

	void RenderpassPostProcess::setupFramebuffer()
	{
		std::vector<VkImageView> attachmentViews;
		attachmentViews.reserve(m_CombinedAttachmentCount);
		for (size_t i = 0; i < m_CombinedAttachmentCount; i++)
		{
			attachmentViews.push_back(m_Attachments[i].m_Attachment->view);
		}

		VkExtent2D size = m_AttachmentManager->GetSize();

		VkFramebufferCreateInfo fbufCreateInfo = {};
		fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbufCreateInfo.pNext = NULL;
		fbufCreateInfo.renderPass = m_Renderpass;
		fbufCreateInfo.pAttachments = attachmentViews.data();
		fbufCreateInfo.attachmentCount = static_cast<uint32_t>(m_CombinedAttachmentCount);
		fbufCreateInfo.width = size.width;
		fbufCreateInfo.height = size.height;
		fbufCreateInfo.layers = 1;
		VK_CHECK_RESULT(vkCreateFramebuffer(LogicalDevice(), &fbufCreateInfo, nullptr, &m_Framebuffer));
	}

	void RenderpassPostProcess::buildCommandBuffer()
	{
		if (m_CmdBuffer == nullptr)
		{
			m_CmdBuffer = m_Device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY, false);
		}

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = m_Renderpass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = m_Main->width;
		renderPassBeginInfo.renderArea.extent.height = m_Main->height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;


		renderPassBeginInfo.framebuffer = m_Framebuffer;

		VK_CHECK_RESULT(vkBeginCommandBuffer(m_CmdBuffer, &cmdBufInfo));

		vkCmdBeginRenderPass(m_CmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)m_Main->width, (float)m_Main->height, 0.0f, 1.0f);
		vkCmdSetViewport(m_CmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(m_Main->width, m_Main->height, 0, 0);
		vkCmdSetScissor(m_CmdBuffer, 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_AttachmentDescriptorSet, 0, nullptr);

		vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

		m_PushConstants = PushConstantsContainer{ m_Main->width, m_Main->height };
		vkCmdPushConstants(m_CmdBuffer, m_PipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsContainer), &m_PushConstants);

		// Final composition as full screen quad
		// Note: Also used for debug display if debugDisplayTarget > 0
		vkCmdDraw(m_CmdBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(m_CmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(m_CmdBuffer));
	}

	RenderpassPostProcess::StaticsContainer::StaticsContainer(RTFilterDemo* demo)
		: m_Device(demo->vulkanDevice), m_ColorSampler_Normalized(demo->m_DefaultColorSampler)
	{
		m_VertexPassthroughShader = demo->LoadShader("postprocess_passthrough.vert.spv", VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);

		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreatePipelineCache(m_Device->logicalDevice, &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));

		// Create sampler to sample from color attachments
		VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
		sampler.flags;
		sampler.magFilter = VkFilter::VK_FILTER_NEAREST;
		sampler.minFilter = VkFilter::VK_FILTER_NEAREST;
		sampler.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sampler.addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.mipLodBias = 0.0f;
		sampler.anisotropyEnable = VK_FALSE;
		sampler.maxAnisotropy = 1.0f;
		sampler.compareEnable = VK_FALSE;
		sampler.compareOp = VkCompareOp::VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		sampler.maxLod = 0.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		sampler.unnormalizedCoordinates = VK_TRUE;
		VK_CHECK_RESULT(vkCreateSampler(m_Device->logicalDevice, &sampler, nullptr, &m_ColorSampler_Standard));
	}
	RenderpassPostProcess::StaticsContainer::~StaticsContainer()
	{
		vkDestroyPipelineCache(m_Device->logicalDevice, m_PipelineCache, nullptr);
		vkDestroySampler(m_Device->logicalDevice, m_ColorSampler_Standard, nullptr);
	}
}