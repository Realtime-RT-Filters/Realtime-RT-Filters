#include "../../headers/renderpasses/RenderpassManager.hpp"
#include "../../headers/renderpasses/Renderpass_Gbuffer.hpp"
#include "../../headers/renderpasses/Renderpass.hpp"
#include "../../headers/RTFilterDemo.hpp"

namespace rtf
{
	RenderpassManager::~RenderpassManager()
	{

	}

	void RenderpassManager::addRenderpass(std::shared_ptr<Renderpass> renderpass)
	{
		m_renderpasses.push_back(renderpass);
	}

	void RenderpassManager::prepare(RTFilterDemo* rtFilterDemo)
	{
		// copy vulkan handles
		m_device = rtFilterDemo->device;
		m_queue = rtFilterDemo->queue;
		m_submitInfo = rtFilterDemo->submitInfo;
		m_presentComplete = rtFilterDemo->semaphores.presentComplete;
		m_renderComplete = rtFilterDemo->semaphores.renderComplete;
		
		for (auto& renderpass : m_renderpasses)
		{
			renderpass->setRtFilterDemo(rtFilterDemo);
			renderpass->prepare();
		}

		// prepare semaphores
		VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();

		// after each renderpass, we need a semaphore signaled that the next renderpass can start
		// except for the last renderpass, so nr -1
		m_semaphores.resize(m_renderpasses.size() - 1);
		for (auto& semaphore : m_semaphores)
		{
			VK_CHECK_RESULT(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &semaphore));
		}
	}

	void RenderpassManager::draw(VkCommandBuffer baseCommandBuffer)
	{

		m_submitInfo.pWaitSemaphores = &m_presentComplete;
		m_submitInfo.pSignalSemaphores = &m_semaphores[0];
		// Signal ready with offscreen semaphore

		for (int idx = 0; idx < m_renderpasses.size()-1; idx++)
		{
			// fetch commandbuffers from renderpass
			m_renderpasses.at(idx)->draw(m_submitInfo.pCommandBuffers, m_submitInfo.commandBufferCount);

			// submit command buffers
			VK_CHECK_RESULT(vkQueueSubmit(m_queue, 1, &m_submitInfo, VK_NULL_HANDLE));

			// set semaphores for next renderpass, as long as 2 semaphores are available
			int availableSemaphores = m_semaphores.size() - 2;
			if (idx < availableSemaphores)
			{
				m_submitInfo.pWaitSemaphores = &m_semaphores[idx];
				m_submitInfo.pSignalSemaphores = &m_semaphores[idx + 1];
			}
		}

		// prepare last semaphores, to preset image when rendering is complete
		m_submitInfo.pWaitSemaphores = &m_semaphores[m_semaphores.size() - 1];
		m_submitInfo.pSignalSemaphores = &m_renderComplete;

		// fetch command buffers from last renderpass
		// TODO: uncomment this and make it work..
		//m_renderpasses.at(m_renderpasses.size() - 1)->draw(m_submitInfo.pCommandBuffers, m_submitInfo.commandBufferCount);

		// using basic command buffer
		m_submitInfo.pCommandBuffers = &baseCommandBuffer;
		m_submitInfo.commandBufferCount = 1;

		VK_CHECK_RESULT(vkQueueSubmit(m_queue, 1, &m_submitInfo, VK_NULL_HANDLE));

		for (auto& renderpass : m_renderpasses)
		{
			renderpass->updateUniformBuffer();
		}
	}
}

