#ifndef Renderpass_PT_h
#define Renderpass_PT_h

#include "Renderpass.hpp"
#include "../../headers/RayTracingCommon.hpp"
#include "../../headers/VulkanglTFModel.h"

namespace rtf
{
	struct FrameBufferAttachment;

	class RenderpassPathTracer : public Renderpass, public RayTracingComponent
	{
	public:
		RenderpassPathTracer() {}

		// Parent methods override
		void prepare() override;
		void draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount) override;
		void cleanUp() override;
		void updateUniformBuffer() override;

	protected:
		// Init 
		void initData();
		void prepareAttachement();
		
		// Build
		void buildCommandBuffer();
		
		// Create Methods
		void createStorageImage(VkFormat format, VkExtent3D extent);
		void createUniformBuffer();
		void createRayTracingPipeline();
		void createShaderBindingTables();
		void createShaderBindingTable(ShaderBindingTable& table, uint32_t);
		void createDescriptorSets();
		ScratchBuffer createScratchBuffer(VkDeviceSize);

		void createAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
		void createBottomLevelAccelerationStructure();
		void createTopLevelAccelerationStructure();

		// Delete methods
		void deleteStorageImage();
		void deleteScratchBuffer(ScratchBuffer&);
		void deleteAccelerationStructure(AccelerationStructure&);

		void updatePushConstants();

		// Helper 
		VkStridedDeviceAddressRegionKHR getSbtEntryStridedDeviceAddressRegion(VkBuffer, uint32_t);
		//VkPhysicalDeviceAccelerationStructureFeaturesKHR* getEnabledFeatures();
		void handleResize(uint32_t width, uint32_t height);
		uint64_t getBufferDeviceAddress(VkBuffer buffer);
		
	private:
		FrameBufferAttachment* m_PositionAttachment, * m_NormalAttachment, * m_AlbedoAttachment, * m_MotionAttachment, * m_Rtoutput, * m_Filteroutput;
		float m_timer{};
		VkCommandBuffer m_commandBuffer{};
	};
}

#endif //Renderpass_PR_h
