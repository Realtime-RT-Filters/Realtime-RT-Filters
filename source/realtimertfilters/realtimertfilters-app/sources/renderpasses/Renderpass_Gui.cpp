#include "../../headers/renderpasses/Renderpass_Gui.hpp"
#include "../../headers/RTFilterDemo.hpp"

namespace rtf
{
	RenderpassGui::RenderpassGui()
	{	}

	RenderpassGui::~RenderpassGui()
	{
		vkDestroyPipeline(m_vulkanDevice->logicalDevice, m_pipeline, nullptr);
		vkDestroyPipelineLayout(m_vulkanDevice->logicalDevice, m_pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_vulkanDevice->logicalDevice, m_descriptorSetLayout, nullptr);
	}

	void RenderpassGui::prepare()
	{
		m_Scene = &(m_rtFilterDemo->m_Scene);
		m_swapchain = &m_rtFilterDemo->swapChain;

		//stuff taken to allow legacy bits to work

		m_commandBuffers = &m_rtFilterDemo->drawCmdBuffers;
		m_currentBuffer = &m_rtFilterDemo->currentBuffer;

		//Get all the needed attachments from the attachment manager
		for (auto& attachment : m_attachments)
		{
			attachment.m_Attachment = m_attachmentManager->getAttachment(attachment.m_AttachmentId);
		}

		//Descriptors
		setupDescriptorSetLayout();
		setupDescriptorPool();
		setupDescriptorSet();

		//prepareRenderpass();
		preparePipelines();
		buildCommandBuffer();
	}

	void RenderpassGui::buildCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };


		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = m_rtFilterDemo->renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = m_rtFilterDemo->width;
		renderPassBeginInfo.renderArea.extent.height = m_rtFilterDemo->height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		for (int32_t i = 0; i < m_commandBuffers->size(); ++i)
		{
			renderPassBeginInfo.framebuffer = m_rtFilterDemo->frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffers->at(i), &cmdBufInfo));

			vkCmdBeginRenderPass(m_commandBuffers->at(i), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


			VkExtent2D size = m_attachmentManager->GetSize();

			VkViewport viewport = vks::initializers::viewport((float)size.width, (float)size.height, 0.0f, 1.0f);
			vkCmdSetViewport(m_commandBuffers->at(i), 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(size.width, size.height, 0, 0);
			vkCmdSetScissor(m_commandBuffers->at(i), 0, 1, &scissor);

			vkCmdBindDescriptorSets(m_commandBuffers->at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

			vkCmdBindPipeline(m_commandBuffers->at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
			// Final composition as full screen quad
			vkCmdDraw(m_commandBuffers->at(i), 3, 1, 0, 0);

			m_rtFilterDemo->drawUI(m_commandBuffers->at(i));

			vkCmdEndRenderPass(m_commandBuffers->at(i));

			VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffers->at(i)));
		}


	}

	void RenderpassGui::draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount)
	{
		out_commandBufferCount = 1;
		out_commandBuffers = &m_commandBuffers->at(*m_currentBuffer);
	}

	void RenderpassGui::setupDescriptorSetLayout()
	{
		// Deferred shading layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			// Binding 1 : Attachments array
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, m_attachments.size()),
			// Binding 2 : Guibase Fragment shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
			// Binding 3 : SceneInfo uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_vulkanDevice->logicalDevice, &descriptorLayout, nullptr, &m_descriptorSetLayout));


		// Shared pipeline layout used by composition
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&m_descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(m_vulkanDevice->logicalDevice, &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));
	}

	void RenderpassGui::setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_attachments.size())
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 3);
		VK_CHECK_RESULT(vkCreateDescriptorPool(m_vulkanDevice->logicalDevice, &descriptorPoolInfo, nullptr, &m_descriptorPool));
	}

	void RenderpassGui::setupDescriptorSet()
	{
		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(m_descriptorPool, &m_descriptorSetLayout, 1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(m_vulkanDevice->logicalDevice, &allocInfo, &m_descriptorSet));

		std::vector<VkDescriptorImageInfo> imageInfos{};
		imageInfos.reserve(m_attachments.size());
		for (size_t i = 0; i < m_attachments.size(); i++)
		{
			imageInfos.push_back(vks::initializers::descriptorImageInfo(m_rtFilterDemo->m_DefaultColorSampler, m_attachments[i].m_Attachment->view, VK_IMAGE_LAYOUT_GENERAL));
		}

		writeDescriptorSets = {
			// Binding 1 : Attachment array
			vks::initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, imageInfos.data(), m_attachments.size()),
			// Binding 2 : Fragment shader uniform buffer
			m_rtFilterDemo->m_UBO_Guibase->writeDescriptorSet(m_descriptorSet, 2),
			// Binding 3 : SceneInfo uniform buffer
			m_rtFilterDemo->m_UBO_SceneInfo->writeDescriptorSet(m_descriptorSet, 3)
		};

		vkUpdateDescriptorSets(m_vulkanDevice->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void RenderpassGui::prepareRenderpass()
	{
		std::array<VkAttachmentDescription, 1> attachments = {};
		// Color attachment
		attachments[0].format = m_swapchain->colorFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = nullptr;
		subpassDescription.inputAttachmentCount = 0;
		subpassDescription.pInputAttachments = nullptr;
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = nullptr;
		subpassDescription.pResolveAttachments = nullptr;

		// Subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 1> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(m_vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &m_rtFilterDemo->renderPass));

	}

	void RenderpassGui::preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(m_pipelineLayout, m_rtFilterDemo->renderPass);
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
		shaderStages[0] = m_rtFilterDemo->LoadShader("gui/gui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_rtFilterDemo->LoadShader("gui/gui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		// Empty vertex input state, vertices are generated by the vertex shader
		VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		pipelineCI.pVertexInputState = &emptyInputState;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_vulkanDevice->logicalDevice, nullptr, 1, &pipelineCI, nullptr, &m_pipeline));

	}

	void RenderpassGui::setAttachmentBindings(std::vector<GuiAttachmentBinding> attachmentBindings)
	{
		m_attachments = attachmentBindings;
		m_dropoutOptions.clear();
		m_dropoutOptions.reserve(m_attachments.size());
		for (auto& attachment : m_attachments)
		{
			m_dropoutOptions.push_back(attachment.m_Displayname);
		}
	}

}

