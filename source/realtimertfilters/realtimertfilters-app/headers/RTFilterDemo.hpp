/*
* Vulkan Example - Deferred shading with multiple render targets (aka G-Buffer) example
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#ifndef RTFilterDemo_h
#define RTFilterDemo_h

#include "disable_warnings.h"
#include <VulkanRaytracingSample.h>
#include "VulkanglTFModel.h"
#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"


#include "Attachment_Manager.hpp"
#include "Renderpass_Gbuffer.hpp"
#include "SpatioTemporalAccumulation.hpp"
#include "RaytracingManager.hpp"
#include "Renderpass_Gui.hpp"

//All Filter render passes here
#include "Renderpass_Filter.hpp"


#define ENABLE_VALIDATION true

// Texture properties
#define TEX_DIM 2048
#define TEX_FILTER VK_FILTER_LINEAR

// Offscreen frame buffer properties
#define FB_DIM TEX_DIM


namespace rtf
{
	class RTFilterDemo : public VulkanExampleBase
	{
	public:
		int32_t debugDisplayTarget = 0;

//		RenderpassGbuffer m_RP_GBuffer;
		SpatioTemporalAccumulation m_spatioTemporalAccumulation;
		RaytracingManager m_rtManager;
		
		//Attachment manager
		Attachment_Manager* m_attachment_manager;


		//Renderpass
		Renderpass_Gui* m_renderpass_gui;

#pragma region helper_structs
		struct
		{
			glm::mat4 projection;
			glm::mat4 model;
			glm::mat4 view;
			glm::vec4 instancePos[3];
		} uboOffscreenVS;

		struct Light
		{
			glm::vec4 position;
			glm::vec3 color;
			float radius;
		};

		struct
		{
			Light lights[6];
			glm::vec4 viewPos;
			int debugDisplayTarget = 0;
		} uboComposition;

		struct
		{
			vks::Buffer offscreen;
			vks::Buffer composition;
		} uniformBuffers;

		struct
		{
			VkPipeline offscreen;
			VkPipeline composition;
		} pipelines;
		VkPipelineLayout pipelineLayout;
		VkPipelineLayout pipelineLayoutOffscreen;


		struct
		{
			VkDescriptorSet model;
		} descriptorSetsGBufferScene;
		VkDescriptorSet descriptorSetGBuffer;
		VkDescriptorSetLayout descriptorSetLayoutGBuffer;
		vkglTF::Model m_Scene;

		struct FrameBuffer
		{
			int32_t width, height;
			VkFramebuffer frameBuffer;
			FrameBufferAttachment *position, *normal, *albedo;
			FrameBufferAttachment* depth;
			VkRenderPass renderPass;
		} offScreenFrameBuf;
#pragma endregion helper_structs

		// One sampler for the frame buffer color attachments
		VkSampler colorSampler;

		VkCommandBuffer offScreenCmdBuffer = VK_NULL_HANDLE;

		// Semaphore used to synchronize between offscreen and final scene rendering
		VkSemaphore offscreenSemaphore = VK_NULL_HANDLE;

		RTFilterDemo();

		~RTFilterDemo();

		// Enable physical device features required for this example
		virtual void getEnabledFeatures();

		// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)
		void prepareOffscreenFramebuffer();

		// Build command buffer for rendering the scene to the offscreen frame buffer attachments
		void buildDeferredCommandBuffer();

		void loadAssets();

		void buildCommandBuffers();

		void setupDescriptorPool();

		void setupDescriptorSetLayout();

		void setupDescriptorSet();

		void preparePipelines();

		// Prepare and initialize uniform buffer containing shader uniforms
		void prepareUniformBuffers();

		// Update matrices used for the offscreen rendering of the scene
		void updateUniformBufferOffscreen();

		// Update lights and parameters passed to the composition shaders
		void updateUniformBufferComposition();

		void draw();

		virtual void prepare();

		virtual void render();

		virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);

		std::string getShadersPath2();

		bool rt_on = false;

	};
}

#endif //RTFilterDemo_h
