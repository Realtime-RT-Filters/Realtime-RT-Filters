#include "../../headers/renderpasses/RenderpassManager.hpp"
#include "../../headers/renderpasses/Renderpass.hpp"
#include "../../headers/renderpasses/Renderpass_Gbuffer.hpp"
#include "../../headers/renderpasses/Renderpass_PostProcess.hpp"
#include "../../headers/renderpasses/Renderpass_Gui.hpp"
#include "../../headers/renderpasses/Renderpass_PathTracer.hpp"
#include "../../headers/RTFilterDemo.hpp"

namespace rtf
{
	RenderpassManager::~RenderpassManager()
	{

	}

	void RenderpassManager::setQueueTemplate(SupportedQueueTemplates queueTemplate)
	{
		switch (queueTemplate)
		{
		case SupportedQueueTemplates::RasterizationOnly:
			m_QT_Active = m_QT_RasterizationOnly;
			m_RPG_Active = m_RPG_RasterOnly;
			break;
		case SupportedQueueTemplates::PathtracerOnly:
			m_QT_Active = m_QT_PathtracerOnly;
			m_RPG_Active = m_RPG_PathtracerOnly;
			break;
		case SupportedQueueTemplates::SVGF:
			m_QT_Active = m_QT_SVGF;
			m_RPG_Active = m_RPG_SVGF;
			break;
		case SupportedQueueTemplates::BMFR:
			m_QT_Active = m_QT_BMFR;
			m_RPG_Active = m_RPG_BMFR;
			break;
		}
	}

#pragma region Prepare

	void RenderpassManager::prepare(RTFilterDemo* rtFilterDemo, size_t semaphorecount)
	{
		// copy vulkan handles
		m_device = rtFilterDemo->device;
		m_queue = rtFilterDemo->queue;
		m_submitInfo = rtFilterDemo->submitInfo;
		m_presentComplete = rtFilterDemo->semaphores.presentComplete;
		m_renderComplete = rtFilterDemo->semaphores.renderComplete;

		// prepare semaphores
		VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();

		// after each renderpass, we need a semaphore signaled that the next renderpass can start
		// except for the last renderpass, so nr -1
		m_semaphores.resize(semaphorecount);
		for (auto& semaphore : m_semaphores)
		{
			VK_CHECK_RESULT(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &semaphore));
		}

