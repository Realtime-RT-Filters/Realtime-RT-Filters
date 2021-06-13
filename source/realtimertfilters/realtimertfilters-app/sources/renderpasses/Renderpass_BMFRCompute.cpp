#include "../../headers/renderpasses/Renderpass_BMFRCompute.hpp"
#include "../../headers/RTFilterDemo.hpp"

namespace bmfr
{
	void RenderpassBMFRCompute::PushFeatureBuffer(FeatureBuffer& featurebuffer)
	{
		m_FeatureBuffer.push_back(featurebuffer);
	}

	void RenderpassBMFRCompute::prepare()
	{
		m_compute_QueueFamilyIndex = m_vulkanDevice->queueFamilyIndices.compute;
		// Get a compute queue from the device
		vkGetDeviceQueue(getLogicalDevice(), m_compute_QueueFamilyIndex, 0, &m_computeQueue);

		// Create compute pipeline
		// Compute pipelines are created separate from graphics pipelines even if they use the same queue

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0: Input image (read-only)
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0),
			// Binding 1: Output image (write)
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(getLogicalDevice(), &descriptorLayout, nullptr, &m_descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vks::initializers::pipelineLayoutCreateInfo(&m_descriptorSetLayout, 1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(getLogicalDevice(), &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));

		VkDescriptorSetAllocateInfo allocInfo =
			vks::initializers::descriptorSetAllocateInfo(m_descriptorPool, &m_descriptorSetLayout, 1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(getLogicalDevice(), &allocInfo, &m_descriptorSet));
		std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets = 
		{
			
			//			vks::initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, &ATTACHMENTDESCRIPTOR)
		};
		vkUpdateDescriptorSets(getLogicalDevice(), computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL);

		// Create compute shader pipelines
		VkComputePipelineCreateInfo computePipelineCreateInfo =
			vks::initializers::computePipelineCreateInfo(m_pipelineLayout, 0);

		// Make pipeline cache
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreatePipelineCache(getLogicalDevice(), &pipelineCacheCreateInfo, nullptr, &m_pipelineCache));

		computePipelineCreateInfo.stage = m_rtFilterDemo->loadShader("bmfr/bmfrMain.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
		VK_CHECK_RESULT(vkCreateComputePipelines(getLogicalDevice(), m_pipelineCache, 1, &computePipelineCreateInfo, nullptr, &m_pipeline));

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

	void RenderpassBMFRCompute::buildCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VK_CHECK_RESULT(vkBeginCommandBuffer(m_cmdBuffer, &cmdBufInfo));

		vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
		vkCmdBindDescriptorSets(m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, 0);

		vkCmdDispatch(m_cmdBuffer, 1, 1, 1);

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