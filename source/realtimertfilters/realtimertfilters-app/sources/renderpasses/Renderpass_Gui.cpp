#include "../../headers/renderpasses/Renderpass_Gui.hpp"
#include "../../headers/RTFilterDemo.hpp"
/*
namespace rtf
{


	Renderpass_Gui::Renderpass_Gui(VkInstance instance, vks::VulkanDevice* device, Attachment_Manager* attachment_manager, RTFilterDemo* demo, VulkanSwapChain* swapChain, float* timer, int* debugDisplayTarget, Camera* camera)
		: Renderpass(instance, device, attachment_manager, demo)
	{

		m_Scene = &(demo->m_Scene);
		m_swapchain = swapChain;

		//stuff taken to allow legacy bits to work
		m_timer = timer;
		m_debugDisplayTarget = debugDisplayTarget;
		m_camera = camera;
	}

	Renderpass_Gui::~Renderpass_Gui()
	{
	}

	void Renderpass_Gui::buildCommandBuffer()
	{
		if (m_CmdBuffer == nullptr)
		{
			m_CmdBuffer = m_Device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
		}

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };


		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = m_rtFilterDemo->renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = m_rtFilterDemo->width;
		renderPassBeginInfo.renderArea.extent.height = m_rtFilterDemo->height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		for (int32_t i = 0; i < m_rtFilterDemo->drawCmdBuffers.size(); ++i)
		{
			renderPassBeginInfo.framebuffer = m_rtFilterDemo->frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(m_CmdBuffer, &cmdBufInfo));

			vkCmdBeginRenderPass(m_CmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


			VkExtent2D size = m_AttachmentManager->GetSize();

			VkViewport viewport = vks::initializers::viewport((float)size.width, (float)size.height, 0.0f, 1.0f);
			vkCmdSetViewport(m_CmdBuffer, 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(size.width, size.height, 0, 0);
			vkCmdSetScissor(m_CmdBuffer, 0, 1, &scissor);

			vkCmdBindDescriptorSets(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptorSetInputAttachments, 0, nullptr);

			vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
			// Final composition as full screen quad
			// Note: Also used for debug display if debugDisplayTarget > 0
			vkCmdDraw(m_CmdBuffer, 3, 1, 0, 0);

			m_rtFilterDemo->drawUI(m_CmdBuffer);

			vkCmdEndRenderPass(m_CmdBuffer);

			VK_CHECK_RESULT(vkEndCommandBuffer(m_CmdBuffer));
		}


	}

	void Renderpass_Gui::draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount)
	{
		out_commandBufferCount = 1;
		out_commandBuffers = &m_CmdBuffer;
	}

	void Renderpass_Gui::prepare()
	{
		//Get all the needed attachments from the attachment manager
		m_position = m_attachmentManager->getAttachment(Attachment::position);
		m_normal = m_attachmentManager->getAttachment(Attachment::normal);
		m_albedo = m_attachmentManager->getAttachment(Attachment::albedo);
		m_motionvector = m_attachmentManager->getAttachment(Attachment::motionvector);
		m_rtoutput = m_attachmentManager->getAttachment(Attachment::rtoutput);
		m_filteroutput = m_attachmentManager->getAttachment(Attachment::filteroutput);



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


		//UBO
		prepareUBO();


		//Descriptors
		setupDescriptorSetLayout();
		setupDescriptorPool();
		setupDescriptorSet();

		prepareRenderpass();
		PreparePipelines();




		//buildCommandBuffer();
	}

	void Renderpass_Gui::prepareUBO()
	{
		// Deferred fragment shader
		VK_CHECK_RESULT(m_Device->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_Buffer,
			sizeof(m_Buffer)));
		
		// Map persistent
		//VK_CHECK_RESULT(uniformBuffers.offscreen.map());
		VK_CHECK_RESULT(m_Buffer.map());

		updateUBO();
		
	}

	void Renderpass_Gui::updateUBO()
	{
		// White
		m_Comp_UBO.lights[0].position = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		m_Comp_UBO.lights[0].color = glm::vec3(1.5f);
		m_Comp_UBO.lights[0].radius = 15.0f * 0.25f;
		// Red
		m_Comp_UBO.lights[1].position = glm::vec4(-2.0f, 0.0f, 0.0f, 0.0f);
		m_Comp_UBO.lights[1].color = glm::vec3(1.0f, 0.0f, 0.0f);
		m_Comp_UBO.lights[1].radius = 15.0f;
		// Blue
		m_Comp_UBO.lights[2].position = glm::vec4(2.0f, -1.0f, 0.0f, 0.0f);
		m_Comp_UBO.lights[2].color = glm::vec3(0.0f, 0.0f, 2.5f);
		m_Comp_UBO.lights[2].radius = 5.0f;
		// Yellow
		m_Comp_UBO.lights[3].position = glm::vec4(0.0f, -0.9f, 0.5f, 0.0f);
		m_Comp_UBO.lights[3].color = glm::vec3(1.0f, 1.0f, 0.0f);
		m_Comp_UBO.lights[3].radius = 2.0f;
		// Green
		m_Comp_UBO.lights[4].position = glm::vec4(0.0f, -0.5f, 0.0f, 0.0f);
		m_Comp_UBO.lights[4].color = glm::vec3(0.0f, 1.0f, 0.2f);
		m_Comp_UBO.lights[4].radius = 5.0f;
		// Yellow
		m_Comp_UBO.lights[5].position = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
		m_Comp_UBO.lights[5].color = glm::vec3(1.0f, 0.7f, 0.3f);
		m_Comp_UBO.lights[5].radius = 25.0f;

		m_Comp_UBO.lights[0].position.x = sin(glm::radians(360.0f * *m_timer)) * 5.0f;
		m_Comp_UBO.lights[0].position.z = cos(glm::radians(360.0f * *m_timer)) * 5.0f;

		m_Comp_UBO.lights[1].position.x = -4.0f + sin(glm::radians(360.0f * *m_timer) + 45.0f) * 2.0f;
		m_Comp_UBO.lights[1].position.z = 0.0f + cos(glm::radians(360.0f * *m_timer) + 45.0f) * 2.0f;

		m_Comp_UBO.lights[2].position.x = 4.0f + sin(glm::radians(360.0f * *m_timer)) * 2.0f;
		m_Comp_UBO.lights[2].position.z = 0.0f + cos(glm::radians(360.0f * *m_timer)) * 2.0f;

		m_Comp_UBO.lights[4].position.x = 0.0f + sin(glm::radians(360.0f * *m_timer + 90.0f)) * 5.0f;
		m_Comp_UBO.lights[4].position.z = 0.0f - cos(glm::radians(360.0f * *m_timer + 45.0f)) * 5.0f;

		m_Comp_UBO.lights[5].position.x = 0.0f + sin(glm::radians(-360.0f * *m_timer + 135.0f)) * 10.0f;
		m_Comp_UBO.lights[5].position.z = 0.0f - cos(glm::radians(-360.0f * *m_timer - 45.0f)) * 10.0f;

		// Current view position
		m_Comp_UBO.viewPos = glm::vec4(m_camera->position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

		m_Comp_UBO.debugDisplayTarget = *m_debugDisplayTarget;

		memcpy(m_Buffer.mapped, &m_Comp_UBO, sizeof(m_Comp_UBO));
	}

	void Renderpass_Gui::setupDescriptorSetLayout()
	{
		// Deferred shading layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			// Binding 1 : Position texture target / Scene colormap
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			// Binding 2 : Normals texture target
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
			// Binding 3 : Albedo texture target
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
			// Binding 4 : Fragment shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
		};


		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_Device->logicalDevice, &descriptorLayout, nullptr, &m_DescriptorSetLayout));

		// Shared pipeline layout used by composition
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&m_DescriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(m_Device->logicalDevice, &pPipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));
	}




	void Renderpass_Gui::setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 3);
		VK_CHECK_RESULT(vkCreateDescriptorPool(m_Device->logicalDevice, &descriptorPoolInfo, nullptr, &m_DescriptorPool));

	}

	void Renderpass_Gui::setupDescriptorSet()
	{

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(m_DescriptorPool, &m_DescriptorSetLayout, 1);

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
		/*
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
			// * /


		VK_CHECK_RESULT(vkAllocateDescriptorSets(m_Device->logicalDevice, &allocInfo, &descriptorSetInputAttachments));
		writeDescriptorSets = {
			// Binding 1 : Position texture target
			vks::initializers::writeDescriptorSet(descriptorSetInputAttachments, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &texDescriptorPosition),
			// Binding 2 : Normals texture target
			vks::initializers::writeDescriptorSet(descriptorSetInputAttachments, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &texDescriptorNormal),
			// Binding 3 : Albedo texture target
			vks::initializers::writeDescriptorSet(descriptorSetInputAttachments, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &texDescriptorAlbedo),
			// Binding 4 : Fragment shader uniform buffer
			vks::initializers::writeDescriptorSet(descriptorSetInputAttachments, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, &m_Buffer.descriptor),
		};
		vkUpdateDescriptorSets(m_Device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void Renderpass_Gui::prepareRenderpass()
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

		VK_CHECK_RESULT(vkCreateRenderPass(m_Device->logicalDevice, &renderPassInfo, nullptr, &m_Renderpass));

	}

	void Renderpass_Gui::PreparePipelines()
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

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(m_PipelineLayout, m_Renderpass);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		//pipelineCI.pDepthStencilState = &depthStencilState;
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
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_Device->logicalDevice, nullptr, 1, &pipelineCI, nullptr, &m_Pipeline));

	}

}
*/
