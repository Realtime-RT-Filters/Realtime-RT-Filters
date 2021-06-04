#pragma once
#include "RaytracingManager.hpp"

namespace rtf {

	class PathTracerManager : public RaytracingManager
	{
	public:
		PathTracerManager() :RaytracingManager() {}
		~PathTracerManager() { this->cleanup(); }

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

		struct GltfShadeMaterial
		{
			glm::vec4 baseColorFactor;
			int  baseColorTexture;
			glm::vec3 emissiveFactor;
		};

		struct PrimMeshInfo
		{
			unsigned int indexOffset;
			unsigned int vertexOffset;
			int  materialIndex;
		};

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
		VkBuffer getMaterialBuffer();
		void prepare(uint32_t width, uint32_t height);
		VkDescriptorImageInfo getTexturesView();
		void createUniformBuffer() override;
		void updateUniformBuffers(float timer, Camera* camera) override;
		void buildCommandBuffer(VkCommandBuffer, VkImage, uint32_t, uint32_t);
	protected:
		VkDescriptorSetLayout pt_pushConstantDescriptorSetLayout;

	};

}

