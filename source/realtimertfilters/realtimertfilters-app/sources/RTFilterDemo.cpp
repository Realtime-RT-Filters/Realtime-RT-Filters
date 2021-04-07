#include "../headers/RTFilterDemo.hpp"

rtf::RTFilterDemo::RTFilterDemo()
	: VulkanRaytracingSample()
{
	title = "RT Filter Demo";
	settings.overlay = false;
	camera.type = Camera::CameraType::lookat;
	camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
	camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.setTranslation(glm::vec3(0.0f, 0.0f, -2.5f));

	// Require Vulkan 1.1
	apiVersion = VK_API_VERSION_1_1;

	// Ray tracing related extensions required by this sample
	enabledDeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	enabledDeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

	// Required by VK_KHR_acceleration_structure
	enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	enabledDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

	// Required for VK_KHR_ray_tracing_pipeline
	enabledDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

	// Required by VK_KHR_spirv_1_4
	enabledDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
}

void rtf::RTFilterDemo::render()
{
	VulkanRaytracingSample::prepareFrame();
	VulkanRaytracingSample::submitFrame();
}