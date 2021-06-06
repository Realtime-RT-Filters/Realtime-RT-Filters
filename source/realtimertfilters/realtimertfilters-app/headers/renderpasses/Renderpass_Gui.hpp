#ifndef Renderpass_Gui_h
#define Renderpass_Gui_h

#include "../disable_warnings.h"
#include <VulkanDevice.h>
#include <vulkanexamplebase.h>
#include "../VulkanglTFModel.h"
#include "../Attachment_Manager.hpp"
#include "Renderpass.hpp"

#include "vulkanexamplebase.h"

namespace rtf
{
	class GuiAttachmentBinding
	{
	public:
		Attachment m_AttachmentId{};
		FrameBufferAttachment* m_Attachment = nullptr;
		std::string m_Displayname = "Generic Attachment";

		GuiAttachmentBinding() = default;
		GuiAttachmentBinding(Attachment attachmentId, std::string& display) : m_AttachmentId(attachmentId), m_Displayname(display) {}
	};

	/// <summary>
	/// Class which acts as final render pass, showing output with UI
	/// </summary>
    class RenderpassGui : public Renderpass
	{
	protected:

		std::vector<GuiAttachmentBinding> m_attachments{};
		std::vector<std::string> m_dropoutOptions{};


		vkglTF::Model* m_Scene = nullptr;


		// Need to know the swapchain
		VulkanSwapChain* m_swapchain;
		std::vector<VkCommandBuffer>* m_commandBuffers{};
		uint32_t* m_currentBuffer{};


		//Old stuff taken from deferred example to allow a render composition
		vks::Buffer m_Buffer;

		struct Light
		{
			glm::vec4 position{};
			glm::vec3 color{};
			float radius = 0.f;
		};

		struct
		{
			Light lights[6]{};
			glm::vec4 viewPos{};
			int debugDisplayTarget = 0;
		} m_composition_ubo;

		vks::Buffer m_composition_UBO_buffer;

		Camera* m_camera{};
		float* m_timer{};
		int32_t* m_debugDisplayTarget = 0;
		//End of legacy stuff

		void prepareUBO();
		void updateUniformBuffer();

		void setupDescriptorSetLayout();
		void setupDescriptorPool();
		void setupDescriptorSet();

		void prepareRenderpass();
		void preparePipelines();


	public:
		RenderpassGui();
		~RenderpassGui();

		void setAttachmentBindings(std::vector<GuiAttachmentBinding> attachmentBindings);
		inline std::vector<std::string> getDropoutOptions() { return m_dropoutOptions; }

		void buildCommandBuffer();
		virtual void prepare() override;
		virtual void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) override;
	};
}

#endif //Renderpass_Gui_h
