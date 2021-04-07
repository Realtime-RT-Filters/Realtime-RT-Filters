#ifndef RTFilterDemo_h
#define RTFilterDemo_h

#include <VulkanRaytracingSample.h>

namespace rtf
{
	class RTFilterDemo : public VulkanRaytracingSample
	{
	public:
		RTFilterDemo();

		virtual void render() override;
	};
}

#endif RTFilterDemo_h