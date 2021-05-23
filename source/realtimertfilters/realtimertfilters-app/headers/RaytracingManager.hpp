#ifndef Raytraching_hpp
#define Raytraching_hpp

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

namespace rtf
{

#pragma region helper_structs
	// Holds information for a ray tracing scratch buffer that is used as a temporary storage
	struct ScratchBuffer
	{
		uint64_t deviceAddress = 0;
		VkBuffer handle = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	// Holds information for a ray tracing tracing acceleration structure
	struct AccelerationStructure {
		VkAccelerationStructureKHR handle;
		uint64_t deviceAddress = 0;
		VkDeviceMemory memory;
		VkBuffer buffer;
	};

	// Holds information for a storage image that the ray tracing shaders output to
	struct StorageImage {
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkFormat format;
	};
#pragma endregion helper_structs

	class RTFilterDemo;

	class RaytracingManager
	{
	public:
		RaytracingManager() = default;
		~RaytracingManager();

		virtual void setup(
			RTFilterDemo* rtFilterDemo,
			VkPhysicalDevice physicalDevice,
			vks::VulkanDevice* vulkanDevice,
			VkDevice device,
			VkQueue queue,
			VulkanSwapChain* swapChain,
			VkDescriptorPool descriptorPool,
			Camera* camera
		);

		virtual void cleanup();
		virtual void buildCommandBuffer(VkCommandBuffer commandBuffer, VkImage swapchainImage, uint32_t width, uint32_t height);
		VkPhysicalDeviceAccelerationStructureFeaturesKHR* getEnabledFeatures();
		
		// Extends the buffer class and holds information for a shader binding table
		class ShaderBindingTable : public vks::Buffer {
		public:
			VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};
		};

		StorageImage storageImage;

		virtual void enableExtensions(std::vector<const char*>& enabledDeviceExtensions);
		virtual void prepare(uint32_t width, uint32_t height);
		virtual ScratchBuffer createScratchBuffer(VkDeviceSize size);
		virtual void deleteScratchBuffer(ScratchBuffer& scratchBuffer);
		virtual void createAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
		virtual void deleteAccelerationStructure(AccelerationStructure& accelerationStructure);
		virtual uint64_t getBufferDeviceAddress(VkBuffer buffer);
		virtual void createStorageImage(VkFormat format, VkExtent3D extent);
		virtual void deleteStorageImage();
		VkStridedDeviceAddressRegionKHR getSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount);
		virtual void createShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount);

#pragma region vulkan_function_pointers
		// Function pointers for ray tracing related stuff
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
		VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
		VkPhysicalDeviceVulkan12Features enabledPhysicalDeviceVulkan12Features;
#pragma endregion vulkan_function_pointers

		//Ray tracing components
		AccelerationStructure bottomLevelAS;
		AccelerationStructure topLevelAS;

		std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
		struct ShaderBindingTables {
			ShaderBindingTable raygen;
			ShaderBindingTable miss;
			ShaderBindingTable hit;
		} shaderBindingTables;

		struct UniformData {
			glm::mat4 viewInverse;
			glm::mat4 projInverse;
			glm::vec4 lightPos;
			int32_t vertexSize;
		} uniformData;
		vks::Buffer ubo;

		struct PushConstant
		{
			glm::vec4	clearColor{ 0.0f, 0.0f, 0.0f, 0.0f };
			glm::vec3	lightPosition{ 0.f, 4.5f, 0.f };
			float         lightIntensity{ 10.f };
			int           lightType{ -1 }; // -1: off, 0: point, 1: infinite
			int           frame{ 0 };
			int           samples{ 2 };
			int           bounces{ 2 };
			int           bounceSamples{ 2 };
			float         temporalAlpha{ 0.1f };
		} m_pushConstants;

		VkDescriptorSet rt_descriptorSet;
		VkDescriptorSetLayout rt_descriptorSetLayout;
		VkPipeline rt_pipeline;
		VkPipelineLayout rt_pipelineLayout;

		virtual void createBottomLevelAccelerationStructure();
		virtual void createTopLevelAccelerationStructure();
		virtual void createShaderBindingTables();
		virtual void createRayTracingPipeline();
		virtual void createDescriptorSets();
		virtual void createUniformBuffer();
		virtual void handleResize(uint32_t width, uint32_t height);
		virtual void updateUniformBuffers(float timer, Camera* camera);

		virtual void setScene(vkglTF::Model* scene) { m_Scene = scene; }

	protected:
		// properties that need to be passed by the vulkanexamplebase class:
		VkPhysicalDevice physicalDevice{};
		vks::VulkanDevice* vulkanDevice{};
		VkDevice device{};
		VkQueue queue{};
		VulkanSwapChain* swapChain{};
		VkDescriptorPool descriptorPool{};
		Camera* camera{};

		// set via setter after scene loaded
		vkglTF::Model* m_Scene{};

		RTFilterDemo* m_rtFilterDemo{};
	};
}

#endif