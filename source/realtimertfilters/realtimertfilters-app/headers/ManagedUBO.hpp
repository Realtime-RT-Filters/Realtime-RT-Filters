#ifndef ManagedUBO_h
#define ManagedUBO_h

#include <VulkanDevice.h>
#include <glm/glm.hpp>
#include "../data/shaders/glsl/ubo_definitions.glsl"
#include <memory>

namespace rtf
{
	class UBOInterface
	{
	public:
		virtual void	prepare() = 0;
		virtual void	update() = 0;
		virtual void	destroy() = 0;
		virtual size_t	getUBOSize() const = 0;
		virtual void*	getUBOData() = 0;

		virtual void writeDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding, VkWriteDescriptorSet& dest) const = 0;
		virtual VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const = 0;

		template<typename T_UBO>
		T_UBO& getUBO() 
		{ 
			assert(sizeof(T_UBO) == getUBOSize());
			T_UBO* data = reinterpret_cast<T_UBO*>(getUBOData()); 
			return *data;
		}
	};

	using UBOPtr = std::shared_ptr<UBOInterface>;

	template<typename T_UBO>
	class ManagedUBO : public UBOInterface
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

		inline virtual void prepare() override;
		inline virtual void update() override;
		inline virtual void destroy() override;
		inline virtual size_t getUBOSize() const override;
		inline virtual void* getUBOData() override;

		inline virtual void writeDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding, VkWriteDescriptorSet& dest) const override;
		inline virtual VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const override;
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
	inline size_t ManagedUBO<T_UBO>::getUBOSize() const
	{
		return sizeof(T_UBO);
	}
	template<typename T_UBO>
	inline void* ManagedUBO<T_UBO>::getUBOData()
	{
		return &m_UBO;
	}
	template<typename T_UBO>
	inline void ManagedUBO<T_UBO>::writeDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding, VkWriteDescriptorSet& dest) const
	{
		dest.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		dest.dstSet = descriptorSet;
		dest.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		dest.dstBinding = binding;
		dest.pBufferInfo = &m_buffer.descriptor;
		dest.descriptorCount = 1;
	}
	template<typename T_UBO>
	inline VkWriteDescriptorSet ManagedUBO<T_UBO>::writeDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const
	{
		VkWriteDescriptorSet result{};
		writeDescriptorSet(descriptorSet, binding, result);
		return result;
	}
}

#endif ManagedUBO_h
