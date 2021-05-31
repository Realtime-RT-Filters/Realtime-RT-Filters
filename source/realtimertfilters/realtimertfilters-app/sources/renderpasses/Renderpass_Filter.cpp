#include "../../headers/renderpasses/Renderpass_Filter.hpp"

namespace rtf
{

	Renderpass_Filter::Renderpass_Filter()
	{
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

	void Renderpass_Filter::draw(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount)
	{
	}

	void Renderpass_Filter::cleanUp()
	{
	}

}
