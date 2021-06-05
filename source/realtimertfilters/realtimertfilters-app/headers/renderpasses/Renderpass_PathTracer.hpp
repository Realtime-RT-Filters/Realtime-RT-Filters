#ifndef Renderpass_PT_h
#define Renderpass_PT_h

#include "Renderpass.hpp"
#include "../../headers/RayTracingCommon.hpp"
#include "../../headers/VulkanglTFModel.h"

namespace rtf
{
	class RenderpassPathTracer : public Renderpass, public RayTracingComponent
	{
	public:
		RenderpassPathTracer() {}

		// Parent methods override
		void prepare() override {};
		void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) override {};
		void cleanUp() override {};
		void updateUniformBuffer() override {};
//
//	protected:
//		void createStorageImage(VkFormat format, VkExtent3D extent);
//		void createUniformBuffer();
//		void updateUniformBuffers(float timer, Camera* camera);
//		void buildCommandBuffer(VkCommandBuffer commandBuffer, VkImage swapchainImage, uint32_t width, uint32_t height);
//		void createRayTracingPipeline();
//		void createShaderBindingTables();
//		void createShaderBindingTable(ShaderBindingTable& table, uint32_t);
//		void createDescriptorSets();
//		void deleteStorageImage();
//		void deleteScratchBuffer(ScratchBuffer& scratchBuffer);
//		VkStridedDeviceAddressRegionKHR getSbtEntryStridedDeviceAddressRegion(VkBuffer, uint32_t);
//		void enableExtensions(std::vector<const char*>& enabledDeviceExtensions);
//		VkPhysicalDeviceAccelerationStructureFeaturesKHR* getEnabledFeatures();
//		void handleResize(uint32_t width, uint32_t height);
//
//		void createBottomLevelAccelerationStructure();
//		ScratchBuffer createScratchBuffer(VkDeviceSize);
//		void createTopLevelAccelerationStructure();
//		void createAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
//
//		uint64_t getBufferDeviceAddress(VkBuffer buffer);
//		void initData();
	};
}

#endif //Renderpass_PR_h
