#ifndef AttachmentBinding_h
#define AttachmentBinding_h

#include "Attachment_Manager.hpp"

namespace rtf
{
	/// <summary>
	/// Class of configuration object used for RenderpassPostProcess (and potentially RenderpassGui down the line)
	/// </summary>
	class TextureBinding
	{
	public:

		/// <summary>
		/// Type determines how the binding is used
		/// </summary>
		enum class Type
		{
			// A readonly storage image, e.g.	layout (binding = 0, rgba32f) uniform readonly image2D Tex_Source
			StorageImage_ReadOnly,
			// A writeonly storage image, e.g.	layout (binding = 0, rgba32f) uniform writeonly image2D Tex_Output
			StorageImage_WriteOnly,
			// A readwrite storage image, e.g.	layout (binding = 0, rgba32f) uniform image2D Tex_InOut
			StorageImage_ReadWrite,
			// A sampled image, e.g.			layout (binding = 0) uniform sampler2D Tex_Source
			Sampler_ReadOnly,
			// A subpass output, e.g.			layout (location = 0) out vec4 Out_Image
			Subpass_Output
		};

		Attachment				m_AttachmentId;
		Type					m_Type;
		FrameBufferAttachment*	m_Attachment;
		VkImageLayout			m_PreLayout;
		VkImageLayout			m_WorkLayout;
		VkImageLayout			m_PostLayout;
		VkImageAspectFlags		m_AspectMask;
		VkImageView				m_ImageView;

		TextureBinding() = default;
		
		/// <param name="attachmentid">Attachment Id</param>
		/// <param name="type">Bind type</param>
		/// <param name="attachment">Pointer to attachment information. Can be resolved later via "resolveAttachment"</param>
		/// <param name="prelayout">Expected layout before the attachment is used</param>
		/// <param name="worklayout">Expected layout during attachment use</param>
		/// <param name="postlayout">Expected layout after attachment has been used</param>
		/// <param name="aspectflags">TBD</param>
		TextureBinding(
			Attachment				attachmentid,
			Type					type,
			FrameBufferAttachment*	attachment = nullptr,
			VkImageLayout			prelayout = VK_IMAGE_LAYOUT_GENERAL,
			VkImageLayout			worklayout = VK_IMAGE_LAYOUT_GENERAL,
			VkImageLayout			postlayout = VK_IMAGE_LAYOUT_GENERAL,
			VkImageAspectFlags		aspectflags = 0
		);

		/// <summary>
		/// Fills m_Attachment by accessing manager with m_AttachmentId
		/// </summary>
		void resolveAttachment(vks::VulkanDevice* vulkanDevice, Attachment_Manager* manager);
		void createImageview(vks::VulkanDevice* vulkanDevice);
		void destroyImageview(vks::VulkanDevice* vulkanDevice);
		/// <summary>
		/// Returns the correct descriptor type based on m_Bind
		/// </summary>
		inline VkDescriptorType getDescriptorType() const;
		/// <summary>
		/// Selects the correct sampler based on m_Bind and m_Sampler
		/// </summary>
		inline VkSampler selectSampler(VkSampler sampler) const;

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
		VkDescriptorImageInfo makeDescriptorImageInfo(VkSampler sampler) const;
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
		static void FillLayoutBindingVector(std::vector<VkDescriptorSetLayoutBinding>& vector, const TextureBinding* data, uint32_t count, uint32_t baseBinding = 0);
		
		/// <summary>
		/// Calls VkCreateDescriptorSetLayout with the correct parameters based on attachmentbindings
		/// </summary>
		/// <param name="logicalDevice">VkDevice</param>
		/// <param name="out">VkDescriptorSetLayout output</param>
		/// <param name="data">source data ptr</param>
		/// <param name="count">count</param>
		static void CreateDescriptorSetLayout(VkDevice logicalDevice, VkDescriptorSetLayout& out, const TextureBinding* data, uint32_t count);

