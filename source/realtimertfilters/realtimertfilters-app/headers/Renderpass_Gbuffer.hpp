#ifndef Renderpass_Gbuffer_h
#define Renderpass_Gbuffer_h

#include "disable_warnings.h"
#include <vulkanexamplebase.h>
#include "VulkanglTFModel.h"
#include "Renderpass.hpp"
#include "Attachment_Manager.hpp"

namespace rtf
{

	struct UBO_GBuffer
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 old_projection;
		glm::mat4 old_model;
		glm::mat4 old_view;
	};

	void UBO_GBuffer_PushCamera(UBO_GBuffer& ubo,Camera& camera);

	class RenderpassGbuffer : public Renderpass
	{
	public:
		VkFramebuffer m_FrameBuffer = nullptr;

		FrameBufferAttachment* m_PositionAttachment = nullptr;
		FrameBufferAttachment* m_NormalAttachment = nullptr;
		FrameBufferAttachment* m_AlbedoAttachment = nullptr;
		FrameBufferAttachment* m_MotionAttachment = nullptr;
		FrameBufferAttachment* m_DepthAttachment = nullptr;

		vks::Buffer m_Buffer = {};
		UBO_GBuffer m_UBO = {};
		VkDescriptorSet m_DescriptorSetAttachments = nullptr;
		VkDescriptorSet m_DescriptorSetScene = nullptr;
		VkCommandBuffer m_CmdBuffer = nullptr;
		VkPipelineCache m_PipelineCache;

		vkglTF::Model* m_Scene = nullptr;


		RenderpassGbuffer(VkInstance instance, vks::VulkanDevice* device, Attachment_Manager* attachmentManager, RTFilterDemo* demo);

		// Inherited via Renderpass
		virtual void prepare() override;
		void prepareRenderpass();
		void prepareAttachments();
		void prepareUBOs();
		void updateUniformBuffer(Camera& camera);
		void setupDescriptorPool();
		void setupDescriptorSetLayout();
		void setupDescriptorSet();
		void buildCommandBuffer();
		void preparePipeline();

		virtual void draw(VkQueue queue) override;
		virtual void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) override;

		virtual void cleanUp() override;
	};
}

#endif //Renderpass_Gbuffer_h
