#include "../../headers/renderpasses/Renderpass_PostProcess.hpp"
#include "../../headers/VulkanglTFModel.h"
#include "../../headers/RTFilterDemo.hpp"

namespace rtf
{
	RenderpassPostProcess::StaticsContainer* GlobalStatics;
	uint32_t InstanceCount;

	RenderpassPostProcess::RenderpassPostProcess()
	{
	}

#pragma region Configuration

	void RenderpassPostProcess::ConfigureShader(const std::string& shadername)
	{
		m_Shadername = shadername;
	}

	void RenderpassPostProcess::PushAttachment(const AttachmentBinding& attachmentbinding)
	{
		m_AttachmentBindings.push_back(attachmentbinding);
	}

	void RenderpassPostProcess::Push_PastRenderpass_BufferCopy(Attachment sourceAttachment, Attachment destinationAttachment)
	{
		m_AttachmentCopies.push_back({ sourceAttachment , destinationAttachment });
	}

	void RenderpassPostProcess::PushUBO(UBOPtr& ubo)
	{
		m_UBOs.push_back(ubo);
	}

#pragma endregion
#pragma region prepare

	void RenderpassPostProcess::prepare()
	{

		// Assure the object has been properly configured
		assert(!m_Shadername.empty());

		assert(preprocessAttachmentBindings());

		// Fetch our statics
		if (GlobalStatics == nullptr)
		{
			GlobalStatics = new StaticsContainer(m_rtFilterDemo);
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

	bool RenderpassPostProcess::preprocessAttachmentBindings()
	{
		bool hasOutput = false;

		for (auto& attachmentBinding : m_AttachmentBindings)
		{
			if (attachmentBinding.m_Attachment == nullptr)
			{
				attachmentBinding.resolveAttachment(m_vulkanDevice, m_attachmentManager);
			}
			if (attachmentBinding.writeAccess())
			{
				hasOutput = true;
			}
		}

		return hasOutput;
	}

	void RenderpassPostProcess::createRenderPass()
	{
		std::vector<VkAttachmentDescription> attachmentDescriptions;
		std::vector<VkAttachmentReference> inputReferences;
		std::vector<VkAttachmentReference> outputReferences;

		AttachmentBinding::FillAttachmentDescriptionStructures(attachmentDescriptions, inputReferences, outputReferences, m_AttachmentBindings.data(), getAttachmentCount());

		// Default render pass setup uses only one subpass
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments = outputReferences.data();
		subpass.colorAttachmentCount = outputReferences.size();
		subpass.pInputAttachments = inputReferences.data();
		subpass.inputAttachmentCount = inputReferences.size();

		// Use subpass dependencies for attachment layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Create render pass
		VkRenderPassCreateInfo renderPassInfo = vks::initializers::renderPassCreateInfo();
		renderPassInfo.pAttachments = attachmentDescriptions.data();
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = dependencies.data();
		VK_CHECK_RESULT(vkCreateRenderPass(m_vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &m_renderpass));
	}

	void RenderpassPostProcess::setupDescriptorSetLayout()
	{
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo;

		AttachmentBinding::CreateDescriptorSetLayout(getLogicalDevice(), m_descriptorSetLayouts[DESCRIPTORSET_IMAGES], m_AttachmentBindings.data(), getAttachmentCount());
		if (getUboCount() > 0)
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings{};
			bindings.reserve(getUboCount());
			for (auto& ubo : m_UBOs)
			{
				bindings.push_back(vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, bindings.size()));
			}
			auto ci = vks::initializers::descriptorSetLayoutCreateInfo(bindings);
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(getLogicalDevice(), &ci, nullptr, &m_descriptorSetLayouts[DESCRIPTORSET_UBOS]));
			pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(m_descriptorSetLayouts, 2);
		}
		else
		{
			pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(m_descriptorSetLayouts, 1);
		}

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

		VK_CHECK_RESULT(vkCreatePipelineLayout(m_vulkanDevice->logicalDevice, &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));

		std::vector<VkDescriptorPoolSize> poolsizes{};
		AttachmentBinding::FillPoolSizesVector(poolsizes, m_AttachmentBindings.data(), getAttachmentCount());
		
