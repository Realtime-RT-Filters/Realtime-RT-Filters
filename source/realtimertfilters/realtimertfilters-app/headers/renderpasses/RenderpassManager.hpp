#pragma once
#include "Renderpass.hpp"
#include "../BMFR.hpp"

namespace rtf 
{
	class RTFilterDemo;
	class RenderpassGbuffer;
	class RenderpassGui;
	class RenderpassPostProcess;
	class RenderpassPathTracer;

	enum class SupportedQueueTemplates : int32_t
	{
		RasterizationOnly,
		PathtracerOnly,
		SVGF,
		BMFR,
		MAX_ENUM
	};

	class RenderpassManager
	{
	public:
		RenderpassManager() {};
		virtual ~RenderpassManager();

		void setQueueTemplate(SupportedQueueTemplates queueTemplate);
		void prepare(RTFilterDemo* rtFilterDemo, size_t semaphorecount);
		void draw(VkCommandBuffer baseCommandBuffer);
		void updateUniformBuffer();

		// RENDERPASSES ********

		std::shared_ptr<RenderpassGbuffer> m_RP_GBuffer{};
		std::shared_ptr<RenderpassGui> m_RP_Gui{};
		std::shared_ptr<RenderpassPathTracer> m_PT{};
		
		std::shared_ptr<RenderpassPostProcess> m_RPF_Gauss{};
		std::shared_ptr<RenderpassPathTracer> m_RP_PT{};
		std::shared_ptr<RenderpassPostProcess> m_RPF_DepthTest{};
		std::shared_ptr<RenderpassPostProcess> m_RPF_TempAccu{};
		std::shared_ptr<RenderpassPostProcess> m_RPF_SVGF_Accumulation{};
		std::shared_ptr<RenderpassPostProcess> m_RPF_SVGF_Atrous{};
		std::shared_ptr<RenderpassPostProcess> m_RPF_Atrous{};

		std::shared_ptr<RenderpassGui> m_RPG_RasterOnly{};
		std::shared_ptr<RenderpassGui> m_RPG_PathtracerOnly{};
		std::shared_ptr<RenderpassGui> m_RPG_SVGF{};
		std::shared_ptr<RenderpassGui> m_RPG_BMFR{};

		std::vector<RenderpassPtr> m_AllRenderpasses{};

		// QUEUETEMPLATES ********

		QueueTemplatePtr m_QT_RasterizationOnly{};
		QueueTemplatePtr m_QT_PathtracerOnly{};
		QueueTemplatePtr m_QT_SVGF{};
		QueueTemplatePtr m_QT_BMFR{};

		// The currently active queue template
		QueueTemplatePtr m_QT_Active{};
		std::shared_ptr<RenderpassGui> m_RPG_Active{};

		friend bmfr::RenderPasses;

		// BMFR specialized stuff
		bmfr::RenderPasses m_BMFR_Renderpasses{};

	protected:

		void prepareRenderpasses(RTFilterDemo* rtFilterDemo);
		void registerRenderpass(const std::shared_ptr<Renderpass>& renderpass);
		void buildQueueTemplates();


		// SEMAPHORES ********

		std::vector<VkSemaphore> m_semaphores{};
		VkSemaphore m_presentComplete{};
		VkSemaphore m_renderComplete{};

		// OTHER ********

		VkDevice m_device{};
		VkSubmitInfo m_submitInfo{};
		VkQueue m_queue{};
	};
}