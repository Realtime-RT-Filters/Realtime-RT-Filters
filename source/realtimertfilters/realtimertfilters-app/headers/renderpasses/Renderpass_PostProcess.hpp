#ifndef Renderpass_PostProcess_h
#define Renderpass_PostProcess_h

#include "Renderpass.hpp"
#include "../Attachment_Manager.hpp"

namespace rtf
{
	class RenderpassPostProcess : public Renderpass
	{
	public:
		enum class AttachmentUse
		{
			ReadOnly,
			WriteOnly,
			ReadWrite,
		};

		RenderpassPostProcess();

		void ConfigureShader(const std::string& shadername);
		void PushAttachment(FrameBufferAttachment* attachment, AttachmentUse use);
		void PushAttachment(FrameBufferAttachment* attachment, AttachmentUse use, VkImageLayout initialLayout, VkImageLayout finalLayout);

		virtual void prepare() override;
		virtual void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) override;
		virtual void cleanUp() override;

		class StaticsContainer 
		{
		public:
			VkSampler m_ColorSampler_Standard = nullptr;
			VkSampler m_ColorSampler_Normalized = nullptr;
			VkPipelineShaderStageCreateInfo m_VertexPassthroughShader{};
			VkPipelineCache m_PipelineCache = nullptr;
			vks::VulkanDevice* m_Device = nullptr;

			StaticsContainer(RTFilterDemo* demo);
			~StaticsContainer();

			StaticsContainer(StaticsContainer& other) = delete;
			StaticsContainer(StaticsContainer&& other) = delete;
			void operator=(StaticsContainer& other) = delete;
		};

	protected:
		bool m_IsPrepared = false;

		std::string m_Shadername;
		struct AttachmentContainer
		{
		public:
			FrameBufferAttachment* m_Attachment;
			AttachmentUse m_Use;
			VkImageLayout m_InitialLayout;
			VkImageLayout m_FinalLayout;
		};
		std::vector<AttachmentContainer> m_Attachments{};
		size_t m_CombinedAttachmentCount = 0;

		StaticsContainer* Statics = nullptr;

		VkFramebuffer m_Framebuffer = nullptr;
		VkDescriptorSet m_AttachmentDescriptorSet = nullptr;
		VkCommandBuffer m_CmdBuffer = nullptr;

		struct PushConstantsContainer
		{
			uint32_t SCR_WIDTH = 0;
			uint32_t SCR_HEIGHT = 0;
		};
		PushConstantsContainer m_PushConstants{};

		virtual void createRenderPass();
		virtual void setupDescriptorSetLayout();
		virtual void setupDescriptorSet();
		virtual void setupPipeline();
		virtual void setupFramebuffer();
		virtual void buildCommandBuffer();
	};
}

#endif Renderpass_PostProcess_h