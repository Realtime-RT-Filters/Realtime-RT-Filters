#ifndef __RAY_COMMON
#define __RAY_COMMON
//#include "RTFilterDemo.hpp"

#include <vulkanexamplebase.h>
#include "VulkanglTFModel.h"
#include "../data/shaders/glsl/ubo_definitions.glsl"
#include "../data/shaders/glsl/pathTracerShader/gltf.glsl"

namespace rtf
{
	class RayTracingComponent {
	protected:
		// ================ Struct types ================

		class ShaderBindingTable : public vks::Buffer {
		public:
			VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};
		};

		struct ScratchBuffer{
			uint64_t deviceAddress = 0;
			VkBuffer handle = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
		}m_scratchBuffer;

		struct AccelerationStructure {
			VkAccelerationStructureKHR handle;
			uint64_t deviceAddress = 0;
			VkDeviceMemory memory;
			VkBuffer buffer;
		}m_bottomLevelAS, m_topLevelAS;

		struct ShaderBindingTables {
			ShaderBindingTable raygen;
			ShaderBindingTable miss;
			ShaderBindingTable hit;
		}m_shaderBindingTables;

	public:
		/// <summary>
		/// Push constant used to configure path tracer
		/// </summary>
		SPC_PathtracerConfig m_pathtracerconfig;

		// ================ Struct types END ================

	protected:
		// ShaderGroups
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};

		VkQueue m_queue{};
		Camera* m_camera{};
		// set via setter after scene loaded
		vkglTF::Model* m_Scene{};
		std::vector<GltfShadeMaterial> m_materials;
		vks::Buffer m_material_buffer;
		std::vector<VkDescriptorImageInfo> m_textures;

		PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
		PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
		PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
		PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
		PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
		PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
		PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
		PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
		PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
		PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

	};
}

#endif //__RAY_COMMON