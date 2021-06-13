#include "../../headers/renderpasses/Renderpass_PathTracer.hpp"
#include "../../headers/RTFilterDemo.hpp"
#include "../../data/shaders/glsl/pathTracerShader/binding.glsl"
#include "../../data/shaders/glsl/pathTracerShader/gltf.glsl"

namespace rtf
{
	void RenderpassPathTracer::prepare()
	{
		// Init push Constant
		initData();
		prepareAttachement();
		createMaterialBuffer();

		// Get the function pointers required for ray tracing
		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(m_vulkanDevice->logicalDevice, "vkGetBufferDeviceAddressKHR"));
		vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_vulkanDevice->logicalDevice, "vkCmdBuildAccelerationStructuresKHR"));
		vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_vulkanDevice->logicalDevice, "vkBuildAccelerationStructuresKHR"));
		vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_vulkanDevice->logicalDevice, "vkCreateAccelerationStructureKHR"));
		vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_vulkanDevice->logicalDevice, "vkDestroyAccelerationStructureKHR"));
		vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_vulkanDevice->logicalDevice, "vkGetAccelerationStructureBuildSizesKHR"));
		vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(m_vulkanDevice->logicalDevice, "vkGetAccelerationStructureDeviceAddressKHR"));
		vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_vulkanDevice->logicalDevice, "vkCmdTraceRaysKHR"));
		vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(m_vulkanDevice->logicalDevice, "vkGetRayTracingShaderGroupHandlesKHR"));
		vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_vulkanDevice->logicalDevice, "vkCreateRayTracingPipelinesKHR"));

		// Create the acceleration structures used to render the ray traced scene
		createBottomLevelAccelerationStructure();
		createTopLevelAccelerationStructure();

		//createStorageImage(m_swapChain->colorFormat, VkExtent3D{ m_rtFilterDemo->width, m_rtFilterDemo->height, 1 });
		//createUniformBuffer();
		createRayTracingPipeline();
		createShaderBindingTables();
		createDescriptorSets();
		buildCommandBuffer();
	}

	void RenderpassPathTracer::draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) {
		updatePushConstants();
		buildCommandBuffer();
		out_commandBufferCount = 1;
		out_commandBuffers = &m_commandBuffer;
	}

	void RenderpassPathTracer::cleanUp() {
		deleteStorageImage();
		deleteAccelerationStructure(m_bottomLevelAS);
		deleteAccelerationStructure(m_topLevelAS);
		m_shaderBindingTables.raygen.destroy();
		m_shaderBindingTables.miss.destroy();
		m_shaderBindingTables.hit.destroy();

		vkDestroyPipeline(m_vulkanDevice->logicalDevice, m_pipeline, nullptr);
		vkDestroyPipelineLayout(m_vulkanDevice->logicalDevice, m_pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_vulkanDevice->logicalDevice, m_descriptorSetLayout, nullptr);
		//m_uniformBufferObject.destroy();
	};

	void RenderpassPathTracer::updateUniformBuffer() {
		//m_uniformData.projInverse = glm::inverse(m_camera->matrices.perspective);
		//m_uniformData.viewInverse = glm::inverse(m_camera->matrices.view);
		//m_uniformData.lightPos = glm::vec4(cos(glm::radians(m_timer * 360.0f)) * 40.0f, -50.0f + sin(glm::radians(m_timer * 360.0f)) * 20.0f, 25.0f + sin(glm::radians(m_timer * 360.0f)) * 5.0f, 0.0f);
		//// Pass the vertex size to the shader for unpacking vertices
		//m_uniformData.vertexSize = sizeof(vkglTF::Vertex);
		//memcpy(m_uniformBufferObject.mapped, &m_uniformData, sizeof(UniformData));
	};

	// Define Class Methods ================================================================================================

	void RenderpassPathTracer::deleteAccelerationStructure(AccelerationStructure& accelerationStructure)
	{
		vkFreeMemory(m_vulkanDevice->logicalDevice, accelerationStructure.memory, nullptr);
		vkDestroyBuffer(m_vulkanDevice->logicalDevice, accelerationStructure.buffer, nullptr);
		vkDestroyAccelerationStructureKHR(m_vulkanDevice->logicalDevice, accelerationStructure.handle, nullptr);
	}

	void RenderpassPathTracer::updatePushConstants()
	{
		uint32_t frameNumber = m_rtFilterDemo->frameCounter;
		//m_pushConstant.clearColor;
		//m_pushConstant.lightIntensity;
		//m_pushConstant.lightType;
		m_pathtracerconfig.Frame++;
		//m_pushConstant.samples;
		//m_pushConstant.bounces;
		//m_pushConstant.bounceSamples;
		m_pathtracerconfig.VertexSize = sizeof(vkglTF::Vertex);
	}

	void RenderpassPathTracer::createRayTracingPipeline() {
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
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, B_MATERIALS),
			//  Textures
			//vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, B_TEXTURES),
		};

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(layoutBindingSet);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_vulkanDevice->logicalDevice, &descriptorSetLayoutCI, nullptr, &m_descriptorSetLayout));

		// Create Pipelinelayout
		VkPipelineLayoutCreateInfo pPipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(&m_descriptorSetLayout, 1);

		//setup push constants
		VkPushConstantRange push_constant;
		push_constant.offset = 0;
		push_constant.size = sizeof(SPC_PathtracerConfig);
		push_constant.stageFlags = VK_SHADER_STAGE_ALL;

		pPipelineLayoutCI.pPushConstantRanges = &push_constant;
		pPipelineLayoutCI.pushConstantRangeCount = 1;

		VK_CHECK_RESULT(vkCreatePipelineLayout(m_vulkanDevice->logicalDevice, &pPipelineLayoutCI, nullptr, &m_pipelineLayout));

		//Setup path tracing shader groups
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

		// Ray generation group
		{
			shaderStages.push_back(m_rtFilterDemo->loadShader(m_rtFilterDemo->getShadersPath() + "pathTracerShader/raygen.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR));
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
			shaderStages.push_back(m_rtFilterDemo->loadShader(m_rtFilterDemo->getShadersPath() + "pathTracerShader/miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR));
			VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			shaderGroups.push_back(shaderGroup);
			// Second shader for shadows
			shaderStages.push_back(m_rtFilterDemo->loadShader(m_rtFilterDemo->getShadersPath() + "pathTracerShader/shadow.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR));
			shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
			shaderGroups.push_back(shaderGroup);
		}

		// Closest hit group
		{
			shaderStages.push_back(m_rtFilterDemo->loadShader(m_rtFilterDemo->getShadersPath() + "pathTracerShader/closesthit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
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
		rayTracingPipelineCI.layout = m_pipelineLayout;
		VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(m_vulkanDevice->logicalDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &m_pipeline));
	}

	void RenderpassPathTracer::createDescriptorSets()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 }
			//{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0 }
		};
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
		VK_CHECK_RESULT(vkCreateDescriptorPool(m_vulkanDevice->logicalDevice, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool));

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = vks::initializers::descriptorSetAllocateInfo(m_descriptorPool, &m_descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(m_vulkanDevice->logicalDevice, &descriptorSetAllocateInfo, &m_descriptorSet));

		VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = vks::initializers::writeDescriptorSetAccelerationStructureKHR();
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = &m_topLevelAS.handle;

		VkWriteDescriptorSet accelerationStructureWrite{};
		accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// The specialized acceleration structure descriptor has to be chained
		accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
		accelerationStructureWrite.dstSet = m_descriptorSet;
		accelerationStructureWrite.dstBinding = B_ACCELERATIONSTRUCTURE;
		accelerationStructureWrite.descriptorCount = 1;
		accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

		VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, m_Rtoutput->view, VK_IMAGE_LAYOUT_GENERAL };
		VkDescriptorBufferInfo vertexBufferDescriptor{ m_Scene->vertices.buffer, 0, VK_WHOLE_SIZE };
		VkDescriptorBufferInfo indexBufferDescriptor{ m_Scene->indices.buffer, 0, VK_WHOLE_SIZE };
		VkDescriptorBufferInfo materialBufferDescriptor{ m_material_buffer.buffer, 0, VK_WHOLE_SIZE };

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			// Top level acceleration structure
			accelerationStructureWrite,
			// Ray tracing result image
			vks::initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, B_IMAGE, &storageImageDescriptor),
			// Uniform data
			m_rtFilterDemo->m_UBO_SceneInfo->writeDescriptorSet(m_descriptorSet, B_UBO),
			//vks::initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, B_UBO, &m_uniformBufferObject.descriptor),
			// Scene vertex buffer
			vks::initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, B_VERTICES, &vertexBufferDescriptor),
			// Scene index buffer
			vks::initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, B_INDICES, &indexBufferDescriptor),
			// Material buffer
			vks::initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, B_MATERIALS, &materialBufferDescriptor),
			// Textures
			//vks::initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, B_TEXTURES, &m_Scene->textures.data()->descriptor)
		};
		vkUpdateDescriptorSets(m_vulkanDevice->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
	}

	void RenderpassPathTracer::createMaterialBuffer()
	{
		std::vector<vkglTF::Material> materials = m_Scene->materials;
		this->m_materials.resize(materials.size());
		for (int i = 0; i < materials.size(); i++) {
			m_materials[i].baseColorFactor = materials[i].baseColorFactor;
			m_materials[i].emissiveFactor = materials[i].emissiveFactor;
			//m_materials[i].baseColorTexture = materials[i].baseColorTexture;

		}

		VK_CHECK_RESULT(m_vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_material_buffer,
			sizeof(GltfShadeMaterial) * this->m_materials.size(),
			m_materials.data())
		);
	}

	void RenderpassPathTracer::createShaderBindingTables()
	{
		const uint32_t handleSize = m_rtFilterDemo->rayTracingPipelineProperties.shaderGroupHandleSize;
		const uint32_t handleSizeAligned = vks::tools::alignedSize(m_rtFilterDemo->rayTracingPipelineProperties.shaderGroupHandleSize, m_rtFilterDemo->rayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
		const uint32_t sbtSize = groupCount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);
		VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesKHR(m_vulkanDevice->logicalDevice, m_pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

		createShaderBindingTable(m_shaderBindingTables.raygen, 1);
		// We are using two miss shaders
		createShaderBindingTable(m_shaderBindingTables.miss, 2);
		createShaderBindingTable(m_shaderBindingTables.hit, 1);

		// Copy handles
		memcpy(m_shaderBindingTables.raygen.mapped, shaderHandleStorage.data(), handleSize);
		// We are using two miss shaders, so we need to get two handles for the miss shader binding table
		memcpy(m_shaderBindingTables.miss.mapped, shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
		memcpy(m_shaderBindingTables.hit.mapped, shaderHandleStorage.data() + handleSizeAligned * 3, handleSize);

	}

	void RenderpassPathTracer::createShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount)
	{
		// Create buffer to hold all shader handles for the SBT
		VK_CHECK_RESULT(m_vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&shaderBindingTable,
			m_rtFilterDemo->rayTracingPipelineProperties.shaderGroupHandleSize * handleCount));
		// Get the strided address to be used when dispatching the rays
		shaderBindingTable.stridedDeviceAddressRegion = getSbtEntryStridedDeviceAddressRegion(shaderBindingTable.buffer, handleCount);
		// Map persistent 
		shaderBindingTable.map();
	}

	VkStridedDeviceAddressRegionKHR RenderpassPathTracer::getSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount)
	{
		const uint32_t handleSizeAligned = vks::tools::alignedSize(m_rtFilterDemo->rayTracingPipelineProperties.shaderGroupHandleSize, m_rtFilterDemo->rayTracingPipelineProperties.shaderGroupHandleAlignment);
		VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{};
		stridedDeviceAddressRegionKHR.deviceAddress = getBufferDeviceAddress(buffer);
		stridedDeviceAddressRegionKHR.stride = handleSizeAligned;
		stridedDeviceAddressRegionKHR.size = handleCount * handleSizeAligned;
		return stridedDeviceAddressRegionKHR;
	}

	void RenderpassPathTracer::initData()
	{
		// Init members;
		m_Scene = &m_rtFilterDemo->m_Scene;
		m_camera = &m_rtFilterDemo->camera;
		m_queue = m_rtFilterDemo->queue;
		m_timer = *&m_rtFilterDemo->timer;

		// Init Push Constant
		m_pathtracerconfig = SPC_PathtracerConfig{};
	}

	void RenderpassPathTracer::prepareAttachement() {
		//m_position, * m_normal, * m_albedo, * m_motionvector, * m_rtoutput, * m_filteroutput;
		m_PositionAttachment = m_attachmentManager->getAttachment(Attachment::position);
		m_NormalAttachment = m_attachmentManager->getAttachment(Attachment::normal);
		m_AlbedoAttachment = m_attachmentManager->getAttachment(Attachment::albedo);
		m_MotionAttachment = m_attachmentManager->getAttachment(Attachment::motionvector);
		m_Rtoutput = m_attachmentManager->getAttachment(Attachment::rtoutput);
		m_Filteroutput = m_attachmentManager->getAttachment(Attachment::filteroutput);
	}

	/*
		Create the bottom level acceleration structure contains the scene's actual geometry (vertices, triangles)
	*/
	void RenderpassPathTracer::createBottomLevelAccelerationStructure()
	{
		VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
		VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

		vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(m_Scene->vertices.buffer);
		indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(m_Scene->indices.buffer);

		uint32_t numTriangles = static_cast<uint32_t>(m_Scene->indices.count) / 3;
		uint32_t maxVertex = m_Scene->vertices.count;

		// Build
		VkAccelerationStructureGeometryKHR accelerationStructureGeometry = vks::initializers::accelerationStructureGeometryKHR();
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.maxVertex = maxVertex;
		accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(vkglTF::Vertex);
		accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
		accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

		// Get size info
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = vks::initializers::accelerationStructureBuildGeometryInfoKHR();
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = vks::initializers::accelerationStructureBuildSizesInfoKHR();
		vkGetAccelerationStructureBuildSizesKHR(
			m_vulkanDevice->logicalDevice,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&numTriangles,
			&accelerationStructureBuildSizesInfo);

		createAccelerationStructure(m_bottomLevelAS, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, accelerationStructureBuildSizesInfo);

		// Create a small scratch buffer used during build of the bottom level acceleration structure
		ScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = vks::initializers::accelerationStructureBuildGeometryInfoKHR();
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = m_bottomLevelAS.handle;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		if (m_rtFilterDemo->accelerationStructureFeatures.accelerationStructureHostCommands)
		{
			// Implementation supports building acceleration structure building on host
			// Implementation supports building acceleration structure building on host
			vkBuildAccelerationStructuresKHR(
				m_vulkanDevice->logicalDevice,
				VK_NULL_HANDLE,
				1,
				&accelerationBuildGeometryInfo,
				accelerationBuildStructureRangeInfos.data());
		}
		else
		{
			// Acceleration structure needs to be build on the device
			VkCommandBuffer commandBuffer = m_vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			vkCmdBuildAccelerationStructuresKHR(
				commandBuffer,
				1,
				&accelerationBuildGeometryInfo,
				accelerationBuildStructureRangeInfos.data());
			m_vulkanDevice->flushCommandBuffer(commandBuffer, m_queue);
		}

		deleteScratchBuffer(scratchBuffer);
	}

	RenderpassPathTracer::ScratchBuffer RenderpassPathTracer::createScratchBuffer(VkDeviceSize size)
	{
		RenderpassPathTracer::ScratchBuffer scratchBuffer{};
		// Buffer and memory
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(m_vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &scratchBuffer.handle));

		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(m_vulkanDevice->logicalDevice, scratchBuffer.handle, &memoryRequirements);
		VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
		memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = m_vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(m_vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &scratchBuffer.memory));

		VK_CHECK_RESULT(vkBindBufferMemory(m_vulkanDevice->logicalDevice, scratchBuffer.handle, scratchBuffer.memory, 0));

		// Buffer device address
		VkBufferDeviceAddressInfoKHR bufferDeviceAddresInfo{};
		bufferDeviceAddresInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAddresInfo.buffer = scratchBuffer.handle;
		scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(m_vulkanDevice->logicalDevice, &bufferDeviceAddresInfo);
		return scratchBuffer;
	}

	void RenderpassPathTracer::deleteScratchBuffer(ScratchBuffer& scratchBuffer)
	{
		if (scratchBuffer.memory != VK_NULL_HANDLE) {
			vkFreeMemory(m_vulkanDevice->logicalDevice, scratchBuffer.memory, nullptr);
		}
		if (scratchBuffer.handle != VK_NULL_HANDLE) {
			vkDestroyBuffer(m_vulkanDevice->logicalDevice, scratchBuffer.handle, nullptr);
		}
	}

	/*
	The top level acceleration structure contains the scene's object instances
*/
	void RenderpassPathTracer::createTopLevelAccelerationStructure()
	{
		VkTransformMatrixKHR transformMatrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f };

		VkAccelerationStructureInstanceKHR instance{};
		instance.transform = transformMatrix;
		instance.instanceCustomIndex = 0;
		instance.mask = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.accelerationStructureReference = m_bottomLevelAS.deviceAddress;

		// Buffer for instance data
		vks::Buffer instancesBuffer;
		VK_CHECK_RESULT(m_vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&instancesBuffer,
			sizeof(VkAccelerationStructureInstanceKHR),
			&instance));

		VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
		instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(instancesBuffer.buffer);

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry = vks::initializers::accelerationStructureGeometryKHR();
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
		accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

		// Get size info
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = vks::initializers::accelerationStructureBuildGeometryInfoKHR();
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		uint32_t primitive_count = 1;

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = vks::initializers::accelerationStructureBuildSizesInfoKHR();
		vkGetAccelerationStructureBuildSizesKHR(
			m_vulkanDevice->logicalDevice,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&primitive_count,
			&accelerationStructureBuildSizesInfo);

		// @todo: as return value?

		createAccelerationStructure(m_topLevelAS, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, accelerationStructureBuildSizesInfo);

		// Create a small scratch buffer used during build of the top level acceleration structure
		ScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = vks::initializers::accelerationStructureBuildGeometryInfoKHR();
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = m_topLevelAS.handle;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = 1;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		if (m_rtFilterDemo->accelerationStructureFeatures.accelerationStructureHostCommands)
		{
			// Implementation supports building acceleration structure building on host
			// Implementation supports building acceleration structure building on host
			vkBuildAccelerationStructuresKHR(
				m_vulkanDevice->logicalDevice,
				VK_NULL_HANDLE,
				1,
				&accelerationBuildGeometryInfo,
				accelerationBuildStructureRangeInfos.data());
		}
		else
		{
			// Acceleration structure needs to be build on the device
			VkCommandBuffer commandBuffer = m_vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			vkCmdBuildAccelerationStructuresKHR(
				commandBuffer,
				1,
				&accelerationBuildGeometryInfo,
				accelerationBuildStructureRangeInfos.data());
			m_vulkanDevice->flushCommandBuffer(commandBuffer, m_queue);
		}

		deleteScratchBuffer(scratchBuffer);
		instancesBuffer.destroy();
	}

	void RenderpassPathTracer::createAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
	{
		// Buffer and memory
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(m_vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &accelerationStructure.buffer));
		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(m_vulkanDevice->logicalDevice, accelerationStructure.buffer, &memoryRequirements);
		VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
		memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		VkMemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = m_vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(m_vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &accelerationStructure.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(m_vulkanDevice->logicalDevice, accelerationStructure.buffer, accelerationStructure.memory, 0));
		// Acceleration structure
		VkAccelerationStructureCreateInfoKHR accelerationStructureCreate_info{};
		accelerationStructureCreate_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreate_info.buffer = accelerationStructure.buffer;
		accelerationStructureCreate_info.size = buildSizeInfo.accelerationStructureSize;
		accelerationStructureCreate_info.type = type;
		vkCreateAccelerationStructureKHR(m_vulkanDevice->logicalDevice, &accelerationStructureCreate_info, nullptr, &accelerationStructure.handle);
		// AS device address
		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = accelerationStructure.handle;
		accelerationStructure.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(m_vulkanDevice->logicalDevice, &accelerationDeviceAddressInfo);
	}

	//void RenderpassPathTracer::createUniformBuffer()
	//{
	//	VK_CHECK_RESULT(m_vulkanDevice->createBuffer(
	//		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	//		&m_uniformBufferObject,
	//		sizeof(UniformData),
	//		&m_uniformData));
	//	VK_CHECK_RESULT(m_uniformBufferObject.map());
	//}

	/*
		If the window has been resized, we need to recreate the storage image and it's descriptor
	*/
	void RenderpassPathTracer::handleResize(uint32_t width, uint32_t height)
	{
		// Recreate image
		//createStorageImage(m_swapChain->colorFormat, { width, height, 1 });
		// Update descriptor
		VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, m_Rtoutput->view, VK_IMAGE_LAYOUT_GENERAL };
		VkWriteDescriptorSet resultImageWrite = vks::initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, B_IMAGE, &storageImageDescriptor);
		vkUpdateDescriptorSets(m_vulkanDevice->logicalDevice, 1, &resultImageWrite, 0, VK_NULL_HANDLE);
	}

	void RenderpassPathTracer::buildCommandBuffer()
	{
		if (m_commandBuffer == nullptr)
		{
			m_commandBuffer = m_vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
		}
		/*
			Dispatch the ray tracing commands
		*/
		VkCommandBufferBeginInfo commandBufferBeginInfo = vks::initializers::commandBufferBeginInfo();
		vkBeginCommandBuffer(m_commandBuffer, &commandBufferBeginInfo);

		VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vks::tools::setImageLayout(
			m_commandBuffer,
			m_Rtoutput->image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			subresourceRange);

		vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline);
		vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, 0);

		//upload the matrix to the GPU via pushconstants
		vkCmdPushConstants(m_commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(SPC_PathtracerConfig), &m_pathtracerconfig);

		VkStridedDeviceAddressRegionKHR emptySbtEntry = {};
		vkCmdTraceRaysKHR(
			m_commandBuffer,
			&m_shaderBindingTables.raygen.stridedDeviceAddressRegion,
			&m_shaderBindingTables.miss.stridedDeviceAddressRegion,
			&m_shaderBindingTables.hit.stridedDeviceAddressRegion,
			&emptySbtEntry,
			m_rtFilterDemo->width,
			m_rtFilterDemo->height,
			1);
		vkEndCommandBuffer(m_commandBuffer);
	}


	uint64_t RenderpassPathTracer::getBufferDeviceAddress(VkBuffer buffer)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
		bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAI.buffer = buffer;
		return vkGetBufferDeviceAddress(m_vulkanDevice->logicalDevice, &bufferDeviceAI);
	}

	void RenderpassPathTracer::createStorageImage(VkFormat format, VkExtent3D extent)
	{
		// Release ressources if image is to be recreated
		if (m_Rtoutput->image != VK_NULL_HANDLE) {
			vkDestroyImageView(m_vulkanDevice->logicalDevice, m_Rtoutput->view, nullptr);
			vkDestroyImage(m_vulkanDevice->logicalDevice, m_Rtoutput->image, nullptr);
			vkFreeMemory(m_vulkanDevice->logicalDevice, m_Rtoutput->mem, nullptr);
			m_Rtoutput = nullptr;
		}

		VkImageCreateInfo image = vks::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = format;
		image.extent = extent;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VK_CHECK_RESULT(vkCreateImage(m_vulkanDevice->logicalDevice, &image, nullptr, &m_Rtoutput->image));

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(m_vulkanDevice->logicalDevice, m_Rtoutput->image, &memReqs);
		VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
		memoryAllocateInfo.allocationSize = memReqs.size;
		memoryAllocateInfo.memoryTypeIndex = m_vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(m_vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &m_Rtoutput->mem));
		VK_CHECK_RESULT(vkBindImageMemory(m_vulkanDevice->logicalDevice, m_Rtoutput->image, m_Rtoutput->mem, 0));

		VkImageViewCreateInfo colorImageView = vks::initializers::imageViewCreateInfo();
		colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorImageView.format = format;
		colorImageView.subresourceRange = {};
		colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorImageView.subresourceRange.baseMipLevel = 0;
		colorImageView.subresourceRange.levelCount = 1;
		colorImageView.subresourceRange.baseArrayLayer = 0;
		colorImageView.subresourceRange.layerCount = 1;
		colorImageView.image = m_Rtoutput->image;
		VK_CHECK_RESULT(vkCreateImageView(m_vulkanDevice->logicalDevice, &colorImageView, nullptr, &m_Rtoutput->view));

		VkCommandBuffer cmdBuffer = m_vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		vks::tools::setImageLayout(cmdBuffer, m_Rtoutput->image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		m_vulkanDevice->flushCommandBuffer(cmdBuffer, m_queue);
	}

	void RenderpassPathTracer::deleteStorageImage()
	{
		vkDestroyImageView(m_vulkanDevice->logicalDevice, m_Rtoutput->view, nullptr);
		vkDestroyImage(m_vulkanDevice->logicalDevice, m_Rtoutput->image, nullptr);
		vkFreeMemory(m_vulkanDevice->logicalDevice, m_Rtoutput->mem, nullptr);
	}

}