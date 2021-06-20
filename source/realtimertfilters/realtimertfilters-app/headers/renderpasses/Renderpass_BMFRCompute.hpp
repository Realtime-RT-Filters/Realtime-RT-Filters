#ifndef Renderpass_BMFRCompute_h
#define Renderpass_BMFRCompute_h

#include "Renderpass.hpp"
#include "../Attachment_Manager.hpp"

namespace bmfr
{
	class FeatureBuffer
	{
	public:
		rtf::Attachment				m_AttachmentId;
		rtf::FrameBufferAttachment*	m_Attachment;
		float						m_Exponent;
	};

	const uint32_t BLOCK_SIZE_X = 32;
	const uint32_t BLOCK_SIZE_Y = 32;

	class RenderpassBMFRCompute : public rtf::Renderpass
	{
	public:

		rtf::FrameBufferAttachment* m_RTInput = nullptr;
		rtf::FrameBufferAttachment* m_Positions = nullptr;
		rtf::FrameBufferAttachment* m_Normals = nullptr;
		rtf::FrameBufferAttachment* m_Output = nullptr;

		RenderpassBMFRCompute() = default;

		//void PushFeatureBuffer(FeatureBuffer& featurebuffer);

		virtual void prepare() override; // Setup pipelines, passes, descriptorsets, etc.
		void AllocateAndWriteDescriptorSet();
		void CreateDescriptorSetLayoutAndPipeline();
		virtual void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) override;
		virtual void cleanUp () override; // Cleanup any mess you made (is called from the destructor)

		VkExtent2D m_Blocks;

	protected:
		void buildCommandBuffer();

		uint32_t m_compute_QueueFamilyIndex{};
		VkQueue m_computeQueue{};
		VkPipelineCache m_pipelineCache{};
		VkCommandPool m_commandPool{};
		VkCommandBuffer m_cmdBuffer{};

		std::vector<FeatureBuffer> m_FeatureBuffer{};
	};
}

#endif