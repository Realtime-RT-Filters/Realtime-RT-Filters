#pragma once
#include "RaytracingManager.hpp"

namespace rtf {

	class PathTracerManager : public RaytracingManager
	{
	public:
		PathTracerManager() :RaytracingManager() {}
		~PathTracerManager() { this->cleanup(); }

		void setup(
			RTFilterDemo* rtFilterDemo,
			VkPhysicalDevice physicalDevice,
			vks::VulkanDevice* vulkanDevice,
			VkDevice device,
			VkQueue queue,
			VulkanSwapChain* swapChain,
			VkDescriptorPool descriptorPool,
			Camera* camera
		) {
			RaytracingManager::setup(rtFilterDemo, physicalDevice, vulkanDevice, device, queue, swapChain, descriptorPool, camera);
		}

		void cleanup() override;
		void createRayTracingPipeline() override;
		void createDescriptorSets() override;
		void prepare(uint32_t width, uint32_t height);
		void createUniformBuffer() override;
		void updateUniformBuffers(float timer, Camera* camera) override;
		void buildCommandBuffer(VkCommandBuffer, VkImage, uint32_t, uint32_t);

	protected:
		VkDescriptorSetLayout pt_pushConstantDescriptorSetLayout;
	};

}

