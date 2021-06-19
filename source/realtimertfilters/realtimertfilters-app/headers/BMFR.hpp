#ifndef BMFR_h
#define BMFR_h

#include "renderpasses/Renderpass.hpp"

namespace rtf
{
	class RTFilterDemo;
	class RenderpassGbuffer;
	class RenderpassGui;
	class RenderpassPostProcess;
	class RenderpassPathTracer;
}

namespace bmfr
{
	class RenderpassBMFRCompute;

	class RenderPasses
	{
	public:
		std::shared_ptr<rtf::RenderpassPostProcess> Prepass;
		std::shared_ptr<RenderpassBMFRCompute> Computepass;
		std::shared_ptr<rtf::RenderpassPostProcess> Postpass;

		RenderPasses() : Prepass(), Computepass(), Postpass() {}
		void prepareBMFRPasses(rtf::RTFilterDemo* rtfilterdemo);
		void addToQueue(rtf::QueueTemplatePtr& queueTemplate);
		~RenderPasses() = default;
	};

}

#endif