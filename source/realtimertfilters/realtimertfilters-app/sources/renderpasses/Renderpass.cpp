#include "../../headers/renderpasses/Renderpass.hpp"
#include "../../headers/RTFilterDemo.hpp"

namespace rtf
{
	void Renderpass::setRtFilterDemo(RTFilterDemo* rtFilterDemo)
	{
		m_rtFilterDemo = rtFilterDemo;
		m_vulkanDevice = rtFilterDemo->vulkanDevice;
		m_attachmentManager = rtFilterDemo->m_attachmentManager;
		m_rtFilterDemo = rtFilterDemo;
	}
}