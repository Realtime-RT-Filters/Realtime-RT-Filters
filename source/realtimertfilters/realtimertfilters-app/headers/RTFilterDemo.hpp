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
#include "Renderpass_Filter.hpp" //Example Renderpass
#include "Renderpass_PostProcess.hpp"


#define ENABLE_VALIDATION true

// Texture properties
#define TEX_DIM 2048
#define TEX_FILTER VK_FILTER_LINEAR

// Offscreen frame buffer properties
#define FB_DIM TEX_DIM


namespace rtf
{
	class PathTracerManager;

	class RTFilterDemo : public VulkanExampleBase
	{
	public:
		int32_t debugDisplayTarget = 0;

		RenderpassGbuffer* m_RP_GBuffer;
		SpatioTemporalAccumulation m_spatioTemporalAccumulation;
		RaytracingManager m_rtManager;
		PathTracerManager* m_pathTracerManager;
		
		//Attachment manager
		Attachment_Manager* m_attachment_manager;


		//Renderpass
		Renderpass_Gui* m_renderpass_gui;
		RenderpassPostProcess* m_GaussPass;

#pragma region helper_structs

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

		//struct
		//{
		//	vks::Buffer composition;
		//} m_Comp_UnformBuffer;
		vks::Buffer m_Comp_UnformBuffer;

		//struct
		//{
		//	VkPipeline composition;
		//} m_Comp_Pipeline;
		VkPipeline m_Comp_Pipeline;
		VkPipelineLayout m_Comp_PipelineLayout;
		VkDescriptorSet m_Comp_DescriptorSet;
		VkDescriptorSetLayout m_Comp_DescriptorSetLayout;
		vkglTF::Model m_Scene;

#pragma endregion helper_structs

		// One sampler for the frame buffer color attachments
		VkSampler m_DefaultColorSampler;

		// Semaphore used to synchronize between offscreen and final scene rendering
		VkSemaphore m_SemaphoreA = VK_NULL_HANDLE;
		VkSemaphore m_SemaphoreB = VK_NULL_HANDLE;

		RTFilterDemo();

		~RTFilterDemo();

		// Enable physical device features required for this example
		virtual void getEnabledFeatures();

		// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)
		void setupDefaultSampler();

		// Build command buffer for rendering the scene to the offscreen frame buffer attachments
		void setupSemaphores();

		void loadAssets();

		void buildCommandBuffers();

		void Comp_SetupDescriptorPool();

		void Comp_SetupDescriptorSetLayout();

		void Comp_SetupDescriptorSet();

		void Comp_PreparePipelines();

		// Prepare and initialize uniform buffer containing shader uniforms
		void Comp_PrepareUniformBuffers();

		// Update lights and parameters passed to the composition shaders
		void Comp_UpdateUniformBuffer();

		void draw();

		virtual void prepare();

		virtual void render();

		virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);
		
		std::string getShadersPath2();

		bool rt_on = false;
		bool path_tracer_on = false;

		VkPipelineShaderStageCreateInfo LoadShader(std::string shadername, VkShaderStageFlagBits stage);

	};
}

#endif //RTFilterDemo_h
