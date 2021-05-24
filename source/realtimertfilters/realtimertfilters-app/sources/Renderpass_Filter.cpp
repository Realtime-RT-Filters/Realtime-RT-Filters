#include "../headers/Renderpass_Filter.hpp"

namespace rtf
{

	Renderpass_Filter::Renderpass_Filter(VkDevice* device, Attachment_Manager* attachment_manager)
	{

		m_attachment_manager = attachment_manager;

	}

	Renderpass_Filter::~Renderpass_Filter()
	{
	}

	void Renderpass_Filter::prepare()
	{
		//Create the Framebuffer with all needed attachments


		// Set up separate renderpass with references to the color and depth attachments
		//std::array<VkAttachmentDescription, 4> attachmentDescs = {};


	}

}
