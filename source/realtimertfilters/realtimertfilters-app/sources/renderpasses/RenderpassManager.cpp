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
		for (auto& semaphore : m_semaphores)
		{
			if(semaphore != nullptr)
			{
				vkDestroySemaphore(m_device, semaphore, nullptr);
			}
		}
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
		if(m_semaphores.size() < semaphorecount)
		{
			m_semaphores.resize(semaphorecount);
		}
		for (auto& semaphore : m_semaphores)
		{
			// cleanup old semaphores before creating new ones
			if(semaphore != nullptr)
			{
				vkDestroySemaphore(m_device, semaphore, nullptr);
			}
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
		m_RPF_Gauss->PushTextureAttachment(TextureBinding(Attachment::albedo, TextureBinding::Type::StorageImage_ReadOnly));
		m_RPF_Gauss->PushTextureAttachment(TextureBinding(Attachment::filteroutput, TextureBinding::Type::StorageImage_ReadWrite));
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassPostProcess>(m_RPF_Gauss));

		// Depthtest Postprocess
		m_RPF_DepthTest = std::make_shared<RenderpassPostProcess>();
		m_RPF_DepthTest->ConfigureShader("filter/postprocess_depthTest.frag.spv");
		TextureBinding depth(Attachment::depth, TextureBinding::Type::Sampler_ReadOnly);
		depth.m_AspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		depth.m_PreLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		m_RPF_DepthTest->PushTextureAttachment(depth);
		m_RPF_DepthTest->PushTextureAttachment(TextureBinding(Attachment::filteroutput, TextureBinding::Type::Subpass_Output));
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassPostProcess>(m_RPF_DepthTest));

		// Temporal Accumulation Postprocess
		m_RPF_TempAccu = std::make_shared<RenderpassPostProcess>();
		m_RPF_TempAccu->ConfigureShader("filter/postprocess_tempAccu.frag.spv");
		m_RPF_TempAccu->PushTextureAttachment(TextureBinding(Attachment::position, TextureBinding::Type::Sampler_ReadOnly));
		m_RPF_TempAccu->PushTextureAttachment(TextureBinding(Attachment::normal, TextureBinding::Type::Sampler_ReadOnly));
		m_RPF_TempAccu->PushTextureAttachment(TextureBinding(Attachment::motionvector, TextureBinding::Type::Sampler_ReadOnly));
		m_RPF_TempAccu->PushTextureAttachment(TextureBinding(Attachment::rtoutput, TextureBinding::Type::Sampler_ReadOnly));
		m_RPF_TempAccu->PushTextureAttachment(TextureBinding(Attachment::prev_accumulatedcolor, TextureBinding::Type::Sampler_ReadOnly));
		m_RPF_TempAccu->PushTextureAttachment(TextureBinding(Attachment::intermediate, TextureBinding::Type::Subpass_Output));
		m_RPF_TempAccu->PushTextureAttachment(TextureBinding(Attachment::prev_position, TextureBinding::Type::Sampler_ReadOnly));
		m_RPF_TempAccu->PushTextureAttachment(TextureBinding(Attachment::prev_normal, TextureBinding::Type::Sampler_ReadOnly));
		m_RPF_TempAccu->PushTextureAttachment(TextureBinding(Attachment::prev_historylength, TextureBinding::Type::Sampler_ReadOnly));
		m_RPF_TempAccu->PushTextureAttachment(TextureBinding(Attachment::new_historylength, TextureBinding::Type::Subpass_Output));
		m_RPF_TempAccu->PushUBO(std::dynamic_pointer_cast<UBOInterface, ManagedUBO<S_AccuConfig>>(rtFilterDemo->m_UBO_AccuConfig));
		m_RPF_TempAccu->Push_PastRenderpass_BufferCopy(Attachment::position, Attachment::prev_position);
		m_RPF_TempAccu->Push_PastRenderpass_BufferCopy(Attachment::normal, Attachment::prev_normal);
		m_RPF_TempAccu->Push_PastRenderpass_BufferCopy(Attachment::new_historylength, Attachment::prev_historylength);
		m_RPF_TempAccu->Push_PastRenderpass_BufferCopy(Attachment::intermediate, Attachment::prev_accumulatedcolor);
		m_RPF_TempAccu->Push_PastRenderpass_BufferCopy(Attachment::intermediate, Attachment::atrous_output);
		registerRenderpass(m_RPF_TempAccu);

		// Atrous Postprocess
		
		m_RPF_Atrous = std::make_shared<RenderpassPostProcess>();
		m_RPF_Atrous->ConfigureShader("filter/postprocess_atrous_membarrier.frag.spv");
		m_RPF_Atrous->PushTextureAttachment(TextureBinding(Attachment::atrous_output, TextureBinding::Type::StorageImage_ReadWrite));
		m_RPF_Atrous->PushTextureAttachment(TextureBinding(Attachment::atrous_intermediate, TextureBinding::Type::StorageImage_ReadWrite));
		m_RPF_Atrous->PushTextureAttachment(TextureBinding(Attachment::normal, TextureBinding::Type::Sampler_ReadOnly));
		m_RPF_Atrous->PushTextureAttachment(depth);
		m_RPF_Atrous->PushUBO(std::dynamic_pointer_cast<UBOInterface, ManagedUBO<S_AtrousConfig>>(rtFilterDemo->m_UBO_AtrousConfig));
		registerRenderpass(m_RPF_Atrous);

		// GUI Pass (RasterizerOnly)
		m_RPG_RasterOnly = std::make_shared<RenderpassGui>();
		m_RPG_RasterOnly->m_allowComposition = true;
		m_RPG_RasterOnly->setAttachmentBindings({
			GuiAttachmentBinding(Attachment::position, std::string("GBuffer::Position")),
			GuiAttachmentBinding(Attachment::normal, std::string("GBuffer::Normal")),
			GuiAttachmentBinding(Attachment::albedo, std::string("GBuffer::Albedo")),
			GuiAttachmentBinding(Attachment::motionvector, std::string("GBuffer::Motion")),
			//GuiAttachmentBinding(Attachment::meshid, std::string("GBuffer::MeshId")), // Right now throws validation errors
			GuiAttachmentBinding(Attachment::filteroutput, std::string("Filter Output"))
			});
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassGui>(m_RPG_RasterOnly));

		// GUI Pass (PathtracerOnly)
		m_RPG_PathtracerOnly = std::make_shared<RenderpassGui>();
		m_RPG_PathtracerOnly->m_allowComposition = false;
		m_RPG_PathtracerOnly->m_usePathtracing = true;
		m_RPG_PathtracerOnly->m_useTempAccu = true;
		m_RPG_PathtracerOnly->setAttachmentBindings({
			GuiAttachmentBinding(Attachment::intermediate, std::string("Temporal Accumulation")),
			GuiAttachmentBinding(Attachment::rtoutput, std::string("Raw RT")),
			GuiAttachmentBinding(Attachment::rtdirect, std::string("Direct RT")),
			GuiAttachmentBinding(Attachment::rtindirect, std::string("Indirect RT")),
			GuiAttachmentBinding(Attachment::albedo, std::string("GBuffer::Albedo"))
			});
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassGui>(m_RPG_PathtracerOnly));

		// GUI Pass (SVGF)
		m_RPG_SVGF = std::make_shared<RenderpassGui>();
		m_RPG_SVGF->m_allowComposition = false;
		m_RPG_SVGF->m_usePathtracing = true;
		m_RPG_SVGF->m_useTempAccu = true;
		m_RPG_SVGF->m_useAtrous = true;
		m_RPG_SVGF->setAttachmentBindings({
			GuiAttachmentBinding(Attachment::atrous_output, std::string("A-Trous")),
			GuiAttachmentBinding(Attachment::intermediate, std::string("Temporal Accumulation")),
			GuiAttachmentBinding(Attachment::rtoutput, std::string("Raw RT"))
			});
		registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassGui>(m_RPG_SVGF));

		// BMFR shit
		m_BMFR_Renderpasses.prepareBMFRPasses(rtFilterDemo);

		// GUI Pass (BMFR)
		m_RPG_BMFR = std::make_shared<RenderpassGui>();
		m_RPG_BMFR->m_allowComposition = false;
		m_RPG_BMFR->m_usePathtracing = true;
		m_RPG_BMFR->setAttachmentBindings({
			GuiAttachmentBinding(Attachment::intermediate, std::string("Accumulated w/o albedo")),
			GuiAttachmentBinding(Attachment::filteroutput, std::string("Second accumulation")),
			GuiAttachmentBinding(Attachment::rtoutput, std::string("Raw RT")),
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
		m_QT_RasterizationOnly->push_back(m_RPF_Gauss);
		m_QT_RasterizationOnly->push_back(m_RPG_RasterOnly);

		// PathtracerOnly
		m_QT_PathtracerOnly = std::make_shared<QueueTemplate>();
		m_QT_PathtracerOnly->push_back(m_RP_GBuffer);

		m_QT_PathtracerOnly->push_back(m_RP_PT);
		m_QT_PathtracerOnly->push_back(m_RPF_TempAccu);
		m_QT_PathtracerOnly->push_back(m_RPG_PathtracerOnly);

		// SVGF
		m_QT_SVGF = std::make_shared<QueueTemplate>();
		m_QT_SVGF->push_back(m_RP_GBuffer);

		m_QT_SVGF->push_back(m_RP_PT);
		m_QT_SVGF->push_back(m_RPF_TempAccu);
		m_QT_SVGF->push_back(m_RPF_Atrous);
		// TODO Add SVGF Renderpasses

		m_QT_SVGF->push_back(m_RPG_SVGF);

		// BMFR
		m_QT_BMFR = std::make_shared<QueueTemplate>();
		m_QT_BMFR->push_back(m_RP_GBuffer);

		m_QT_BMFR->push_back(m_RP_PT);
		m_BMFR_Renderpasses.addToQueue(m_QT_BMFR);

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

