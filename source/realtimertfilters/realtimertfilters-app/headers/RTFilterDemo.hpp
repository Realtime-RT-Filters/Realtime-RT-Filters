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

#include "ManagedUBO.hpp"

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

	using PFN_FilterDemoGui = void (RTFilterDemo::*)(vks::UIOverlay* overlay);

	class RTFilterDemo : public VulkanExampleBase
	{
	public:
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

#pragma region Scene/Shared UBO

		vkglTF::Model m_Scene{};

		int32_t m_enabledLightCount = 1;
		bool m_animateLights[UBO_SCENEINFO_LIGHT_COUNT]{};

		UBO_SceneInfo m_UBO_SceneInfo{};
		UBO_Guibase m_UBO_Guibase{};
		
		bool m_ShowSceneControls = false;
		bool m_ShowPathtracerControls = false;

#pragma endregion

		// One sampler for the frame buffer color attachments
		VkSampler m_DefaultColorSampler;

		RTFilterDemo();

		~RTFilterDemo();

		// Enable physical device features required for this example
		virtual void getEnabledFeatures();
		VkPhysicalDeviceAccelerationStructureFeaturesKHR* getEnabledFeaturesRayTracing();

		void enableExtensions(std::vector<const char*>& enabledDeviceExtensions);

		// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)
		void setupDefaultSampler();

		void loadAssets();

		// called by base class
		void buildCommandBuffers() override;

		virtual void prepare() override;

		virtual void render() override;

		virtual void setupUBOs();
		virtual void updateUBOs();

		virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay) override;
		virtual void ResetGUIState();
		virtual void SceneControlUIOverlay(vks::UIOverlay* overlay);
		virtual void PathtracerConfigUIOverlay(vks::UIOverlay* overlay);
		
		std::wstring getShadersPathW();

		bool gui_rp_on = false;

		VkPipelineShaderStageCreateInfo LoadShader(std::string shadername, VkShaderStageFlagBits stage);

		// Available features and properties
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
		VkPhysicalDeviceVulkan12Features vulkan12Features;

		// Enabled features and properties
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
		VkPhysicalDeviceVulkan12Features enabledPhysicalDeviceVulkan12Features{};

	};

}

#endif //RTFilterDemo_h