		uint32_t maxSets = 1;
		if (getUboCount() > 0)
		{
			poolsizes.push_back(VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(getUboCount())});
			maxSets = 2;
		}

		VkDescriptorPoolCreateInfo poolCI = vks::initializers::descriptorPoolCreateInfo(poolsizes, maxSets);

		VK_CHECK_RESULT(vkCreateDescriptorPool(getLogicalDevice(), &poolCI, nullptr, &m_descriptorPool));
	}

	void RenderpassPostProcess::setupDescriptorSet()
	{
		uint32_t descriptorSetCount = (getUboCount() > 0) ? 2U : 1U;
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(m_descriptorPool, m_descriptorSetLayouts, descriptorSetCount);
		vkAllocateDescriptorSets(m_vulkanDevice->logicalDevice, &allocInfo, m_descriptorSets);

		std::vector<VkDescriptorImageInfo> imageInfos{};
		std::vector<VkWriteDescriptorSet> writes{};
		AttachmentBinding::FillWriteDescriptorSetStructures(imageInfos, writes, m_AttachmentBindings.data(), getAttachmentCount(),
			m_descriptorSets[DESCRIPTORSET_IMAGES], Statics->m_ColorSampler_Direct, Statics->m_ColorSampler_Normalized);

		uint32_t binding = 0;
		for (auto& ubo : m_UBOs)
		{
			writes.push_back(ubo->writeDescriptorSet(m_descriptorSets[DESCRIPTORSET_UBOS], binding));
			binding++;
		}

		vkUpdateDescriptorSets(getLogicalDevice(), writes.size(), writes.data(), 0, nullptr);
	}

	void RenderpassPostProcess::setupPipeline()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
		for (size_t i = 0; i < getAttachmentCount(); i++)
		{
			const AttachmentBinding& attachment = m_AttachmentBindings[i];
			if (attachment.m_Bind == AttachmentBinding::BindType::Sampled && attachment.writeAccess())
			{
				blendAttachmentStates.push_back(vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE));
			}
		}
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(blendAttachmentStates.size(), blendAttachmentStates.data());
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(m_pipelineLayout, m_renderpass);
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
		shaderStages[1] = m_rtFilterDemo->LoadShader(m_Shadername, VK_SHADER_STAGE_FRAGMENT_BIT);
		// Empty vertex input state, vertices are generated by the vertex shader
		VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		pipelineCI.pVertexInputState = &emptyInputState;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_vulkanDevice->logicalDevice, Statics->m_PipelineCache, 1, &pipelineCI, nullptr, &m_pipeline));
	}

	void RenderpassPostProcess::setupFramebuffer()
	{
		std::vector<VkImageView> attachmentViews;
		attachmentViews.reserve(getAttachmentCount());
		for (auto& attachment : m_AttachmentBindings)
		{
			if (attachment.usesAttachmentDescription())
			{
				attachmentViews.push_back(attachment.m_ImageView);
			}
		}

		VkExtent2D size = m_attachmentManager->GetSize();

		VkFramebufferCreateInfo fbufCreateInfo = {};
		fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbufCreateInfo.pNext = NULL;
		fbufCreateInfo.renderPass = m_renderpass;
		fbufCreateInfo.pAttachments = attachmentViews.data();
		fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
		fbufCreateInfo.width = size.width;
		fbufCreateInfo.height = size.height;
		fbufCreateInfo.layers = 1;
		VK_CHECK_RESULT(vkCreateFramebuffer(m_vulkanDevice->logicalDevice, &fbufCreateInfo, nullptr, &m_Framebuffer));
	}

	void RenderpassPostProcess::buildCommandBuffer()
	{
		if (m_CmdBuffer == nullptr)
		{
			m_CmdBuffer = m_vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
		}

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(m_CmdBuffer, &cmdBufInfo));

		for (const AttachmentBinding& attachment : m_AttachmentBindings)
		{
			if (attachment.requireImageTransition())
			{
				attachment.makeImageTransition(m_CmdBuffer);
			}
		}

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = m_renderpass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = m_rtFilterDemo->width;
		renderPassBeginInfo.renderArea.extent.height = m_rtFilterDemo->height;
		renderPassBeginInfo.clearValueCount = 0;
		renderPassBeginInfo.pClearValues = clearValues;


		renderPassBeginInfo.framebuffer = m_Framebuffer;

		vkCmdBeginRenderPass(m_CmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)m_rtFilterDemo->width, (float)m_rtFilterDemo->height, 0.0f, 1.0f);
		vkCmdSetViewport(m_CmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(m_rtFilterDemo->width, m_rtFilterDemo->height, 0, 0);
		vkCmdSetScissor(m_CmdBuffer, 0, 1, &scissor);

		uint32_t descriptorSetCount = (getUboCount() > 0) ? 2U : 1U;
		vkCmdBindDescriptorSets(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, descriptorSetCount, m_descriptorSets, 0, nullptr);

		vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		m_PushConstants = PushConstantsContainer{ m_rtFilterDemo->width, m_rtFilterDemo->height };
		vkCmdPushConstants(m_CmdBuffer, m_pipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsContainer), &m_PushConstants);

		// Final composition as full screen quad
		// Note: Also used for debug display if debugDisplayTarget > 0
		vkCmdDraw(m_CmdBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(m_CmdBuffer);

		for (auto copyBufferPair : m_AttachmentCopies)
		{
			Attachment source = copyBufferPair.first;
			VkImage sourceImage = m_rtFilterDemo->m_attachmentManager->getAttachment(source)->image;

			Attachment destination = copyBufferPair.second;
			VkImage destinationImage = m_rtFilterDemo->m_attachmentManager->getAttachment(destination)->image;

			VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			// Prepare layout of source attachment to function as transfer source
			vks::tools::setImageLayout(
				m_CmdBuffer,
				sourceImage,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			);



			// Prepare layout of destination attachment to function as transfer destination
			vks::tools::setImageLayout(
				m_CmdBuffer,
				destinationImage,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			);

			VkImageCopy copyRegion{};
			copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.srcOffset = { 0, 0, 0 };
			copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.dstOffset = { 0, 0, 0 };
			copyRegion.extent = { m_rtFilterDemo->width, m_rtFilterDemo->height, 1 };
			vkCmdCopyImage(m_CmdBuffer, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destinationImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			// Transition destination image back to layout general
			vks::tools::setImageLayout(
				m_CmdBuffer,
				destinationImage,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL
				);

			// Transition source image back to layout general
			vks::tools::setImageLayout(
				m_CmdBuffer,
				sourceImage,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL
				);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(m_CmdBuffer));
	}

#pragma endregion
#pragma region draw

	void RenderpassPostProcess::draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount)
	{
		out_commandBufferCount = 1;
		out_commandBuffers = &m_CmdBuffer;
	}

