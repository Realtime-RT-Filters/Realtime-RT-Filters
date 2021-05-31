#ifndef Renderpass_Gui_h
#define Renderpass_Gui_h

#include "../disable_warnings.h"
#include <VulkanDevice.h>
#include <vulkanexamplebase.h>
#include "../VulkanglTFModel.h"
#include "../Attachment_Manager.hpp"
#include "Renderpass.hpp"

#include "vulkanexamplebase.h"
/*
namespace rtf
{
	/// <summary>
	/// Class which acts as final render pass, showing output with UI
	/// </summary>
    class Renderpass_Gui : public Renderpass
	{
	protected:

		
		FrameBufferAttachment* m_position, * m_normal, * m_albedo, * m_motionvector, * m_rtoutput, * m_filteroutput;
		
		VkDescriptorSet descriptorSetInputAttachments;



		// One sampler for the frame buffer color attachments
		VkSampler colorSampler;

		vkglTF::Model* m_Scene = nullptr;


		// Need to know the swapchain
		VulkanSwapChain* m_swapchain;


		VkCommandBuffer m_CmdBuffer = nullptr;


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
		} m_Comp_UBO;

		Camera* m_camera;
		float* m_timer;
		int32_t* m_debugDisplayTarget = 0;
		//End of legacy stuff



		void prepareUBO();
		void updateUBO();


		void setupDescriptorSetLayout();
		void setupDescriptorPool();
		void setupDescriptorSet();


		void prepareRenderpass();
		void PreparePipelines();

	public:
		Renderpass_Gui(VkInstance instance, vks::VulkanDevice* device, Attachment_Manager* attachmentManager, RTFilterDemo* demo, VulkanSwapChain* swapChain, float* timer, int* debugDisplayTarget, Camera* camera);
		~Renderpass_Gui();

		void buildCommandBuffer();
		virtual void prepare() override;


		virtual void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) override;
	};
}
*/
#endif //Renderpass_Gui_h
