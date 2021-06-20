#include "../headers/BMFR.hpp"
#include "../headers/RTFilterDemo.hpp"
#include "../headers/renderpasses/RenderpassManager.hpp"
#include "../headers/renderpasses/Renderpass_PostProcess.hpp"
#include "../headers/renderpasses/Renderpass_BMFRCompute.hpp"

namespace bmfr
{
	using namespace rtf;

	void RenderPasses::prepareBMFRPasses(rtf::RTFilterDemo* rtfilterdemo)
	{
		RenderpassManager* renderpassManager = rtfilterdemo->m_renderpassManager;

		Prepass = std::make_shared<RenderpassPostProcess>();
		Prepass->ConfigureShader("bmfr/bmfrPreProcess.frag.spv");
		Prepass->PushTextureAttachment(TextureBinding(Attachment::position, TextureBinding::Type::Sampler_ReadOnly));
		Prepass->PushTextureAttachment(TextureBinding(Attachment::normal, TextureBinding::Type::Sampler_ReadOnly));
		Prepass->PushTextureAttachment(TextureBinding(Attachment::motionvector, TextureBinding::Type::Sampler_ReadOnly));
		Prepass->PushTextureAttachment(TextureBinding(Attachment::rtoutput, TextureBinding::Type::Sampler_ReadOnly));
		Prepass->PushTextureAttachment(TextureBinding(Attachment::prev_accumulatedcolor, TextureBinding::Type::Sampler_ReadOnly));
		Prepass->PushTextureAttachment(TextureBinding(Attachment::intermediate, TextureBinding::Type::Subpass_Output));
		Prepass->PushTextureAttachment(TextureBinding(Attachment::prev_position, TextureBinding::Type::Sampler_ReadOnly));
		Prepass->PushTextureAttachment(TextureBinding(Attachment::prev_normal, TextureBinding::Type::Sampler_ReadOnly));
		Prepass->PushTextureAttachment(TextureBinding(Attachment::prev_historylength, TextureBinding::Type::Sampler_ReadOnly));
		Prepass->PushTextureAttachment(TextureBinding(Attachment::new_historylength, TextureBinding::Type::Subpass_Output));
		Prepass->PushTextureAttachment(TextureBinding(Attachment::albedo, TextureBinding::Type::Sampler_ReadOnly));
		Prepass->PushUBO(std::dynamic_pointer_cast<UBOInterface, ManagedUBO<S_AccuConfig>>(rtfilterdemo->m_UBO_AccuConfig));
		Prepass->Push_PastRenderpass_BufferCopy(Attachment::position, Attachment::prev_position);
		Prepass->Push_PastRenderpass_BufferCopy(Attachment::normal, Attachment::prev_normal);
		Prepass->Push_PastRenderpass_BufferCopy(Attachment::new_historylength, Attachment::prev_historylength);
		Prepass->Push_PastRenderpass_BufferCopy(Attachment::intermediate, Attachment::prev_accumulatedcolor);
		renderpassManager->registerRenderpass(Prepass);

		Computepass = std::make_shared<RenderpassBMFRCompute>();
		renderpassManager->registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassBMFRCompute>(Computepass));

		Postpass = std::make_shared<RenderpassPostProcess>();
		Postpass->ConfigureShader("bmfr/bmfrPostProcess.frag.spv");
		Postpass->PushTextureAttachment(TextureBinding(Attachment::motionvector, TextureBinding::Type::Sampler_ReadOnly));
		Postpass->PushTextureAttachment(TextureBinding(Attachment::intermediate, TextureBinding::Type::Sampler_ReadOnly));
		Postpass->PushTextureAttachment(TextureBinding(Attachment::prev_accumulatedregression, TextureBinding::Type::Sampler_ReadOnly));
		Postpass->PushTextureAttachment(TextureBinding(Attachment::filteroutput, TextureBinding::Type::Subpass_Output));
		Postpass->PushTextureAttachment(TextureBinding(Attachment::new_historylength, TextureBinding::Type::Sampler_ReadOnly));
		Postpass->PushTextureAttachment(TextureBinding(Attachment::albedo, TextureBinding::Type::Sampler_ReadOnly));
		Postpass->Push_PastRenderpass_BufferCopy(Attachment::filteroutput, Attachment::prev_accumulatedregression);
		Postpass->PushUBO(std::dynamic_pointer_cast<UBOInterface, ManagedUBO<S_AccuConfig>>(rtfilterdemo->m_UBO_AccuConfig));
		//renderpassManager->registerRenderpass(Postpass);
	}

	void RenderPasses::addToQueue(rtf::QueueTemplatePtr& queueTemplate)
	{
		queueTemplate->push_back(Prepass);
		//queueTemplate->push_back(Computepass);
		//queueTemplate->push_back(Postpass);
	}

}