#include "..\headers\PathTracerManager.hpp"
#include "..\headers\RTFilterDemo.hpp"
#include "..\data\shaders\glsl\pathTracerShader\binding.glsl"


void rtf::PathTracerManager::prepare(uint32_t width, uint32_t height)
{
	// Get properties and features
	rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProperties2{};
	deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperties2.pNext = &rayTracingPipelineProperties;
	vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);

	accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

	physicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	accelerationStructureFeatures.pNext = &physicalDeviceVulkan12Features;

	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.pNext = &accelerationStructureFeatures;
	vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);
	if (physicalDeviceVulkan12Features.runtimeDescriptorArray != VK_TRUE) {
		throw std::runtime_error("PathTracerManager::prepare: missing runtimeDescriptorArray feature");
	}
	if (physicalDeviceVulkan12Features.shaderSampledImageArrayNonUniformIndexing != VK_TRUE) {
		throw std::runtime_error("PathTracerManager::prepare: missing shaderSampledImageArrayNonUniformIndexing feature");
	}

	// Get the function pointers required for ray tracing
	vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
	vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
	vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
	vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
	vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
	vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
	vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
	vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
	vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));

	// Create the acceleration structures used to render the ray traced scene
	createBottomLevelAccelerationStructure();
	createTopLevelAccelerationStructure();

	createStorageImage(swapChain->colorFormat, { width, height, 1 });
	createUniformBuffer();
	createRayTracingPipeline();
	createShaderBindingTables();
	createDescriptorSets();
}

void rtf::PathTracerManager::createRayTracingPipeline() {
	std::vector<VkDescriptorSetLayoutBinding> layoutBindingSet = {
		// Acceleration structure
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR , B_ACCELERATIONSTRUCTURE),
		// Storage image
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR,B_IMAGE),
		//  Uniform buffer
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, B_UBO),
		// Vertex buffer 
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, B_VERTICES),
		// Index buffer
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, B_INDICES),
		//  Material
		//vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, B_MATERIALS),
		//  Textures
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, B_TEXTURES),
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(layoutBindingSet);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &rt_descriptorSetLayout));

	// Create Pipelinelayout
	VkPipelineLayoutCreateInfo pPipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(&rt_descriptorSetLayout, 1);

	//setup push constants
	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(PushConstant);
	push_constant.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

	pPipelineLayoutCI.pPushConstantRanges = &push_constant;
	pPipelineLayoutCI.pushConstantRangeCount = 1;

	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCI, nullptr, &rt_pipelineLayout));

	//Setup path tracing shader groups
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

void rtf::PathTracerManager::createDescriptorSets()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
	};
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &rt_descriptorSetLayout, 1);
	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &rt_descriptorSet));

	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = vks::initializers::writeDescriptorSetAccelerationStructureKHR();
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.handle;

	VkWriteDescriptorSet accelerationStructureWrite{};
	accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	// The specialized acceleration structure descriptor has to be chained
	accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
	accelerationStructureWrite.dstSet = rt_descriptorSet;
	accelerationStructureWrite.dstBinding = B_ACCELERATIONSTRUCTURE;
	accelerationStructureWrite.descriptorCount = 1;
	accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

	VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, storageImage.view, VK_IMAGE_LAYOUT_GENERAL };
	VkDescriptorBufferInfo vertexBufferDescriptor{ m_Scene->vertices.buffer, 0, VK_WHOLE_SIZE };
	VkDescriptorBufferInfo indexBufferDescriptor{ m_Scene->indices.buffer, 0, VK_WHOLE_SIZE };
	//VkDescriptorBufferInfo materialBufferDescriptor{ getMaterialBuffer(), 0, VK_WHOLE_SIZE };
	
	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		// Top level acceleration structure
		accelerationStructureWrite,
		// Ray tracing result image
		vks::initializers::writeDescriptorSet(rt_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, B_IMAGE, &storageImageDescriptor),
		// Uniform data
		vks::initializers::writeDescriptorSet(rt_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, B_UBO, &ubo.descriptor),
		// Scene vertex buffer
		vks::initializers::writeDescriptorSet(rt_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, B_VERTICES, &vertexBufferDescriptor),
		// Scene index buffer
		vks::initializers::writeDescriptorSet(rt_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, B_INDICES, &indexBufferDescriptor),
		// Material 
		//vks::initializers::writeDescriptorSet(rt_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, B_MATERIALS, &m_Scene->materials.data()->descriptor, m_Scene->materials.size()),
		// Textures
		vks::initializers::writeDescriptorSet(rt_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, B_TEXTURES, &m_Scene->textures.data()->descriptor,m_Scene->textures.size())
	};

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

void rtf::PathTracerManager::createUniformBuffer()
{
	RaytracingManager::createUniformBuffer();


}

void rtf::PathTracerManager::updateUniformBuffers(float timer, Camera* camera)
{
	RaytracingManager::updateUniformBuffers(timer, camera);
	this->m_pushConstants.frame++;
}

void rtf::PathTracerManager::cleanup() {
	RaytracingManager::cleanup();
}


void rtf::PathTracerManager::buildCommandBuffer(VkCommandBuffer commandBuffer, VkImage swapchainImage, uint32_t width, uint32_t height)
{
	/*
		Dispatch the ray tracing commands
	*/
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rt_pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rt_pipelineLayout, 0, 1, &rt_descriptorSet, 0, 0);

	//upload the matrix to the GPU via pushconstants
	vkCmdPushConstants(commandBuffer, rt_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &m_pushConstants);

	VkStridedDeviceAddressRegionKHR emptySbtEntry = {};
	vkCmdTraceRaysKHR(
		commandBuffer,
		&shaderBindingTables.raygen.stridedDeviceAddressRegion,
		&shaderBindingTables.miss.stridedDeviceAddressRegion,
		&shaderBindingTables.hit.stridedDeviceAddressRegion,
		&emptySbtEntry,
		width,
		height,
		1);

	/*
		Copy ray tracing output to swap chain image
	*/
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;

	// Prepare current swap chain image as transfer destination
	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vks::tools::setImageLayout(
		commandBuffer,
		swapchainImage,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	// Prepare ray tracing output image as transfer source
	vks::tools::setImageLayout(
		commandBuffer,
		storageImage.image,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		subresourceRange);

	VkImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { width, height, 1 };
	vkCmdCopyImage(commandBuffer, storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	// Transition swap chain image back for presentation
	vks::tools::setImageLayout(
		commandBuffer,
		swapchainImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		subresourceRange);

	// Transition ray tracing output image back to general layout
	vks::tools::setImageLayout(
		commandBuffer,
		storageImage.image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		subresourceRange);
}

