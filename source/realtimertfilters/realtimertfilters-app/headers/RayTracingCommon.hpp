#ifndef __RAY_COMMON
#define __RAY_COMMON
#include "RTFilterDemo.hpp"


namespace rtf
{
	class RayTracingComponent {

	public:
		void setQueue(VkQueue queue)
		{
			this->m_queue = queue;
		}
		void setSwapChain(VulkanSwapChain* chain)
		{
			this->m_swapChain = chain;
		}

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

		struct StorageImage {
			VkDeviceMemory memory = VK_NULL_HANDLE;
			VkImage image = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VkFormat format;
		}m_storageImage;

		struct ShaderBindingTables {
			ShaderBindingTable raygen;
			ShaderBindingTable miss;
			ShaderBindingTable hit;
		}m_shaderBindingTables;

		struct UniformData {
			glm::mat4 viewInverse;
			glm::mat4 projInverse;
			glm::vec4 lightPos;
			int32_t vertexSize;
		}m_uniformData;
		vks::Buffer m_uniformBufferObject;

		struct PushConstant{
			glm::vec4	  clearColor;
			glm::vec3	  lightPosition;
			float         lightIntensity;
			int           lightType; // -1: off, 0: point, 1: infinite
			int           frame;
			int           samples;
			int           bounces;
			int           bounceSamples;
		}m_pushConstant;

		// ================ Struct types END ================

		// ShaderGroups
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};

		VkQueue m_queue{};
		VulkanSwapChain* m_swapChain{};
		Camera* m_camera{};
		// set via setter after scene loaded
		vkglTF::Model* m_Scene{};

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

		// Available features and properties
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
		VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features;

		// Enabled features and properties
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
		VkPhysicalDeviceVulkan12Features enabledPhysicalDeviceVulkan12Features;

	};
}


#endif //__RAY_COMMON