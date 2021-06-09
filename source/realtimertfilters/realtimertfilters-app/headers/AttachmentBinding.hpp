#ifndef AttachmentBinding_h
#define AttachmentBinding_h

#include "Attachment_Manager.hpp"

namespace rtf
{
	/// <summary>
	/// Class of configuration object used for RenderpassPostProcess (and potentially RenderpassGui down the line)
	/// </summary>
	class AttachmentBinding
	{
	public:

		/// <summary>
		/// Defines how the attachment is used in the shader
		/// </summary>
		enum class BindType
		{
			// Access via imageLoad(...) / imageStore(...)
			StorageImage,
			// Access via a sampler 
			Sampled,
		};

		/// <summary>
		/// Defines how sampling happens
		/// </summary>
		enum class SamplerType
		{
			// Using ivec2 texture coordinates
			Direct,
			// Using normalized vec2 coordinates and mipmapping
			Normalized
		};

		/// <summary>
		/// Defines how the attachment is used
		/// </summary>
		enum class AccessMode
		{
			ReadOnly,
			WriteOnly,
			ReadWrite
		};

		Attachment				m_AttachmentId;
		FrameBufferAttachment*	m_Attachment;
		AccessMode				m_AccessMode;
		BindType				m_Bind;
		SamplerType				m_Sampler;
		VkImageLayout			m_PreLayout;
		VkImageLayout			m_WorkLayout;
		VkImageLayout			m_PostLayout;
		VkImageAspectFlags		m_AspectMask;
		VkImageView				m_ImageView;

		AttachmentBinding() = default;
		
		/// <param name="attachmentid">Attachment Id</param>
		/// <param name="accessmode">Access mode</param>
		/// <param name="bindtype">Bind type</param>
		/// <param name="attachment">Pointer to attachment information. Can be resolved later via "resolveAttachment"</param>
		/// <param name="samplertype">Which sampler is to be used</param>
		/// <param name="prelayout">Expected layout before the attachment is used</param>
		/// <param name="worklayout">Expected layout during attachment use</param>
		/// <param name="postlayout">Expected layout after attachment has been used</param>
		/// <param name="aspectflags">TBD</param>
		AttachmentBinding(
			Attachment				attachmentid,
			AccessMode				accessmode,
			BindType				bindtype = BindType::StorageImage,
			FrameBufferAttachment*	attachment = nullptr,
			SamplerType				samplertype = SamplerType::Direct,
			VkImageLayout			prelayout = VK_IMAGE_LAYOUT_GENERAL,
			VkImageLayout			worklayout = VK_IMAGE_LAYOUT_GENERAL,
			VkImageLayout			postlayout = VK_IMAGE_LAYOUT_GENERAL,
			VkImageAspectFlags		aspectflags = VK_IMAGE_ASPECT_COLOR_BIT
		);

		/// <summary>
		/// Fills m_Attachment by accessing manager with m_AttachmentId
		/// </summary>
		void resolveAttachment(vks::VulkanDevice* vulkanDevice, Attachment_Manager* manager);
		void createImageview(vks::VulkanDevice* vulkanDevice);
		/// <summary>
		/// Returns the correct descriptor type based on m_Bind
		/// </summary>
		inline VkDescriptorType getDescriptorType() const;
		/// <summary>
		/// Selects the correct sampler based on m_Bind and m_Sampler
		/// </summary>
		inline VkSampler selectSampler(VkSampler direct, VkSampler normalized) const;

		/// <summary>
		/// Returns true, if accessmode is either readonly or readwrite
		/// </summary>
		inline bool readAccess() const;
		/// <summary>
		/// Returns true, if accessmode is either writeonly or readwrite
		/// </summary>
		inline bool writeAccess() const;
		/// <summary>
		/// Returns true if the bindmode and accessmode require the definition of a descriptor
		/// </summary>
		inline bool usesDescriptor() const;
		/// <summary>
		/// Returns true if the bindmode requires the definition of an attachment description
		/// </summary>
		inline bool usesAttachmentDescription() const;
		/// <summary>
		/// Returns true if the defined layouts require a layout conversion before use
		/// </summary>
		inline bool requireImageTransition() const;

		/// <summary>
		/// Makes VkDescriptorSetLayoutBinding struct
		/// </summary>
		VkDescriptorSetLayoutBinding makeDescriptorSetLayoutBinding(uint32_t binding) const;
		/// <summary>
		/// Makes VkDescriptorImageInfo struct
		/// </summary>
		VkDescriptorImageInfo makeDescriptorImageInfo(VkSampler direct, VkSampler normalized) const;
		/// <summary>
		/// Makes VkWriteDescriptorSet struct
		/// </summary>
		VkWriteDescriptorSet makeWriteDescriptorSet(VkDescriptorSet descrSet, uint32_t binding, VkDescriptorImageInfo* imageInfo) const;

		void makeImageTransition(VkCommandBuffer commandBuffer) const;
		
		/// <summary>
		/// Fill a vector of VkDescriptorSetLayoutBindings based on attachment bindings
		/// </summary>
		/// <param name="vector">target</param>
		/// <param name="data">source data ptr</param>
		/// <param name="count">count</param>
		/// <param name="baseBinding">binding offset</param>
		static void FillLayoutBindingVector(std::vector<VkDescriptorSetLayoutBinding>& vector, const AttachmentBinding* data, uint32_t count, uint32_t baseBinding = 0);
		