#pragma endregion
#pragma region cleanup

	void RenderpassPostProcess::cleanUp()
	{
		vkDestroyRenderPass(m_vulkanDevice->logicalDevice, m_renderpass, nullptr);
		vkDestroyPipeline(m_vulkanDevice->logicalDevice, m_pipeline, nullptr);
		vkDestroyPipelineLayout(m_vulkanDevice->logicalDevice, m_pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_vulkanDevice->logicalDevice, m_descriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(m_vulkanDevice->logicalDevice, m_descriptorPool, nullptr);
		vkDestroyFramebuffer(m_vulkanDevice->logicalDevice, m_Framebuffer, nullptr);

		// Clean up statics
		InstanceCount--;
		Statics = nullptr;
		if (InstanceCount == 0)
		{
			delete GlobalStatics;
		}
	}

#pragma endregion
#pragma region StaticsContainer

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
		VK_CHECK_RESULT(vkCreateSampler(m_Device->logicalDevice, &sampler, nullptr, &m_ColorSampler_Direct));
	}
	RenderpassPostProcess::StaticsContainer::~StaticsContainer()
	{
		vkDestroyPipelineCache(m_Device->logicalDevice, m_PipelineCache, nullptr);
		vkDestroySampler(m_Device->logicalDevice, m_ColorSampler_Direct, nullptr);
	}

#pragma endregion
}