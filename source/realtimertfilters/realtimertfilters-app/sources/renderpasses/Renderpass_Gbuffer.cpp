#include "../../headers/renderpasses/Renderpass_Gbuffer.hpp"
#include "../../headers/RTFilterDemo.hpp"
#include <VulkanTools.cpp>

namespace rtf
{

	RenderpassGbuffer::RenderpassGbuffer()
	{
	}

	// Heavily inspired from Sascha Willems' "deferred" vulkan example
	void RenderpassGbuffer::prepare()
	{
		m_Scene = &(m_rtFilterDemo->m_Scene);
		prepareAttachments();
		prepareRenderpass();
		setupDescriptorSetLayout();
		preparePipeline();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffer();
	}

	void RenderpassGbuffer::prepareAttachments()
	{
		m_PositionAttachment = m_attachmentManager->getAttachment(Attachment::position);
		m_NormalAttachment = m_attachmentManager->getAttachment(Attachment::normal);
		m_AlbedoAttachment = m_attachmentManager->getAttachment(Attachment::albedo);
		m_MotionAttachment = m_attachmentManager->getAttachment(Attachment::motionvector);
		m_MeshIdAttachment = m_attachmentManager->getAttachment(Attachment::meshid);
		m_DepthAttachment = m_attachmentManager->getAttachment(Attachment::depth);
		assert(m_PositionAttachment != nullptr);
		assert(m_NormalAttachment != nullptr);
		assert(m_AlbedoAttachment != nullptr);
		assert(m_MotionAttachment != nullptr);
		assert(m_MeshIdAttachment != nullptr);
		assert(m_DepthAttachment != nullptr);
	}

	void RenderpassGbuffer::prepareRenderpass()
	{
		// Formatting attachment data into VkAttachmentDescription structs
		const uint32_t ATTACHMENT_COUNT_COLOR = 5;
		const uint32_t ATTACHMENT_COUNT_DEPTH = 1;
		const uint32_t ATTACHMENT_COUNT = ATTACHMENT_COUNT_COLOR + ATTACHMENT_COUNT_DEPTH;
		VkAttachmentDescription attachmentDescriptions[ATTACHMENT_COUNT] = {};
		FrameBufferAttachment* attachments[] =
		{
			m_PositionAttachment, m_NormalAttachment, m_AlbedoAttachment, m_MotionAttachment, m_MeshIdAttachment, m_DepthAttachment
		};

		for (uint32_t i = 0; i < ATTACHMENT_COUNT; i++)
		{
			attachmentDescriptions[i].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
			attachmentDescriptions[i].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentDescriptions[i].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDescriptions[i].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescriptions[i].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescriptions[i].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescriptions[i].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
			attachmentDescriptions[i].format = attachments[i]->format;
		}
		attachmentDescriptions[ATTACHMENT_COUNT_COLOR].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // The depth attachment needs a different layout
		attachmentDescriptions[ATTACHMENT_COUNT_COLOR].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

																												 // Preparing attachment reference structs
		VkAttachmentReference attachmentReferences_Color[ATTACHMENT_COUNT_COLOR] = {};
		for (uint32_t i = 0; i < ATTACHMENT_COUNT_COLOR; i++)
		{
			attachmentReferences_Color[i] = VkAttachmentReference{ i, VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }; // Assign incremental ids
		}

		VkAttachmentReference attachmentReference_Depth =
		{ ATTACHMENT_COUNT_COLOR, VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }; // the depth attachment gets the final id (one higher than the highest color attachment id)

																									 // Subpass description
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = ATTACHMENT_COUNT_COLOR;
		subpass.pColorAttachments = attachmentReferences_Color;
		subpass.pDepthStencilAttachment = &attachmentReference_Depth;

		VkSubpassDependency subPassDependencies[2] = {};
		subPassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subPassDependencies[0].dstSubpass = 0;
		subPassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subPassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subPassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subPassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subPassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subPassDependencies[1].srcSubpass = 0;
		subPassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subPassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subPassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subPassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subPassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subPassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pAttachments = attachmentDescriptions;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(ATTACHMENT_COUNT);
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = subPassDependencies;

		VK_CHECK_RESULT(vkCreateRenderPass(m_vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &m_renderpass));

		VkImageView attachmentViews[ATTACHMENT_COUNT] = {
			m_PositionAttachment->view, m_NormalAttachment->view, m_AlbedoAttachment->view, m_MotionAttachment->view, m_MeshIdAttachment->view, m_DepthAttachment->view
		};

		VkExtent2D size = m_attachmentManager->GetSize();

		VkFramebufferCreateInfo fbufCreateInfo = {};
		fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbufCreateInfo.pNext = NULL;
		fbufCreateInfo.renderPass = m_renderpass;
		fbufCreateInfo.pAttachments = attachmentViews;
		fbufCreateInfo.attachmentCount = static_cast<uint32_t>(ATTACHMENT_COUNT);
		fbufCreateInfo.width = size.width;
		fbufCreateInfo.height = size.height;
		fbufCreateInfo.layers = 1;
		VK_CHECK_RESULT(vkCreateFramebuffer(m_vulkanDevice->logicalDevice, &fbufCreateInfo, nullptr, &m_FrameBuffer));
	}

