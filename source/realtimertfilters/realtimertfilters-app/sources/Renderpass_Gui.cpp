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
		//Get all the needed attachments from the attachment manager
		m_position = m_attachment_manager->getAttachment(Attachment::position);
		m_normal = m_attachment_manager->getAttachment(Attachment::normal);
		m_albedo = m_attachment_manager->getAttachment(Attachment::albedo);
		m_motionvector = m_attachment_manager->getAttachment(Attachment::motionvector);
		m_rtouput = m_attachment_manager->getAttachment(Attachment::rtoutput);
		m_filteroutput = m_attachment_manager->getAttachment(Attachment::filteroutput);


		// Set up separate renderpass with references to the color and depth attachments
		//std::array<VkAttachmentDescription, 4> attachmentDescs = {};


	}

}
