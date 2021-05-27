#ifndef Attachment_Manager_h
#define Attachment_Manager_h

#include "disable_warnings.h"
#include <VulkanDevice.h>


namespace rtf {

	//These are global types needed by other classes
	enum class Attachment { position, normal, albedo, depth, meshid, motionvector, rtoutput, filteroutput};

	// Framebuffer
	struct FrameBufferAttachment
	{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
		VkFormat format;
	};

	/// <summary>
	/// Class managing attachments for all render passes
	/// </summary>
	class Attachment_Manager
	{

	public:
		static const uint32_t DEFAULT_WIDTH = 2048, DEFAULT_HEIGHT = 2048;


		Attachment_Manager(VkDevice* device, vks::VulkanDevice* vulkanDevice, VkPhysicalDevice* physicalDevice, uint32_t width = DEFAULT_WIDTH, uint32_t height = DEFAULT_HEIGHT);
		~Attachment_Manager();

		inline VkExtent2D GetSize() const { return m_size; }

		void createAttachment(VkFormat format, VkImageUsageFlags usage, FrameBufferAttachment* attachment, int width, int height);
		FrameBufferAttachment* getAttachment(Attachment);

		void CreateAllAttachments(int width, int height);
		void DestroyAllAttachments();

	private:

		void destroyAttachment(FrameBufferAttachment* attachment);

		//Attachment Manager needs to be aware of certain Vulkan components
		VkDevice* m_device;

		//vks::Vkdevice is a combined logical/physical vulkan device
		vks::VulkanDevice* vulkanDevice;
		VkPhysicalDevice* physicalDevice;

		// Keeping track of current attachment size
		VkExtent2D m_size;

		//List of managed Attachments
		FrameBufferAttachment m_position;
		FrameBufferAttachment m_normal;
		FrameBufferAttachment m_albedo;
		FrameBufferAttachment m_depth;
		FrameBufferAttachment m_meshid;
		FrameBufferAttachment m_motionvector;
		FrameBufferAttachment m_rtoutput;
		FrameBufferAttachment m_filteroutput;

	};
}

#endif //Attachment_Manager_h
