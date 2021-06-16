#ifndef Renderpass_PostProcess_h
#define Renderpass_PostProcess_h

#include "Renderpass.hpp"
#include "../Attachment_Manager.hpp"
#include "../TextureBinding.hpp"
#include "../ManagedUBO.hpp"

namespace rtf
{

	class RenderpassPostProcess : public Renderpass
	{
	public:

		RenderpassPostProcess();

		void ConfigureShader(const std::string& shadername);
		void PushTextureAttachment(const TextureBinding& attachmentbinding);
		void Push_PastRenderpass_BufferCopy(Attachment sourceAttachment, Attachment destinationAttachment);
		void PushUBO(const UBOPtr& ubo);

		virtual void prepare() override;
		virtual void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) override;
		virtual void cleanUp() override;

		/// <summary>
		/// Manages static information to be used by all postprocess renderpasses
		/// </summary>
		class StaticsContainer
		{
		public:
			VkSampler m_ColorSampler_Direct = nullptr;
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
		std::vector<TextureBinding> m_TextureBindings{};

		/// use attachment copies to copy the content of one attachment into another, after the renderpass has finished
		std::vector<std::pair<Attachment, Attachment>> m_AttachmentCopies{};

		inline size_t getAttachmentCount() { return m_TextureBindings.size(); }


		StaticsContainer* Statics = nullptr;

		VkFramebuffer m_Framebuffer = nullptr;
		VkCommandBuffer m_CmdBuffer = nullptr;

		// [0] = Attachments/Storage Images, [1] = UBOs
		uint32_t DESCRIPTORSET_IMAGES = 0;
		uint32_t DESCRIPTORSET_UBOS = 1;
		VkDescriptorSetLayout m_descriptorSetLayouts[2]{};
		VkDescriptorSet m_descriptorSets[2]{};

		struct PushConstantsContainer
		{
			uint32_t SCR_WIDTH = 0;
			uint32_t SCR_HEIGHT = 0;
		};
		PushConstantsContainer m_PushConstants{};

		virtual bool preprocessTextureBindings();
		virtual void createRenderPass();
		virtual void setupDescriptorSetLayout();
		virtual void setupDescriptorSet();
		virtual void setupPipeline();
		virtual void setupFramebuffer();
		virtual void buildCommandBuffer();

		std::vector<UBOPtr> m_UBOs{};
		inline size_t getUboCount() { return m_UBOs.size(); }

	};
}

#endif //Renderpass_PostProcess_h