		prepareRenderpasses(rtFilterDemo);
		buildQueueTemplates();
		setQueueTemplate(SupportedQueueTemplates::RasterizationOnly);
	}

	void RenderpassManager::prepareRenderpasses(RTFilterDemo* rtFilterDemo)
	{
		// CREATE RENDERPASS OBJECTS AND PRECONFIGURE

		// GBuffer
		m_RP_GBuffer = std::make_shared<RenderpassGbuffer>();
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassGbuffer>(m_RP_GBuffer));

		// Gauss Postprocess
		m_RPF_Gauss = std::make_shared<RenderpassPostProcess>();
		m_RPF_Gauss->ConfigureShader("filter/postprocess_gauss.frag.spv");
		m_RPF_Gauss->PushAttachment(rtFilterDemo->m_attachmentManager->getAttachment(Attachment::albedo), RenderpassPostProcess::AttachmentUse::ReadOnly);
		m_RPF_Gauss->PushAttachment(rtFilterDemo->m_attachmentManager->getAttachment(Attachment::position), RenderpassPostProcess::AttachmentUse::WriteOnly);
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassPostProcess>(m_RPF_Gauss));

		// GUI Pass (RasterizerOnly)
		m_RPG_RasterOnly = std::make_shared<RenderpassGui>();
		m_RPG_RasterOnly->setAttachmentBindings({
			GuiAttachmentBinding(Attachment::position, std::string("GBuffer::Position")),
			GuiAttachmentBinding(Attachment::normal, std::string("GBuffer::Normal")),
			GuiAttachmentBinding(Attachment::albedo, std::string("GBuffer::Albedo")),
			GuiAttachmentBinding(Attachment::motionvector, std::string("GBuffer::Motion"))
			});
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassGui>(m_RPG_RasterOnly));

		// GUI Pass (PathtracerOnly)
		m_RPG_PathtracerOnly = std::make_shared<RenderpassGui>();
		m_RPG_PathtracerOnly->m_enableComposition = false;
		m_RPG_PathtracerOnly->setAttachmentBindings({
			GuiAttachmentBinding(Attachment::rtoutput, std::string("RT Output")),
			GuiAttachmentBinding(Attachment::albedo, std::string("GBuffer::Albedo"))
			});
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassGui>(m_RPG_PathtracerOnly));

		// GUI Pass (SVGF)
		m_RPG_SVGF = std::make_shared<RenderpassGui>();
		m_RPG_SVGF->setAttachmentBindings({
			GuiAttachmentBinding(Attachment::position, std::string("GBuffer::Position")),
			GuiAttachmentBinding(Attachment::normal, std::string("GBuffer::Normal")),
			GuiAttachmentBinding(Attachment::albedo, std::string("GBuffer::Albedo"))
			});
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassGui>(m_RPG_SVGF));

		// GUI Pass (BMFR)
		m_RPG_BMFR = std::make_shared<RenderpassGui>();
		m_RPG_BMFR->setAttachmentBindings({
			GuiAttachmentBinding(Attachment::position, std::string("GBuffer::Position")),
			GuiAttachmentBinding(Attachment::normal, std::string("GBuffer::Normal")),
			GuiAttachmentBinding(Attachment::albedo, std::string("GBuffer::Albedo"))
			});
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassGui>(m_RPG_BMFR));

		// Path Tracer Pass
		m_RP_PT = std::make_shared<RenderpassPathTracer>();
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassPathTracer>(m_RP_PT));
		
		// SET RTFILTERDEMO AND PREPARE RENDERPASSES
		for (auto& renderpass : m_AllRenderpasses)
		{
			renderpass->setRtFilterDemo(rtFilterDemo);
			renderpass->prepare();
		}
	}

	void RenderpassManager::registerRenderpass(const std::shared_ptr<Renderpass>& renderpass)
	{
		m_AllRenderpasses.push_back(renderpass);
	}

	void RenderpassManager::buildQueueTemplates()
	{
		// RasterizationOnly
		m_QT_RasterizationOnly = std::make_shared<QueueTemplate>();
		m_QT_RasterizationOnly->push_back(m_RP_GBuffer);
		m_QT_RasterizationOnly->push_back(m_RPG_RasterOnly);

		// PathtracerOnly
		m_QT_PathtracerOnly = std::make_shared<QueueTemplate>();
		m_QT_PathtracerOnly->push_back(m_RP_GBuffer);

		// TODO Add Pathtracer Renderpass
		m_QT_PathtracerOnly->push_back(m_RP_PT);
		m_QT_PathtracerOnly->push_back(m_RPG_PathtracerOnly);

		// SVGF
		m_QT_SVGF = std::make_shared<QueueTemplate>();
		m_QT_SVGF->push_back(m_RP_GBuffer);

		// TODO Add Pathtracer Renderpass
		// TODO Add SVGF Renderpasses

		m_QT_SVGF->push_back(m_RPG_SVGF);

		// BMFR
		m_QT_BMFR = std::make_shared<QueueTemplate>();
		m_QT_BMFR->push_back(m_RP_GBuffer);

		// TODO Add Pathtracer Renderpass
		// TODO Add BMFR Renderpasses

		m_QT_BMFR->push_back(m_RPG_BMFR);
	}

#pragma endregion
#pragma region Update/Draw

	void RenderpassManager::draw(VkCommandBuffer baseCommandBuffer)
	{
		m_submitInfo.pWaitSemaphores = &m_presentComplete;
		m_submitInfo.pSignalSemaphores = &m_semaphores[0];
		// Signal ready with offscreen semaphore

		for (int idx = 0; idx < m_QT_Active->size() - 1; idx++)
		{
			// fetch commandbuffers from renderpass
			m_QT_Active->at(idx)->draw(m_submitInfo.pCommandBuffers, m_submitInfo.commandBufferCount);

			// submit command buffers
			VK_CHECK_RESULT(vkQueueSubmit(m_queue, 1, &m_submitInfo, VK_NULL_HANDLE));

			// set semaphores for next renderpass, as long as 1 additional semaphores is available
			int availableSemaphores = m_semaphores.size() - 1 - idx;
			m_submitInfo.pWaitSemaphores = &m_semaphores[idx];
			if (availableSemaphores > 0)
			{
				m_submitInfo.pSignalSemaphores = &m_semaphores[size_t(idx + 1)];
			}
		}

		// prepare last semaphores, to preset image when rendering is complete
		m_submitInfo.pSignalSemaphores = &m_renderComplete;

		// fetch command buffers from last renderpass
		m_QT_Active->at(m_QT_Active->size() - 1)->draw(m_submitInfo.pCommandBuffers, m_submitInfo.commandBufferCount);

		// using basic command buffer
		//m_submitInfo.pCommandBuffers = &baseCommandBuffer;
		//m_submitInfo.commandBufferCount = 1;

		VK_CHECK_RESULT(vkQueueSubmit(m_queue, 1, &m_submitInfo, VK_NULL_HANDLE));
	}

	void RenderpassManager::updateUniformBuffer()
	{
		for (auto& renderpass : m_AllRenderpasses)
		{
			renderpass->updateUniformBuffer();
		}
	}

#pragma endregion
}

