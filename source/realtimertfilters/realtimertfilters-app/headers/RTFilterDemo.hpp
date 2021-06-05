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

#include "SpatioTemporalAccumulation.hpp"
#include "RaytracingManager.hpp"

#include <memory>


#define ENABLE_VALIDATION true

// Texture properties
#define TEX_DIM 2048
#define TEX_FILTER VK_FILTER_LINEAR

// Offscreen frame buffer properties
#define FB_DIM TEX_DIM


namespace rtf
{
	class PathTracerManager;
	class RenderpassManager;

	class RenderpassGbuffer;
	class RenderpassGui;
	class RenderpassPathTracer;

	class RTFilterDemo : public VulkanExampleBase
	{
	public:
		int32_t debugDisplayTarget = 0;

		SpatioTemporalAccumulation m_spatioTemporalAccumulation;
		RaytracingManager m_rtManager;
		PathTracerManager* m_pathTracerManager{};
		
		//Attachment manager
		Attachment_Manager* m_attachmentManager;

		//Renderpass
		friend RenderpassManager;
		RenderpassManager* m_renderpassManager{};
		int32_t m_RenderMode = 0;

		// access to attributes for the renderpasses
		friend RenderpassGbuffer;
		friend RenderpassGui;
		friend RenderpassPathTracer;


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
		} m_composition_ubo;

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
		std::shared_ptr<RenderpassGui> m_renderpassGui;

#pragma endregion helper_structs

		// One sampler for the frame buffer color attachments
		VkSampler m_DefaultColorSampler;

		RTFilterDemo();

		~RTFilterDemo();

		// Enable physical device features required for this example
		virtual void getEnabledFeatures();

		// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)
		void setupDefaultSampler();

		void loadAssets();

		// called by base class
		void buildCommandBuffers() override;

		virtual void prepare() override;

		void prepareRenderpasses();
		void buildQueueTemplates();

		virtual void render() override;

		virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay) override;
		
		std::string getShadersPath2();

		std::wstring getShadersPathW();

		bool rt_on = false;
		bool path_tracer_on = false;
		bool gui_rp_on = false;

		VkPipelineShaderStageCreateInfo LoadShader(std::string shadername, VkShaderStageFlagBits stage);

	};
}

#endif //RTFilterDemo_h
