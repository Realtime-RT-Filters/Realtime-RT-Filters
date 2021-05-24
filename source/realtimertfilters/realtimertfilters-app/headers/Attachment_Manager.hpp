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
		Attachment_Manager(VkDevice* device, vks::VulkanDevice* vulkanDevice, VkPhysicalDevice* physicalDevice);
		~Attachment_Manager();


		void createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment* attachment, int width, int height);
		FrameBufferAttachment* getAttachment(Attachment);


	private:

		void createAllAttachments();

		//Attachment Manager needs to be aware of certain Vulkan components
		VkDevice* device;

		//vks::Vkdevice is a combined logical/physical vulkan device
		vks::VulkanDevice* vulkanDevice;
		VkPhysicalDevice* physicalDevice;



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
