#include "..\headers\PathTracerManager.hpp"
#include "..\headers\RTFilterDemo.hpp"



void rtf::PathTracerManager::createRayTracingPipeline() {
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		// Binding 0: Acceleration structure
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0),
		// Binding 1: Storage image
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1),
		// Binding 2: Uniform buffer
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 2),
		// Binding 3: Vertex buffer 
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 3),
		// Binding 4: Index buffer
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 4),
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &rt_descriptorSetLayout));

	VkPipelineLayoutCreateInfo pPipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(&rt_descriptorSetLayout, 1);
	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCI, nullptr, &rt_pipelineLayout));

	//Setup ray tracing shader groups
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	// Ray generation group
	{
		shaderStages.push_back(m_rtFilterDemo->loadShader(m_rtFilterDemo->getShadersPath2() + "pathTracerShader/raygen.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}

	// Miss group
	{
		shaderStages.push_back(m_rtFilterDemo->loadShader(m_rtFilterDemo->getShadersPath2() + "pathTracerShader/miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
		// Second shader for shadows
		shaderStages.push_back(m_rtFilterDemo->loadShader(m_rtFilterDemo->getShadersPath2() + "pathTracerShader/shadow.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR));
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroups.push_back(shaderGroup);
	}

	// Closest hit group
	{
		shaderStages.push_back(m_rtFilterDemo->loadShader(m_rtFilterDemo->getShadersPath2() + "pathTracerShader/closesthit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}

	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI = vks::initializers::rayTracingPipelineCreateInfoKHR();
	rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	rayTracingPipelineCI.pStages = shaderStages.data();
	rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
	rayTracingPipelineCI.pGroups = shaderGroups.data();
	rayTracingPipelineCI.maxPipelineRayRecursionDepth = 2;
	rayTracingPipelineCI.layout = rt_pipelineLayout;
	VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &rt_pipeline));
	
}