#ifndef Attachment_Manager_h
#define Attachment_Manager_h

#include "disable_warnings.h"
#include <VulkanDevice.h>


namespace rtf {

	//These are global types needed by other classes
	enum Attachment { position, normal, albedo, output_rt, output_filter };

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
		Attachment_Manager(VkDevice device, vks::VulkanDevice* vulkanDevice);
		~Attachment_Manager();

		void createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment* attachment, int width, int height);
		


	private:

		//Attachment Manager needs to be aware of certain Vulkan components
		//VkInstance* instance;
		VkDevice device;
		//This Vkdevice is a combined logical/physical vulkan device
		vks::VulkanDevice* vulkanDevice;

	};
}

#endif Attachment_Manager_h