	void RenderpassGbuffer::draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount)
	{
		out_commandBufferCount = 1;
		out_commandBuffers = &m_CmdBuffer;
	}

	void RenderpassGbuffer::cleanUp()
	{
		vkDestroyDescriptorPool(m_vulkanDevice->logicalDevice, m_descriptorPool, nullptr);
		vkFreeCommandBuffers(m_vulkanDevice->logicalDevice, m_vulkanDevice->commandPool, 1, &m_CmdBuffer);
		vkDestroyFramebuffer(m_vulkanDevice->logicalDevice, m_FrameBuffer, nullptr);
		vkDestroyPipeline(m_vulkanDevice->logicalDevice, m_pipeline, nullptr);
		vkDestroyPipelineLayout(m_vulkanDevice->logicalDevice, m_pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_vulkanDevice->logicalDevice, m_descriptorSetLayout, nullptr);
		vkDestroyRenderPass(m_vulkanDevice->logicalDevice, m_renderpass, nullptr);
		vkDestroyPipelineCache(m_vulkanDevice->logicalDevice, m_PipelineCache, nullptr);
	}


	void RenderpassGbuffer::setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8),
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 9)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 3);
		VK_CHECK_RESULT(vkCreateDescriptorPool(m_vulkanDevice->logicalDevice, &descriptorPoolInfo, nullptr, &m_descriptorPool));
	}

	void RenderpassGbuffer::setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayout> gltfDescriptorSetLayouts = { vkglTF::descriptorSetLayoutUbo, vkglTF::descriptorSetLayoutImage };
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfoOffscreen = vks::initializers::pipelineLayoutCreateInfo(gltfDescriptorSetLayouts.data(), 2);
		VK_CHECK_RESULT(vkCreatePipelineLayout(m_vulkanDevice->logicalDevice, &pPipelineLayoutCreateInfoOffscreen, nullptr, &m_pipelineLayout));
	}

	void RenderpassGbuffer::setupDescriptorSet()
	{
		std::vector<VkWriteDescriptorSet> writeDescriptorSets;

		// Model
		// use descriptor set layout delivered by gltf
		VkDescriptorSetAllocateInfo allocInfoOffscreen = vks::initializers::descriptorSetAllocateInfo(m_descriptorPool, &vkglTF::descriptorSetLayoutUbo, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(m_vulkanDevice->logicalDevice, &allocInfoOffscreen, &m_DescriptorSetScene));
		writeDescriptorSets = {
			// Binding 0: Vertex shader uniform buffer
			m_rtFilterDemo->m_UBO_SceneInfo->writeDescriptorSet(m_DescriptorSetScene, 0)
		};
		vkUpdateDescriptorSets(m_vulkanDevice->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void RenderpassGbuffer::buildCommandBuffer()
	{
		if (m_CmdBuffer == nullptr)
		{
			m_CmdBuffer = m_vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
		}

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		// Clear values for all attachments written in the fragment shader
		std::array<VkClearValue, 6> clearValues;
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[4].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[5].depthStencil = { 1.0f, 0 };

		VkExtent2D size = m_attachmentManager->GetSize();

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = m_renderpass;
		renderPassBeginInfo.framebuffer = m_FrameBuffer;
		renderPassBeginInfo.renderArea.extent = size;
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		VK_CHECK_RESULT(vkBeginCommandBuffer(m_CmdBuffer, &cmdBufInfo));

		vkCmdBeginRenderPass(m_CmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)size.width, (float)size.height, 0.0f, 1.0f);
		vkCmdSetViewport(m_CmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(size.width, size.height, 0, 0);
		vkCmdSetScissor(m_CmdBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		// Instanced object
		vkCmdBindDescriptorSets(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_DescriptorSetScene, 0, nullptr);
		m_Scene->draw(m_CmdBuffer, vkglTF::RenderFlags::BindImages, m_pipelineLayout, 1); // vkglTF::RenderFlags::BindImages

		vkCmdEndRenderPass(m_CmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(m_CmdBuffer));
	}

	void RenderpassGbuffer::preparePipeline()
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreatePipelineCache(m_vulkanDevice->logicalDevice, &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		VkPipelineShaderStageCreateInfo shaderStages[2]
		{
			m_rtFilterDemo->LoadShader("prepass/rasterprepass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			m_rtFilterDemo->LoadShader("prepass/rasterprepass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(m_pipelineLayout, m_renderpass);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = 2;
		pipelineCI.pStages = shaderStages;

		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState(
			{
				vkglTF::VertexComponent::Position,
				vkglTF::VertexComponent::UV,
				vkglTF::VertexComponent::Color,
				vkglTF::VertexComponent::Normal,
				vkglTF::VertexComponent::Tangent,
				vkglTF::VertexComponent::MeshId
			}
		);
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

		// Blend attachment states required for all color attachments
		// This is important, as color write mask will otherwise be 0x0 and you
		// won't see anything rendered to the attachment
		std::array<VkPipelineColorBlendAttachmentState, 5> blendAttachmentStates = {
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
		};

		colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		colorBlendState.pAttachments = blendAttachmentStates.data();

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_vulkanDevice->logicalDevice, m_PipelineCache, 1, &pipelineCI, nullptr, &m_pipeline));
	}
}
