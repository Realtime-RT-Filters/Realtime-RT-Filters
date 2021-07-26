#ifndef Renderpass_Gbuffer_h
#define Renderpass_Gbuffer_h

#include "../disable_warnings.h"
#include <vulkanexamplebase.h>
#include "../VulkanglTFModel.h"
#include "Renderpass.hpp"
#include "../Attachment_Manager.hpp"

namespace rtf
{

	class RenderpassGbuffer : public Renderpass
	{
	public:
		VkFramebuffer m_FrameBuffer = nullptr;

		FrameBufferAttachment* m_PositionAttachment = nullptr;
		FrameBufferAttachment* m_NormalAttachment = nullptr;
		FrameBufferAttachment* m_AlbedoAttachment = nullptr;
		FrameBufferAttachment* m_MotionAttachment = nullptr;
		FrameBufferAttachment* m_MeshIdAttachment = nullptr;
		FrameBufferAttachment* m_DepthAttachment = nullptr;

		VkDescriptorSet m_DescriptorSetAttachments = nullptr;
		VkDescriptorSet m_DescriptorSetScene = nullptr;
		VkCommandBuffer m_CmdBuffer = nullptr;
		VkPipelineCache m_PipelineCache;

		vkglTF::Model* m_Scene = nullptr;

		RenderpassGbuffer();
		virtual ~RenderpassGbuffer() { cleanUp(); }

		// Inherited via Renderpass
		virtual void prepare() override;
		void prepareRenderpass();
		void prepareAttachments();
		void setupDescriptorPool();
		void setupDescriptorSetLayout();
		void setupDescriptorSet();
		void buildCommandBuffer();
		void preparePipeline();

		virtual void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) override;
		virtual void cleanUp() override;
	};
}

#endif //Renderpass_Gbuffer_h
