#ifndef RTFilterDemo_h
#define RTFilterDemo_h

#include <VulkanRaytracingSample.h>

namespace rtf
{
	class RTFilterDemo : public VulkanRaytracingSample
	{
	public:

		bool m_debugDisplay = true;

		RTFilterDemo();

		virtual void render() override;
		virtual void prepare() override;
		virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay) override;
	};
}

#endif RTFilterDemo_h