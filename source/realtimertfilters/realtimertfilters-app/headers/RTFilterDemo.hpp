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

#include "Renderpass_Gbuffer.hpp"
#include "SpatioTemporalAccumulation.hpp"


#define ENABLE_VALIDATION false

// Texture properties
#define TEX_DIM 2048
#define TEX_FILTER VK_FILTER_LINEAR

// Offscreen frame buffer properties
#define FB_DIM TEX_DIM

#include "Attachment_Manager.hpp"

namespace rtf
{
	class RTFilterDemo : public VulkanExampleBase
	{
	public:
		int32_t debugDisplayTarget = 0;

//		RenderpassGbuffer m_RP_GBuffer;
		SpatioTemporalAccumulation m_spatioTemporalAccumulation;
		
		//Attachment manager
		Attachment_Manager* m_attachment_manager;

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


		struct
		{
			VkDescriptorSet model;
		} descriptorSetsGBufferScene;
		VkDescriptorSet descriptorSetGBuffer;
		VkDescriptorSetLayout descriptorSetLayoutGBuffer;
		VkDescriptorSet rt_descriptorSet;
		VkDescriptorSetLayout rt_descriptorSetLayout;
		vkglTF::Model m_Scene;

		struct FrameBuffer
		{
			int32_t width, height;
			VkFramebuffer frameBuffer;
			FrameBufferAttachment *position, *normal, *albedo;
			FrameBufferAttachment* depth;
			VkRenderPass renderPass;
		} offScreenFrameBuf;

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

		void prepare();

		virtual void render();

		virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);



		//Ray tracing components




		AccelerationStructure bottomLevelAS;
		AccelerationStructure topLevelAS;

		std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
		struct ShaderBindingTables {
			ShaderBindingTable raygen;
			ShaderBindingTable miss;
			ShaderBindingTable hit;
		} shaderBindingTables;

		struct UniformData {
			glm::mat4 viewInverse;
			glm::mat4 projInverse;
			glm::vec4 lightPos;
			int32_t vertexSize;
		} uniformData;
		vks::Buffer ubo;


		VkPipeline rt_pipeline;
		VkPipelineLayout rt_pipelineLayout;



		void createBottomLevelAccelerationStructure();
		void createTopLevelAccelerationStructure();
		void createShaderBindingTables();
		void createRayTracingPipeline();
		void createDescriptorSets();
		void createUniformBuffer();
		void handleResize();
		void rt_buildCommandBuffers();
		void updateUniformBuffers();
		void rt_draw();

		bool rt_on = false;

	};
}

#endif RTFilterDemo_h