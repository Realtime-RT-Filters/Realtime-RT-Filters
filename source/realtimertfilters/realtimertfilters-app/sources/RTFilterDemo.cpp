#include "../headers/RTFilterDemo.hpp"
#include "../headers/VulkanglTFModel.h"
#include "../project_defines.hpp"
#include "../headers/PathTracerManager.hpp"
#include "../headers/SpirvCompiler.hpp"

#include "../headers/renderpasses/RenderpassManager.hpp"

#include "../headers/renderpasses/Renderpass_Gbuffer.hpp"
#include "../headers/renderpasses/Renderpass_Gui.hpp"

//All Filter render passes here
#include "../headers/renderpasses/Renderpass_Filter.hpp" //Example Renderpass
#include "../headers/renderpasses/Renderpass_PostProcess.hpp"

namespace rtf
{
#pragma region Constructor & Vulkan Instance Inits

	RTFilterDemo::RTFilterDemo() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "RT Filter Demo";
		camera.type = Camera::CameraType::firstperson;
		camera.movementSpeed = 5.0f;
#ifndef __ANDROID__
		camera.rotationSpeed = 0.25f;
#endif
		camera.position = { 2.15f, 0.3f, -8.75f };
		camera.setRotation(glm::vec3(-0.75f, 12.5f, 0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
		settings.overlay = true;

		// required for debugprintf in shaders
		enabledDeviceExtensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);


		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		enabledInstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		apiVersion = VK_API_VERSION_1_2;

		enableExtensions(enabledDeviceExtensions);

#ifdef _WIN32
		SpirvCompiler compiler(getShadersPathW(), getShadersPathW());
#else
		SpirvCompiler compiler(getShadersPath(), getShadersPath());
#endif
		compiler.CompileAll();

	}

	// Enable physical device features required for this example

	void RTFilterDemo::getEnabledFeatures()
	{
		// Enable anisotropic filtering if supported
		if (deviceFeatures.samplerAnisotropy)
		{
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		}
		if (deviceFeatures.fragmentStoresAndAtomics)
		{
			enabledFeatures.fragmentStoresAndAtomics = VK_TRUE;
		}
		if (deviceFeatures.vertexPipelineStoresAndAtomics)
		{
			enabledFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;
		}

		deviceCreatepNextChain = getEnabledFeaturesRayTracing();
	}

	VkPhysicalDeviceAccelerationStructureFeaturesKHR* RTFilterDemo::getEnabledFeaturesRayTracing()
	{
		// Get properties and features for Ray Tracer
		rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		rayTracingPipelineProperties.pNext = nullptr;
		VkPhysicalDeviceProperties2 deviceProperties2{};
		deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties2.pNext = &rayTracingPipelineProperties;
		vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);

		vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		vulkan12Features.pNext = nullptr;
		rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		rayTracingPipelineFeatures.pNext = &vulkan12Features;
		accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;

		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &accelerationStructureFeatures;
		vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

		if (vulkan12Features.runtimeDescriptorArray != VK_TRUE) {
			throw std::runtime_error("RTFilterDemo::getEnabledFeaturesRayTracing: missing runtimeDescriptorArray feature");
		}
		if (vulkan12Features.bufferDeviceAddress != VK_TRUE) {
			throw std::runtime_error("RTFilterDemo::getEnabledFeaturesRayTracing: missing bufferDeviceAddress feature");
		}
		if (vulkan12Features.descriptorIndexing != VK_TRUE) {
			throw std::runtime_error("RTFilterDemo::getEnabledFeaturesRayTracing: missing descriptorIndexing feature");
		}
		if (vulkan12Features.shaderSampledImageArrayNonUniformIndexing != VK_TRUE) {
			throw std::runtime_error("RTFilterDemo::getEnabledFeaturesRayTracing: missing shaderSampledImageArrayNonUniformIndexing feature");
		}

		// Enable features required for ray tracing using feature chaining via pNext		
		enabledPhysicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		enabledPhysicalDeviceVulkan12Features.runtimeDescriptorArray = VK_TRUE;
		enabledPhysicalDeviceVulkan12Features.bufferDeviceAddress = VK_TRUE;
		enabledPhysicalDeviceVulkan12Features.descriptorIndexing = VK_TRUE;
		enabledPhysicalDeviceVulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		enabledPhysicalDeviceVulkan12Features.pNext = nullptr;

		if (rayTracingPipelineFeatures.rayTracingPipeline != VK_TRUE) {
			throw std::runtime_error("RTFilterDemo::getEnabledFeaturesRayTracing: missing rayTracingPipeline feature");
		}

		enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
		enabledRayTracingPipelineFeatures.pNext = &enabledPhysicalDeviceVulkan12Features;

		if (accelerationStructureFeatures.accelerationStructure != VK_TRUE) {
			throw std::runtime_error("RTFilterDemo::getEnabledFeaturesRayTracing: missing accelerationStructure feature");
		}

		enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
		enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;

		return &enabledAccelerationStructureFeatures;
	}

	void RTFilterDemo::enableExtensions(std::vector<const char*>& enabledDeviceExtensions)
	{
		// Ray tracing related extensions required by this sample

		enabledDeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

		// Required by VK_KHR_acceleration_structure
		enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

		// Required for VK_KHR_ray_tracing_pipeline
		enabledDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

		// Required by VK_KHR_spirv_1_4
		enabledDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

	}

