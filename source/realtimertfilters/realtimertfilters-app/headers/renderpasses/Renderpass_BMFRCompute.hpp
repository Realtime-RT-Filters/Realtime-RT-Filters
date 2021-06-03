#ifndef Renderpass_BMFRCompute_h
#define Renderpass_BMFRCompute_h

#include "Renderpass.hpp"

namespace rtf
{
	class RenderpassBMFRCompute : public Renderpass
	{
	public:

		RenderpassBMFRCompute() = default;

		virtual void prepare() override; // Setup pipelines, passes, descriptorsets, etc.
		virtual void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) override;
		virtual void cleanUp() override; // Cleanup any mess you made (is called from the destructor)


	protected:
		void buildCommandBuffer();

		uint32_t m_compute_QueueFamilyIndex;
		VkQueue m_computeQueue;
		VkPipelineCache m_pipelineCache;
		VkCommandPool m_commandPool;
		VkCommandBuffer m_cmdBuffer;
	};
}

#endif