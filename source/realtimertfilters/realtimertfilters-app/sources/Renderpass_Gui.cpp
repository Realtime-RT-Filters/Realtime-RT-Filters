#include "../headers/Renderpass_Gui.hpp"

namespace rtf
{

	Renderpass_Gui::Renderpass_Gui(VkDevice* device, Attachment_Manager* attachment_manager)
	{

		m_attachment_manager = attachment_manager;

	}

	Renderpass_Gui::~Renderpass_Gui()
	{
	}

	void Renderpass_Gui::prepare()
	{
		//Create the Framebuffer with all needed attachments


		// Set up separate renderpass with references to the color and depth attachments
		//std::array<VkAttachmentDescription, 4> attachmentDescs = {};


	}

}
