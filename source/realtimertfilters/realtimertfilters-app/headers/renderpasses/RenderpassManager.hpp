#pragma once
#include "Renderpass.hpp"

namespace rtf 
{
	class RTFilterDemo;

	class RenderpassManager
	{
	public:
		RenderpassManager() {};
		virtual ~RenderpassManager();

		void addRenderpass(std::shared_ptr<Renderpass> renderpass);
		void prepare(RTFilterDemo* rtFilterDemo);
		void draw(VkCommandBuffer baseCommandBuffer);
		void updateUniformBuffer();

	protected:
		std::vector<VkSemaphore> m_semaphores;
		std::vector<std::shared_ptr<Renderpass>> m_renderpasses;
		VkDevice m_device{};
		VkSubmitInfo m_submitInfo{};
		VkQueue m_queue{};
		VkSemaphore m_presentComplete{};
		VkSemaphore m_renderComplete{};
	};
}