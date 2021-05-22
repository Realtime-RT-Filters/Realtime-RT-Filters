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

		void createRayTracingPipeline() override;
	};
}