#pragma endregion
#pragma region Prepare

	void RTFilterDemo::prepare()
	{
		const size_t SEMAPHORE_COUNT = 5;

		VulkanExampleBase::prepare();

		std::cout << "loading assets.." << std::endl;
		loadAssets();
		std::cout << "done." << std::endl;

		setupDefaultSampler();

		//We create the Attachment manager
		m_attachmentManager = new Attachment_Manager(&device, vulkanDevice, &physicalDevice, width, height);

		m_renderpassManager = new RenderpassManager();
		m_renderpassManager->prepare(this, SEMAPHORE_COUNT);

		//Ray tracing
		/*m_rtManager.setup(this, physicalDevice, vulkanDevice, device, queue, &swapChain, descriptorPool, &camera);
		m_rtManager.prepare(width, height);

		m_pathTracerManager->setup(this, physicalDevice, vulkanDevice, device, queue, &swapChain, descriptorPool, &camera);
		m_pathTracerManager->prepare(width, height);*/
		//buildCommandBuffers();

		prepared = true;
	}

	void RTFilterDemo::setupDefaultSampler()
	{
		// Create sampler to sample from color attachments
		VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_NEAREST;
		sampler.minFilter = VK_FILTER_NEAREST;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 1.0f;
		sampler.minLod = 0.0f;
		sampler.maxLod = 1.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &m_DefaultColorSampler));
	}

	void RTFilterDemo::loadAssets()
	{

		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;

		// raytracing: we need to set additional buffer creation flags, before loading the scene:
		// Instead of a simple triangle, we'll be loading a more complex scene for this example
		// The shaders are accessing the vertex and index buffers of the scene, so the proper usage flag has to be set on the vertex and index buffers for the scene
		vkglTF::memoryPropertyFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		vkglTF::descriptorBindingFlags |= vkglTF::DescriptorBindingFlags::ImageNormalMap;

		//m_Scene.loadFromFile(getAssetPath() + "glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf", vulkanDevice, queue, glTFLoadingFlags);
		m_Scene.loadFromFile(getAssetPath() + MODEL_NAME, vulkanDevice, queue, glTFLoadingFlags);
		//m_Scene.loadFromFile(getAssetPath() + "models/armor/armor.gltf", vulkanDevice, queue, glTFLoadingFlags);
		//m_rtManager.setScene(&m_Scene);
		//m_pathTracerManager->setScene(&m_Scene);
	}

#pragma endregion
#pragma region Render

	void RTFilterDemo::buildCommandBuffers()
	{
		// ui overlay updated, rebuild gui command buffers
		m_renderpassManager->m_RPG_Active->buildCommandBuffer();

	}

	void RTFilterDemo::render()
	{
		if (!prepared)
			return;

		rt_on = debugDisplayTarget == 5;
		path_tracer_on = debugDisplayTarget == 6;

		VulkanExampleBase::prepareFrame();
		if (rt_on || path_tracer_on)
		{
			// command buffers are built differently for rt.
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
			VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		}
		// submit the renderpasses one after another
		m_renderpassManager->draw(drawCmdBuffers[currentBuffer]);
		VulkanExampleBase::submitFrame();

		m_renderpassManager->updateUniformBuffer();
		//m_rtManager.updateUniformBuffers(timer, &camera);
		//m_pathTracerManager->updateUniformBuffers(timer, &camera);
	}

	void RTFilterDemo::OnUpdateUIOverlay(vks::UIOverlay* overlay)
	{
		if (overlay->header("Settings"))
		{
			if (overlay->comboBox("Mode", &m_RenderMode, { "Rasterization Only", "Pathtracer Only", "SVGF", "BMFR" }))
			{
				m_renderpassManager->setQueueTemplate(static_cast<SupportedQueueTemplates>(m_RenderMode));
			}
			if (overlay->comboBox("Display", &debugDisplayTarget, m_renderpassManager->m_RPG_Active->getDropoutOptions()))
			{
				//Comp_UpdateUniformBuffer();
			}
		}
	}

#pragma endregion
#pragma region Cleanup

	RTFilterDemo::~RTFilterDemo()
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class

		vkDestroySampler(device, m_DefaultColorSampler, nullptr);

		// Frame buffer
		delete m_attachmentManager;

		// Destroy composition view components
		//vkDestroyPipeline(device, m_Comp_Pipeline, nullptr);
		//vkDestroyPipelineLayout(device, m_Comp_PipelineLayout, nullptr);
		//vkDestroyDescriptorSetLayout(device, m_Comp_DescriptorSetLayout, nullptr);
		//m_Comp_UnformBuffer.destroy();

		if (m_renderpassManager)
		{
			delete m_renderpassManager;
		}

		//Ray tracing destructors
		//m_rtManager.cleanup();
	}

#pragma endregion
#pragma region Helper Methods

	std::string RTFilterDemo::getShadersPath2()
	{
		return getShadersPath();
	}
	std::wstring RTFilterDemo::getShadersPathW()
	{
		return getAssetPathW() + L"shaders/glsl/";
	}
	VkPipelineShaderStageCreateInfo RTFilterDemo::LoadShader(std::string shadername, VkShaderStageFlagBits stage)
	{
		return loadShader(getShadersPath() + shadername, stage);
	}
#pragma endregion
}