		/// <summary>
		/// Calls VkCreateDescriptorSetLayout with the correct parameters based on attachmentbindings
		/// </summary>
		/// <param name="logicalDevice">VkDevice</param>
		/// <param name="out">VkDescriptorSetLayout output</param>
		/// <param name="data">source data ptr</param>
		/// <param name="count">count</param>
		static void CreateDescriptorSetLayout(VkDevice logicalDevice, VkDescriptorSetLayout& out, const AttachmentBinding* data, uint32_t count);

		/// <summary>
		/// Adds entries to target vector for StorageImages and Samplers, as required
		/// </summary>
		static void FillPoolSizesVector(std::vector<VkDescriptorPoolSize>& vector, const AttachmentBinding* data, uint32_t count);

		/// <summary>
		/// Calls VkCreateDescriptorPool with the correct parameters based on attachmentbindings
		/// </summary>
		/// <param name="logicalDevice">VkDevice</param>
		/// <param name="out">VkDescriptorPool output</param>
		/// <param name="data">source data ptr</param>
		/// <param name="count">count</param>
		/// <param name="maxSets">max sets that can be create with this pool. Makes no sense to be more than 1 in most cases, when using this function.</param>
		static void CreateDescriptorPool(VkDevice logicalDevice, VkDescriptorPool& out, const AttachmentBinding* data, uint32_t count, uint32_t maxSets = 1);
		
		/// <summary>
		/// Prepares for the vkwriteDescriptorset command by filling imageInfos and writes vector.
		/// </summary>
		/// <param name="data">source data ptr</param>
		/// <param name="count">count</param>
		/// <param name="descrSet">descriptor set to write to</param>
		/// <param name="direct">Sampler to use for direct sampling access</param>
		/// <param name="normalized">Sampler to use for normalized sampling access</param>
		/// <param name="baseBinding">binding offset</param>
		static void FillWriteDescriptorSetStructures(std::vector<VkDescriptorImageInfo>& imageInfos, std::vector<VkWriteDescriptorSet>& writes, const AttachmentBinding* data, uint32_t count, VkDescriptorSet descrSet, VkSampler direct, VkSampler normalized, uint32_t baseBinding = 0);

		/// <summary>
		/// Calls VkUpateDescriptorSet() with the correct parameters based on Attachmentbindings
		/// </summary>
		/// <param name="logicalDevice">VkDevice</param>
		/// <param name="data">source data ptr</param>
		/// <param name="count">count</param>
		/// <param name="descrSet">descriptor set to write to</param>
		/// <param name="direct">Sampler to use for direct sampling access</param>
		/// <param name="normalized">Sampler to use for normalized sampling access</param>
		/// <param name="baseBinding">binding offset</param>
		static void UpdateDescriptorSet(VkDevice logicalDevice, const AttachmentBinding* data, uint32_t count, VkDescriptorSet descrSet, VkSampler direct, VkSampler normalized, uint32_t baseBinding = 0);

		/// <summary>
		/// Fills attachment description structures based on AttachmentBindings
		/// </summary>
		/// <param name="descriptions">VkAttachmentDescriptions</param>
		/// <param name="inputrefs">VkAttachmentReferences used as inputAttachments</param>
		/// <param name="outputrefs">VkAttachmentReferences used as colorAttachments</param>
		/// <param name="data">source data ptr</param>
		/// <param name="count">count</param>
		static void FillAttachmentDescriptionStructures(std::vector<VkAttachmentDescription>& descriptions, std::vector<VkAttachmentReference>& inputrefs, std::vector<VkAttachmentReference>& outputrefs, const AttachmentBinding* data, uint32_t count);

	};

	inline VkDescriptorType AttachmentBinding::getDescriptorType() const
	{
		if (m_Bind == BindType::StorageImage)
		{
			return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		}
		else
		{
			return VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}
	}

	inline VkSampler rtf::AttachmentBinding::selectSampler(VkSampler direct, VkSampler normalized) const
	{
		switch (m_Bind)
		{
		case BindType::Sampled:
			return (m_Sampler == SamplerType::Direct) ? direct : normalized;
		case BindType::StorageImage:
		default:
			return nullptr;
		}
	}
	inline bool AttachmentBinding::readAccess() const
	{
		return (m_AccessMode == AccessMode::ReadOnly) || m_AccessMode == AccessMode::ReadWrite;
	}
	inline bool AttachmentBinding::writeAccess() const
	{
		return m_AccessMode == AccessMode::WriteOnly || m_AccessMode == AccessMode::ReadWrite;
	}
	inline bool AttachmentBinding::usesDescriptor() const
	{
		if (m_Bind == BindType::StorageImage)
		{
			return true; // A storageImage always needs a descriptor
		}
		return !writeAccess(); // Output attachments are not sampled, and don't require a descriptor
	}
	inline bool AttachmentBinding::usesAttachmentDescription() const
	{
		return m_Bind == BindType::Sampled && writeAccess();
	}
	inline bool AttachmentBinding::requireImageTransition() const
	{
		return m_PreLayout != m_WorkLayout;
	}
}


#endif AttachmentBinding_h