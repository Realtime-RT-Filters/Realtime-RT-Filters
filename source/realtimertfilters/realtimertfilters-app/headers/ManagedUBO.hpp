#ifndef ManagedUBO_h
#define ManagedUBO_h

#include <VulkanDevice.h>
#include <glm/glm.hpp>
#include "../data/shaders/glsl/ubo_definitions.glsl"
#include <memory>

namespace rtf
{
	template<typename T_UBO>
	class ManagedUBO
	{
	protected:
		vks::VulkanDevice* m_vulkanDevice;
		vks::Buffer m_buffer;
		T_UBO m_UBO;

	public:
		using Ptr = std::shared_ptr<ManagedUBO<T_UBO>>;

		explicit inline ManagedUBO(vks::VulkanDevice* vulkanDevice);
		ManagedUBO(const ManagedUBO<T_UBO>& other) = delete;
		ManagedUBO(const ManagedUBO<T_UBO>&& other) = delete;
		void operator=(const ManagedUBO<T_UBO>& other) = delete;
		~ManagedUBO();

		inline T_UBO& UBO() { return m_UBO; }

		inline virtual void prepare();
		inline virtual void update();
		inline virtual void destroy();

		inline virtual void writeDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding, VkWriteDescriptorSet& dest);
		inline virtual VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding);
	};

	using UBO_SceneInfo = ManagedUBO<S_Sceneinfo>::Ptr;
	using UBO_Guibase = ManagedUBO<S_Guibase>::Ptr;

	template<typename T_UBO>
	ManagedUBO<T_UBO>::ManagedUBO(vks::VulkanDevice* vulkanDevice)
		: m_vulkanDevice(vulkanDevice), m_buffer(), m_UBO()
	{}

	template<typename T_UBO>
	inline ManagedUBO<T_UBO>::~ManagedUBO()
	{}

	template<typename T_UBO>
	void ManagedUBO<T_UBO>::prepare()
	{
		// Offscreen vertex shader
		VK_CHECK_RESULT(m_vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_buffer,
			sizeof(m_UBO)));

		// Map persistent
		VK_CHECK_RESULT(m_buffer.map());
	}

	template<typename T_UBO>
	inline void ManagedUBO<T_UBO>::update()
	{
		memcpy(m_buffer.mapped, &m_UBO, sizeof(m_UBO));
	}

	template<typename T_UBO>
	inline void ManagedUBO<T_UBO>::destroy()
	{
		m_buffer.destroy();
	}
	template<typename T_UBO>
	inline void ManagedUBO<T_UBO>::writeDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding, VkWriteDescriptorSet& dest)
	{
		dest.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		dest.dstSet = descriptorSet;
		dest.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		dest.dstBinding = binding;
		dest.pBufferInfo = &m_buffer.descriptor;
		dest.descriptorCount = 1;
	}
	template<typename T_UBO>
	inline VkWriteDescriptorSet ManagedUBO<T_UBO>::writeDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding)
	{
		VkWriteDescriptorSet result{};
		writeDescriptorSet(descriptorSet, binding, result);
		return result;
	}
}

#endif ManagedUBO_h
