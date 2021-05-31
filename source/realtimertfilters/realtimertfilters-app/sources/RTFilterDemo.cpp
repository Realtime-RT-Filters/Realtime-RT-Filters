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

		m_rtManager.enableExtensions(enabledDeviceExtensions);

		m_pathTracerManager = new PathTracerManager();
		m_pathTracerManager->enableExtensions(enabledDeviceExtensions);

#ifdef _WIN32
		SpirvCompiler compiler(getShadersPathW(), getShadersPathW());
#else
		SpirvCompiler compiler(getShadersPath(), getShadersPath());
#endif
		compiler.CompileAll();

	}
	RTFilterDemo::~RTFilterDemo()
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class

		vkDestroySampler(device, m_DefaultColorSampler, nullptr);

		// Frame buffer
		delete m_attachmentManager;

		// Destroy composition view components
		vkDestroyPipeline(device, m_Comp_Pipeline, nullptr);
		vkDestroyPipelineLayout(device, m_Comp_PipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, m_Comp_DescriptorSetLayout, nullptr);
		m_Comp_UnformBuffer.destroy();

		if (m_renderpassManager)
		{
			delete m_renderpassManager;
		}

		//Ray tracing destructors
		m_rtManager.cleanup();
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
		
		deviceCreatepNextChain = m_pathTracerManager->getEnabledFeatures();
	}


	// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)

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
		m_rtManager.setScene(&m_Scene);
		m_pathTracerManager->setScene(&m_Scene);
	}
	void RTFilterDemo::buildCommandBuffers()
	{
		//when ray tracing is turned on, build different command buffer
		if (rt_on)
		{
			if (resized)
			{
				m_rtManager.handleResize(width, height);
			}

			VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
			for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
			{
				VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
				m_rtManager.buildCommandBuffer(drawCmdBuffers[i], swapChain.images[i], width, height);
				drawUI(drawCmdBuffers[i]);
				VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
			}
		}
		else if (path_tracer_on) {
			if (resized)
			{
				m_pathTracerManager->handleResize(width, height);
			}

			VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
			for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
			{
				VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
				m_pathTracerManager->buildCommandBuffer(drawCmdBuffers[i], swapChain.images[i], width, height);
				drawUI(drawCmdBuffers[i]);
				VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
			}
		}
		else
		{

			if (gui_rp_on) {
				

			}
			else {

				VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

				VkClearValue clearValues[2];
				clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };
				clearValues[1].depthStencil = { 1.0f, 0 };

				VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
				renderPassBeginInfo.renderPass = renderPass;
				renderPassBeginInfo.renderArea.offset.x = 0;
				renderPassBeginInfo.renderArea.offset.y = 0;
				renderPassBeginInfo.renderArea.extent.width = width;
				renderPassBeginInfo.renderArea.extent.height = height;
				renderPassBeginInfo.clearValueCount = 2;
				renderPassBeginInfo.pClearValues = clearValues;

				for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
				{
					renderPassBeginInfo.framebuffer = frameBuffers[i];

					VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

					vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

					VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
					vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

					VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
					vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

					vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Comp_PipelineLayout, 0, 1, &m_Comp_DescriptorSet, 0, nullptr);

					vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Comp_Pipeline);
					// Final composition as full screen quad
					// Note: Also used for debug display if debugDisplayTarget > 0
					vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

					drawUI(drawCmdBuffers[i]);

					vkCmdEndRenderPass(drawCmdBuffers[i]);

					VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
				}

			}
		}
	}
	void RTFilterDemo::Comp_SetupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 9)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 3);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}
	void RTFilterDemo::Comp_SetupDescriptorSetLayout()
	{
		// Deferred shading layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			// Binding 1 : Position texture target / Scene colormap
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			// Binding 2 : Normals texture target
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
			// Binding 3 : Albedo texture target
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
			// Binding 4 : Fragment shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
			// Binding 3 : Albedo texture target
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &m_Comp_DescriptorSetLayout));

		// Shared pipeline layout used by composition
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&m_Comp_DescriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &m_Comp_PipelineLayout));
	}

	void RTFilterDemo::Comp_SetupDescriptorSet()
	{

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &m_Comp_DescriptorSetLayout, 1);

		// Image descriptors for the offscreen color attachments
		VkDescriptorImageInfo texDescriptorPosition =
			vks::initializers::descriptorImageInfo(
				m_DefaultColorSampler,
				m_attachmentManager->getAttachment(Attachment::position)->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo texDescriptorNormal =
			vks::initializers::descriptorImageInfo(
				m_DefaultColorSampler,
				m_attachmentManager->getAttachment(Attachment::normal)->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo texDescriptorAlbedo =
			vks::initializers::descriptorImageInfo(
				m_DefaultColorSampler,
				m_attachmentManager->getAttachment(Attachment::albedo)->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo texDescriptorMotion =
			vks::initializers::descriptorImageInfo(
				m_DefaultColorSampler,
				m_attachmentManager->getAttachment(Attachment::motionvector)->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// Deferred composition
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &m_Comp_DescriptorSet));
		writeDescriptorSets = {
			// Binding 1 : Position texture target
			vks::initializers::writeDescriptorSet(m_Comp_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &texDescriptorPosition),
			// Binding 2 : Normals texture target
			vks::initializers::writeDescriptorSet(m_Comp_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &texDescriptorNormal),
			// Binding 3 : Albedo texture target
			vks::initializers::writeDescriptorSet(m_Comp_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &texDescriptorAlbedo),
			// Binding 4 : Fragment shader uniform buffer
			vks::initializers::writeDescriptorSet(m_Comp_DescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, &m_Comp_UnformBuffer.descriptor),
			// Binding 5 : motion buffer vector
			vks::initializers::writeDescriptorSet(m_Comp_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &texDescriptorMotion),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}
	void RTFilterDemo::Comp_PreparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(m_Comp_PipelineLayout, renderPass);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		// Final fullscreen composition pass pipeline
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		shaderStages[0] = loadShader(getShadersPath() + "deferred/deferred.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "deferred/deferred.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		// Empty vertex input state, vertices are generated by the vertex shader
		VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		pipelineCI.pVertexInputState = &emptyInputState;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &m_Comp_Pipeline));
	}

	// Prepare and initialize uniform buffer containing shader uniforms

	void RTFilterDemo::Comp_PrepareUniformBuffers()
	{

		// Deferred fragment shader
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_Comp_UnformBuffer,
			sizeof(m_Comp_UBO)));

		// Map persistent
		//VK_CHECK_RESULT(uniformBuffers.offscreen.map());
		VK_CHECK_RESULT(m_Comp_UnformBuffer.map());

		Comp_UpdateUniformBuffer();
	}

	// Update lights and parameters passed to the composition shaders

	void RTFilterDemo::Comp_UpdateUniformBuffer()
	{
		// White
		m_Comp_UBO.lights[0].position = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		m_Comp_UBO.lights[0].color = glm::vec3(1.5f);
		m_Comp_UBO.lights[0].radius = 15.0f * 0.25f;
		// Red
		m_Comp_UBO.lights[1].position = glm::vec4(-2.0f, 0.0f, 0.0f, 0.0f);
		m_Comp_UBO.lights[1].color = glm::vec3(1.0f, 0.0f, 0.0f);
		m_Comp_UBO.lights[1].radius = 15.0f;
		// Blue
		m_Comp_UBO.lights[2].position = glm::vec4(2.0f, -1.0f, 0.0f, 0.0f);
		m_Comp_UBO.lights[2].color = glm::vec3(0.0f, 0.0f, 2.5f);
		m_Comp_UBO.lights[2].radius = 5.0f;
		// Yellow
		m_Comp_UBO.lights[3].position = glm::vec4(0.0f, -0.9f, 0.5f, 0.0f);
		m_Comp_UBO.lights[3].color = glm::vec3(1.0f, 1.0f, 0.0f);
		m_Comp_UBO.lights[3].radius = 2.0f;
		// Green
		m_Comp_UBO.lights[4].position = glm::vec4(0.0f, -0.5f, 0.0f, 0.0f);
		m_Comp_UBO.lights[4].color = glm::vec3(0.0f, 1.0f, 0.2f);
		m_Comp_UBO.lights[4].radius = 5.0f;
		// Yellow
		m_Comp_UBO.lights[5].position = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
		m_Comp_UBO.lights[5].color = glm::vec3(1.0f, 0.7f, 0.3f);
		m_Comp_UBO.lights[5].radius = 25.0f;

		m_Comp_UBO.lights[0].position.x = sin(glm::radians(360.0f * timer)) * 5.0f;
		m_Comp_UBO.lights[0].position.z = cos(glm::radians(360.0f * timer)) * 5.0f;

		m_Comp_UBO.lights[1].position.x = -4.0f + sin(glm::radians(360.0f * timer) + 45.0f) * 2.0f;
		m_Comp_UBO.lights[1].position.z = 0.0f + cos(glm::radians(360.0f * timer) + 45.0f) * 2.0f;

		m_Comp_UBO.lights[2].position.x = 4.0f + sin(glm::radians(360.0f * timer)) * 2.0f;
		m_Comp_UBO.lights[2].position.z = 0.0f + cos(glm::radians(360.0f * timer)) * 2.0f;

		m_Comp_UBO.lights[4].position.x = 0.0f + sin(glm::radians(360.0f * timer + 90.0f)) * 5.0f;
		m_Comp_UBO.lights[4].position.z = 0.0f - cos(glm::radians(360.0f * timer + 45.0f)) * 5.0f;

		m_Comp_UBO.lights[5].position.x = 0.0f + sin(glm::radians(-360.0f * timer + 135.0f)) * 10.0f;
		m_Comp_UBO.lights[5].position.z = 0.0f - cos(glm::radians(-360.0f * timer - 45.0f)) * 10.0f;

		// Current view position
		m_Comp_UBO.viewPos = glm::vec4(camera.position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

		m_Comp_UBO.debugDisplayTarget = debugDisplayTarget;

		memcpy(m_Comp_UnformBuffer.mapped, &m_Comp_UBO, sizeof(m_Comp_UBO));
	}
	void RTFilterDemo::draw()
	{
		VulkanExampleBase::prepareFrame();
		// submit the renderpasses one after another
		m_renderpassManager->draw(drawCmdBuffers[currentBuffer]);
		VulkanExampleBase::submitFrame();
	}
	void RTFilterDemo::prepare()
	{
		VulkanExampleBase::prepare();

		std::cout << "loading assets.." << std::endl;
		loadAssets();
		std::cout << "done." << std::endl;

		//We create the Attachment manager
		m_attachmentManager = new Attachment_Manager(&device, vulkanDevice, &physicalDevice, width, height);

		m_renderpassManager = new RenderpassManager();
		m_renderpassManager->addRenderpass(std::make_shared<RenderpassGbuffer>());

		auto gbufferPass = std::make_shared<RenderpassPostProcess>();
		gbufferPass->ConfigureShader("filter/postprocess_gauss.frag.spv");
		gbufferPass->PushAttachment(m_attachmentManager->getAttachment(Attachment::albedo), RenderpassPostProcess::AttachmentUse::ReadOnly);
		gbufferPass->PushAttachment(m_attachmentManager->getAttachment(Attachment::rtoutput), RenderpassPostProcess::AttachmentUse::WriteOnly);
		m_renderpassManager->addRenderpass(gbufferPass);

		m_renderpassManager->prepare(this);

		setupDefaultSampler();
		Comp_PrepareUniformBuffers();
		Comp_SetupDescriptorSetLayout();
		Comp_PreparePipelines();
		Comp_SetupDescriptorPool();
		Comp_SetupDescriptorSet();

		//Ray tracing
		m_rtManager.setup(this, physicalDevice, vulkanDevice, device, queue, &swapChain, descriptorPool, &camera);
		m_rtManager.prepare(width, height);

		m_pathTracerManager->setup(this, physicalDevice, vulkanDevice, device, queue, &swapChain, descriptorPool, &camera);
		m_pathTracerManager->prepare(width, height);
		buildCommandBuffers();

		//GUI Renderpass
		//m_renderpass_gui = new Renderpass_Gui(instance, vulkanDevice, m_attachmentManager, this, &swapChain, &timer, &debugDisplayTarget, &camera);
		//m_renderpass_gui->prepare();


		prepared = true;
	}

	void RTFilterDemo::render()
	{
		if (!prepared)
			return;

		rt_on = debugDisplayTarget == 5;
		path_tracer_on = debugDisplayTarget == 6;

		if (rt_on)
		{
			// command buffers are built differently for rt.
			VulkanExampleBase::prepareFrame();
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
			VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
			VulkanExampleBase::submitFrame();
		}else if (path_tracer_on)
		{
			// command buffers are built differently for pt.
			VulkanExampleBase::prepareFrame();
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
			VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
			VulkanExampleBase::submitFrame();
		}
		else
		{
			draw();
		}


		if (!paused)
		{
			Comp_UpdateUniformBuffer();
		}
		if (camera.updated)
		{
			Comp_UpdateUniformBuffer();
		}

		m_rtManager.updateUniformBuffers(timer, &camera);
		m_pathTracerManager->updateUniformBuffers(timer, &camera);
	}
	void RTFilterDemo::OnUpdateUIOverlay(vks::UIOverlay* overlay)
	{
		if (overlay->header("Settings"))
		{
			if (overlay->comboBox("Display", &debugDisplayTarget, { "Final composition", "Position", "Normals", "Albedo", "Specular", "Ray Tracing", "Path Tracing", "Motion vectors" }))
			{
				Comp_UpdateUniformBuffer();
			}
		}
	}

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
}