		/// <summary>
		/// Adds entries to target vector for StorageImages and Samplers, as required
		/// </summary>
		static void FillPoolSizesVector(std::vector<VkDescriptorPoolSize>& vector, const TextureBinding* data, uint32_t count);

		/// <summary>
		/// Calls VkCreateDescriptorPool with the correct parameters based on attachmentbindings
		/// </summary>
		/// <param name="logicalDevice">VkDevice</param>
		/// <param name="out">VkDescriptorPool output</param>
		/// <param name="data">source data ptr</param>
		/// <param name="count">count</param>
		/// <param name="maxSets">max sets that can be create with this pool. Makes no sense to be more than 1 in most cases, when using this function.</param>
		static void CreateDescriptorPool(VkDevice logicalDevice, VkDescriptorPool& out, const TextureBinding* data, uint32_t count, uint32_t maxSets = 1);
		
		/// <summary>
		/// Prepares for the vkwriteDescriptorset command by filling imageInfos and writes vector.
		/// </summary>
		/// <param name="data">source data ptr</param>
		/// <param name="count">count</param>
		/// <param name="descrSet">descriptor set to write to</param>
		/// <param name="direct">Sampler to use for direct sampling access</param>
		/// <param name="normalized">Sampler to use for normalized sampling access</param>
		/// <param name="baseBinding">binding offset</param>
		static void FillWriteDescriptorSetStructures(std::vector<VkDescriptorImageInfo>& imageInfos, std::vector<VkWriteDescriptorSet>& writes, const TextureBinding* data, uint32_t count, VkDescriptorSet descrSet, VkSampler sampler, uint32_t baseBinding = 0);

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
		static void UpdateDescriptorSet(VkDevice logicalDevice, const TextureBinding* data, uint32_t count, VkDescriptorSet descrSet, VkSampler sampler, uint32_t baseBinding = 0);

		/// <summary>
		/// Fills attachment description structures based on AttachmentBindings
		/// </summary>
		/// <param name="descriptions">VkAttachmentDescriptions</param>
		/// <param name="inputrefs">VkAttachmentReferences used as inputAttachments</param>
		/// <param name="outputrefs">VkAttachmentReferences used as colorAttachments</param>
		/// <param name="data">source data ptr</param>
		/// <param name="count">count</param>
		static void FillAttachmentDescriptionStructures(std::vector<VkAttachmentDescription>& descriptions, std::vector<VkAttachmentReference>& inputrefs, std::vector<VkAttachmentReference>& outputrefs, const TextureBinding* data, uint32_t count);

	};

	inline VkDescriptorType TextureBinding::getDescriptorType() const
	{
		switch (m_Type)
		{
		case Type::StorageImage_ReadOnly:
		case Type::StorageImage_ReadWrite:
			return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case Type::Sampler_ReadOnly:
			return VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case Type::Subpass_Output:
		default:
			return VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}

	inline VkSampler rtf::TextureBinding::selectSampler(VkSampler sampler) const
	{
		if (m_Type == Type::Sampler_ReadOnly)
		{
			return sampler;
		}
		return nullptr;
	}
	inline bool TextureBinding::readAccess() const
	{
		return (m_Type == Type::StorageImage_ReadOnly || m_Type == Type::StorageImage_ReadWrite || m_Type == Type::Sampler_ReadOnly);
	}
	inline bool TextureBinding::writeAccess() const
	{
		return (m_Type == Type::StorageImage_ReadWrite || m_Type == Type::StorageImage_WriteOnly || m_Type == Type::Subpass_Output);
	}
	inline bool TextureBinding::usesDescriptor() const
	{
		return !(m_Type == Type::Subpass_Output);
	}
	inline bool TextureBinding::usesAttachmentDescription() const
	{
		return m_Type == Type::Subpass_Output;
	}
	inline bool TextureBinding::requireImageTransition() const
	{
		return m_PreLayout != m_WorkLayout;
	}
}


#endif AttachmentBinding_h