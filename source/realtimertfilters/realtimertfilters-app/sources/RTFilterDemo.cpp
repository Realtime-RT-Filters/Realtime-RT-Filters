#include "../headers/RTFilterDemo.hpp"
#include "../headers/VulkanglTFModel.h"
#include "../project_defines.hpp"
#include "../headers/SpirvCompiler.hpp"

#include "../headers/renderpasses/RenderpassManager.hpp"

#include "../headers/renderpasses/Renderpass_Gbuffer.hpp"
#include "../headers/renderpasses/Renderpass_Gui.hpp"

//All Filter render passes here
#include "../headers/renderpasses/Renderpass_PostProcess.hpp"
#include "../headers/renderpasses/Renderpass_PathTracer.hpp"

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

		if (vulkan12Features.runtimeDescriptorArray != VK_TRUE)
		{
			throw std::runtime_error("RTFilterDemo::getEnabledFeaturesRayTracing: missing runtimeDescriptorArray feature");
		}
		if (vulkan12Features.bufferDeviceAddress != VK_TRUE)
		{
			throw std::runtime_error("RTFilterDemo::getEnabledFeaturesRayTracing: missing bufferDeviceAddress feature");
		}
		if (vulkan12Features.descriptorIndexing != VK_TRUE)
		{
			throw std::runtime_error("RTFilterDemo::getEnabledFeaturesRayTracing: missing descriptorIndexing feature");
		}
		if (vulkan12Features.shaderSampledImageArrayNonUniformIndexing != VK_TRUE)
		{
			throw std::runtime_error("RTFilterDemo::getEnabledFeaturesRayTracing: missing shaderSampledImageArrayNonUniformIndexing feature");
		}

		// Enable features required for ray tracing using feature chaining via pNext		
		enabledPhysicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		enabledPhysicalDeviceVulkan12Features.runtimeDescriptorArray = VK_TRUE;
		enabledPhysicalDeviceVulkan12Features.bufferDeviceAddress = VK_TRUE;
		enabledPhysicalDeviceVulkan12Features.descriptorIndexing = VK_TRUE;
		enabledPhysicalDeviceVulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		enabledPhysicalDeviceVulkan12Features.separateDepthStencilLayouts = VK_TRUE;
		enabledPhysicalDeviceVulkan12Features.pNext = nullptr;

		if (rayTracingPipelineFeatures.rayTracingPipeline != VK_TRUE)
		{
			throw std::runtime_error("RTFilterDemo::getEnabledFeaturesRayTracing: missing rayTracingPipeline feature");
		}

		enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
		enabledRayTracingPipelineFeatures.pNext = &enabledPhysicalDeviceVulkan12Features;

		if (accelerationStructureFeatures.accelerationStructure != VK_TRUE)
		{
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
		VulkanExampleBase::prepare();

		std::cout << "loading assets.." << std::endl;
		loadAssets();
		std::cout << "done." << std::endl;

		setupDefaultSampler();

		//We create the Attachment manager
		m_attachmentManager = new Attachment_Manager(vulkanDevice, queue, width, height);

		setupUBOs();

		m_renderpassManager = new RenderpassManager();
		m_renderpassManager->prepare(this, m_semaphoreCount);

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
		updateUBOs();
		m_renderpassManager->updateUniformBuffer();

		VulkanExampleBase::prepareFrame();
		// submit the renderpasses one after another
		m_renderpassManager->draw(drawCmdBuffers[currentBuffer]);
		VulkanExampleBase::submitFrame();

		//m_rtManager.updateUniformBuffers(timer, &camera);
		//m_pathTracerManager->updateUniformBuffers(timer, &camera);
	}

	void RTFilterDemo::windowResized()
	{
		// update attachment manager width height
		m_attachmentManager->resize({width, height});
		// rebuild command buffer
		m_renderpassManager->m_RPG_Active->prepare();
		m_renderpassManager->m_RPG_Active->buildCommandBuffer();
		m_renderpassManager->prepare(this, m_semaphoreCount);
	}

	void RTFilterDemo::setupUBOs()
	{
		m_UBO_SceneInfo = std::make_shared<ManagedUBO<S_Sceneinfo>>(vulkanDevice);
		m_UBO_SceneInfo->prepare();

		S_Sceneinfo& sceneubo = m_UBO_SceneInfo->UBO();

		m_animateLights[0] = false;
		sceneubo.Lights[0].Color = glm::vec3(0.333f);
		sceneubo.Lights[0].RadiantFlux = 15.f;
		sceneubo.Lights[0].Position = glm::vec3(0.0f, -2.f, 5.f);

		for (int i = 1; i < UBO_SCENEINFO_LIGHT_COUNT; i++)
		{
			m_animateLights[i] = true;
			sceneubo.Lights[i].Color = glm::normalize(glm::vec3((i * 29) % 11, (i * 29) % 7, (i * 31) % 11));
			sceneubo.Lights[i].RadiantFlux = 15.f;
			sceneubo.Lights[i].Position = glm::vec3(0.f, i * -1.5f, 0.f);
			sceneubo.Lights[i].Type = (i < m_enabledLightCount) ? 1.0 : -1.0;
		}

		m_UBO_Guibase = std::make_shared<ManagedUBO<S_Guibase>>(vulkanDevice);
		m_UBO_Guibase->prepare();

		m_UBO_AccuConfig = std::make_shared<ManagedUBO<S_AccuConfig>>(vulkanDevice);
		m_UBO_AccuConfig->prepare();

		m_UBO_AtrousConfig = std::make_shared<ManagedUBO<S_AtrousConfig>>(vulkanDevice);
		m_UBO_AtrousConfig->prepare();

		m_UBO_BMFRConfig = std::make_shared<ManagedUBO<S_BMFRConfig>>(vulkanDevice);
		m_UBO_BMFRConfig->prepare();

		updateUBOs();
	}

	void RTFilterDemo::updateUBOs()
	{
		S_Sceneinfo& ubo = m_UBO_SceneInfo->UBO();
		for (int i = 0; i < m_enabledLightCount; i++)
		{
			S_Light& light = ubo.Lights[i];
			if (m_animateLights[i])
			{
				float offset = i * (UBO_SCENEINFO_LIGHT_COUNT / 360.f);
				ubo.Lights[i].Position.x = sin(glm::radians(360.0f * timer + offset)) *  (i % 4 + 1) * 2.0f;
				ubo.Lights[i].Position.z = cos(glm::radians(360.0f * timer + offset)) * ((i + 2) % 3 + 1) * 2.0f;
			}
		}

		// Current view position
		ubo.ViewPos = camera.position * glm::vec3(-1.0f, 1.0f, -1.0f);

		// Matrices
		ubo.ViewMatPrev = ubo.ViewMat;
		ubo.ProjMatPrev = ubo.ProjMat;
		ubo.ViewMat = camera.matrices.view;
		ubo.ProjMat = camera.matrices.perspective;
		ubo.ViewMatInverse = glm::inverse(camera.matrices.view);
		ubo.ProjMatInverse = glm::inverse(camera.matrices.perspective);

		m_UBO_Guibase->UBO().WindowHeight = height;
		m_UBO_Guibase->UBO().WindowWidth = width;

		m_UBO_SceneInfo->update();
		m_UBO_Guibase->update();
		m_UBO_AccuConfig->update();
		m_UBO_AtrousConfig->update();
		m_UBO_BMFRConfig->update();
	}

	void RTFilterDemo::OnUpdateUIOverlay(vks::UIOverlay* overlay)
	{
		S_Guibase& guiubo = m_UBO_Guibase->UBO();
		if (overlay->header("Display"))
		{
			if (overlay->comboBox("Mode", &m_RenderMode, { "Rasterization Only", "Pathtracer Only", "SVGF", "BMFR" }))
			{
				m_renderpassManager->setQueueTemplate(static_cast<SupportedQueueTemplates>(m_RenderMode));

				ResetGUIState();
			}
			overlay->sliderFloat("Splitview Factor", &guiubo.SplitViewFactor, 0.0f, 1.0f);
			bool doCompositionLeft = false;
			bool doCompositionRight = false;
			if (m_renderpassManager->m_RPG_Active->m_allowComposition)
			{
				doCompositionLeft = guiubo.ImageLeft == INT_MAX;
				overlay->checkBox("Left: Composition", &doCompositionLeft);
				guiubo.ImageLeft = doCompositionLeft ? INT_MAX : (guiubo.ImageLeft == INT_MAX ? 0 : guiubo.ImageLeft);
				doCompositionRight = guiubo.ImageRight == INT_MAX;
				overlay->checkBox("Right: Composition", &doCompositionRight);
				guiubo.ImageRight = doCompositionRight ? INT_MAX : (guiubo.ImageRight == INT_MAX ? 0 : guiubo.ImageRight);
			}
			std::vector<std::string>& dropoutoptions = m_renderpassManager->m_RPG_Active->getDropoutOptions();
			if (!doCompositionLeft)
			{
				overlay->comboBox("Left: Attachment", &guiubo.ImageLeft, dropoutoptions);
			}
			if (!doCompositionRight)
			{
				overlay->comboBox("Right: Attachment", &guiubo.ImageRight, dropoutoptions);
			}
		}
		SceneControlUIOverlay(overlay);
		PathtracerConfigUIOverlay(overlay);
		AccumulationConfigUIOverlay(overlay);
		AtrousConfigUIOverlay(overlay);
	}

	void RTFilterDemo::ResetGUIState()
	{
		S_Guibase& guiubo = m_UBO_Guibase->UBO();
		if (m_renderpassManager->m_RPG_Active->m_allowComposition)
		{
			guiubo.ImageLeft = INT_MAX;
			guiubo.ImageRight = INT_MAX;
		}
		else
		{
			guiubo.ImageLeft = 0;
			guiubo.ImageRight = 0;
		}
	}

	void RTFilterDemo::SceneControlUIOverlay(vks::UIOverlay* overlay)
	{
		S_Sceneinfo& ubo = m_UBO_SceneInfo->UBO();
		overlay->checkBox("Show Scene Controls", &m_ShowSceneControls);
		if (m_ShowSceneControls)
		{
			if (overlay->sliderInt("Light Sources", &m_enabledLightCount, 1, UBO_SCENEINFO_LIGHT_COUNT))
			{
				for (int i = 0; i < UBO_SCENEINFO_LIGHT_COUNT; i++)
				{
					if (i < m_enabledLightCount)
					{
						ubo.Lights[i].Type = abs(ubo.Lights[i].Type);
					}
					else
					{
						ubo.Lights[i].Type = abs(ubo.Lights[i].Type) * -1;
					}
				}
			}
			for (int i = 0; i < m_enabledLightCount; i++)
			{
				std::string temp = std::string("Light #") + std::to_string(i);
				if (overlay->header(temp.data()))
				{
					S_Light& light = ubo.Lights[i];
					int32_t type = static_cast<int32_t>(light.Type) - 1;
					if (overlay->comboBox("Type", &type, { "Point", "Directional" }))
					{
						light.Type = type + 1;
					}
					if (light.Type == 1.0) // Directional Light
					{
						overlay->inputFloat("Pos X", &light.Position.x, 0.2f, 1);
						overlay->inputFloat("Pos Y", &light.Position.y, 0.2f, 1);
						overlay->inputFloat("Pos Z", &light.Position.z, 0.2f, 1);
					}
					else
					{
						overlay->inputFloat("Dir X", &light.Position.x, 0.2f, 1);
						overlay->inputFloat("Dir Y", &light.Position.y, 0.2f, 1);
						overlay->inputFloat("Dir Z", &light.Position.z, 0.2f, 1);
						light.Position = glm::normalize(light.Position);
					}
					overlay->inputFloat("Color R", &light.Color.r, 0.2f, 1);
					overlay->inputFloat("Color G", &light.Color.g, 0.2f, 1);
					overlay->inputFloat("Color B", &light.Color.b, 0.2f, 1);
					light.Color = glm::normalize(light.Color);
					overlay->inputFloat("Power", &light.RadiantFlux, 0.05f, 1);
					overlay->checkBox("Animate", &m_animateLights[i]);
				}
			}
		}
	}

	void RTFilterDemo::PathtracerConfigUIOverlay(vks::UIOverlay* overlay)
	{
		if (!m_renderpassManager->m_RPG_Active->m_usePathtracing)
		{
			return;
		}
		overlay->checkBox("Show Pathtracer Controls", &m_ShowPathtracerControls);
		if (m_ShowPathtracerControls)
		{
			SPC_PathtracerConfig& pathtracerConfig = m_renderpassManager->m_RP_PT->m_pathtracerconfig;
			int32_t samplesPerPixel = static_cast<int32_t>(pathtracerConfig.PrimarySamplesPerPixel);
			int32_t bounceDepth = static_cast<int32_t>(pathtracerConfig.MaxBounceDepth);
			int32_t samplesPerBounce = static_cast<int32_t>(pathtracerConfig.SecondarySamplesPerBounce);
			overlay->sliderInt("Samples per Pixel", &samplesPerPixel, 1, 16);
			overlay->sliderInt("Bounce Depth", &bounceDepth, 1, 6);
			overlay->sliderInt("Samples per Bounce", &samplesPerBounce, 1, 8);
			pathtracerConfig.PrimarySamplesPerPixel = static_cast<uint>(samplesPerPixel);
			pathtracerConfig.MaxBounceDepth = static_cast<uint>(bounceDepth);
			pathtracerConfig.SecondarySamplesPerBounce = static_cast<uint>(samplesPerBounce);
		}
	}

	void RTFilterDemo::AccumulationConfigUIOverlay(vks::UIOverlay* overlay)
	{
		if (!m_renderpassManager->m_RPG_Active->m_useTempAccu)
		{
			return;
		}
		S_AccuConfig& ubo = m_UBO_AccuConfig->UBO();
		overlay->checkBox("Enable TempAccu", &ubo.EnableAccumulation);
		if (ubo.EnableAccumulation)
		{
			float maxPosDiff = sqrt(ubo.MaxPosDifference);
			overlay->sliderFloat("Max PosDiff Squared", &maxPosDiff, 0.f, 1.f);
			ubo.MaxPosDifference = maxPosDiff * maxPosDiff;
			overlay->sliderFloat("Max Normal Deviation", &ubo.MaxNormalAngleDifference, 0.f, 3.141f);
			overlay->sliderFloat("Min new data weight", &ubo.MinNewWeight, 0.f, 1.f);
		}
	}

	void RTFilterDemo::AtrousConfigUIOverlay(vks::UIOverlay* overlay)
	{
		if (!m_renderpassManager->m_RPG_Active->m_useAtrous)
		{
			return;
		}
		S_AtrousConfig& ubo = m_UBO_AtrousConfig->UBO();
		overlay->sliderInt("Iterations", &ubo.iterations, 0, 9);
		if (ubo.iterations > 0)
		{
			overlay->sliderFloat("Normal Phi", &ubo.n_phi, 0.f, 0.5);
			ubo.n_phi = ubo.n_phi < 0.0001 ? 0.0001 : ubo.n_phi;
			overlay->sliderFloat("Depth Phi", &ubo.p_phi, 0.f, 0.5);
			ubo.p_phi = ubo.p_phi < 0.0001 ? 0.0001 : ubo.p_phi;
			overlay->sliderFloat("Color Phi", &ubo.c_phi, 0.f, 0.5);
			ubo.c_phi = ubo.c_phi < 0.0001 ? 0.0001 : ubo.c_phi;
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

