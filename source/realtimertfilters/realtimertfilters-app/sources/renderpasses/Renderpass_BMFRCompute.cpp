#include "../../headers/renderpasses/Renderpass_BMFRCompute.hpp"
#include "../../headers/RTFilterDemo.hpp"

namespace bmfr
{
	//void RenderpassBMFRCompute::PushFeatureBuffer(FeatureBuffer& featurebuffer)
	//{
	//	m_FeatureBuffer.push_back(featurebuffer);
	//}

	using namespace rtf;

	void RenderpassBMFRCompute::prepare()
	{
		m_Blocks = {(m_rtFilterDemo->width + BLOCK_SIZE_X - 1) / BLOCK_SIZE_X, (m_rtFilterDemo->height + BLOCK_SIZE_Y - 1) / BLOCK_SIZE_Y};

		m_RTInput = m_attachmentManager->getAttachment(Attachment::intermediate);
		m_Positions = m_attachmentManager->getAttachment(Attachment::position);
		m_Normals = m_attachmentManager->getAttachment(Attachment::normal);
		m_Output = m_attachmentManager->getAttachment(Attachment::compute_output);

		m_compute_QueueFamilyIndex = m_vulkanDevice->queueFamilyIndices.graphics;
		// Get a compute queue from the device
		vkGetDeviceQueue(getLogicalDevice(), m_compute_QueueFamilyIndex, 0, &m_computeQueue);

		CreateDescriptorSetLayoutAndPipeline();

		AllocateAndWriteDescriptorSet();

		// Separate command pool as queue family for compute may be different than graphics
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = m_compute_QueueFamilyIndex;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(getLogicalDevice(), &cmdPoolInfo, nullptr, &m_commandPool));

		// Create a command buffer for compute operations
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vks::initializers::commandBufferAllocateInfo(
				m_commandPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);

		VK_CHECK_RESULT(vkAllocateCommandBuffers(getLogicalDevice(), &cmdBufAllocateInfo, &m_cmdBuffer));

		// Build a single command buffer containing the compute dispatch commands
		buildCommandBuffer();
	}

	void RenderpassBMFRCompute::AllocateAndWriteDescriptorSet()
	{
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vks::initializers::descriptorPoolSize(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			vks::initializers::descriptorPoolSize(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 5),
		};
		VkDescriptorPoolCreateInfo poolCI = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
		VK_CHECK_RESULT(vkCreateDescriptorPool(getLogicalDevice(), &poolCI, nullptr, &m_descriptorPool));

		VkDescriptorSetAllocateInfo allocInfo =
			vks::initializers::descriptorSetAllocateInfo(m_descriptorPool, &m_descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(getLogicalDevice(), &allocInfo, &m_descriptorSet));

		VkDescriptorImageInfo rtin_imageInfo = vks::initializers::descriptorImageInfo(m_rtFilterDemo->m_DefaultColorSampler, m_RTInput->view, VkImageLayout::VK_IMAGE_LAYOUT_GENERAL);
		VkDescriptorImageInfo pos_imageInfo = vks::initializers::descriptorImageInfo(m_rtFilterDemo->m_DefaultColorSampler, m_Positions->view, VkImageLayout::VK_IMAGE_LAYOUT_GENERAL);
		VkDescriptorImageInfo normals_imageInfo = vks::initializers::descriptorImageInfo(m_rtFilterDemo->m_DefaultColorSampler, m_Normals->view, VkImageLayout::VK_IMAGE_LAYOUT_GENERAL);
		VkDescriptorImageInfo output_imageInfo = vks::initializers::descriptorImageInfo(m_rtFilterDemo->m_DefaultColorSampler, m_Output->view, VkImageLayout::VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets =
		{
			// Binding 0: BMFR Config UBO
			m_rtFilterDemo->m_UBO_BMFRConfig->writeDescriptorSet(m_descriptorSet, 0),
			// Binding 1: Positions
			vks::initializers::writeDescriptorSet(m_descriptorSet, VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &pos_imageInfo),
			// Binding 2: Normals
			vks::initializers::writeDescriptorSet(m_descriptorSet, VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2, &normals_imageInfo),
			// Binding 3: Accumulated RT Image
			vks::initializers::writeDescriptorSet(m_descriptorSet, VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3, &rtin_imageInfo),
			// Binding 4: Output
			vks::initializers::writeDescriptorSet(m_descriptorSet, VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4, &output_imageInfo),
		};
		vkUpdateDescriptorSets(getLogicalDevice(), computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL);
	}

	void RenderpassBMFRCompute::CreateDescriptorSetLayoutAndPipeline()
	{
		// Create compute pipeline
		// Compute pipelines are created separate from graphics pipelines even if they use the same queue

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0: BMFR Config UBO
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
			// Binding 1: Positions
			vks::initializers::descriptorSetLayoutBinding(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT, 1),
			// Binding 2: Normals
			vks::initializers::descriptorSetLayoutBinding(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT, 2),
			// Binding 3: Accumulated RT Image
			vks::initializers::descriptorSetLayoutBinding(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT, 3),
			// Binding 4: Output
			vks::initializers::descriptorSetLayoutBinding(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT, 4),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(getLogicalDevice(), &descriptorLayout, nullptr, &m_descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vks::initializers::pipelineLayoutCreateInfo(&m_descriptorSetLayout, 1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(getLogicalDevice(), &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));

		// Create compute shader pipelines
		VkComputePipelineCreateInfo computePipelineCreateInfo =
			vks::initializers::computePipelineCreateInfo(m_pipelineLayout, 0);

		// Make pipeline cache
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreatePipelineCache(getLogicalDevice(), &pipelineCacheCreateInfo, nullptr, &m_pipelineCache));

		computePipelineCreateInfo.stage = m_rtFilterDemo->LoadShader("bmfr/bmfrMain.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
		VK_CHECK_RESULT(vkCreateComputePipelines(getLogicalDevice(), m_pipelineCache, 1, &computePipelineCreateInfo, nullptr, &m_pipeline));
	}

	void RenderpassBMFRCompute::buildCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VK_CHECK_RESULT(vkBeginCommandBuffer(m_cmdBuffer, &cmdBufInfo));

		vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
		vkCmdBindDescriptorSets(m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, 0);

		vkCmdDispatch(m_cmdBuffer, m_Blocks.width, m_Blocks.height, 1);

		vkEndCommandBuffer(m_cmdBuffer);
	}

	void RenderpassBMFRCompute::draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount)
	{
		out_commandBuffers = &m_cmdBuffer;
		out_commandBufferCount = 1;
	}

	void RenderpassBMFRCompute::cleanUp()
	{
		vkDestroyPipeline(getLogicalDevice(), m_pipeline, nullptr);
		vkDestroyPipelineLayout(getLogicalDevice(), m_pipelineLayout, nullptr);
		vkDestroyPipelineCache(getLogicalDevice(), m_pipelineCache, nullptr);
		vkDestroyDescriptorSetLayout(getLogicalDevice(), m_descriptorSetLayout, nullptr);
		vkDestroyCommandPool(getLogicalDevice(), m_commandPool, nullptr);
	}
}