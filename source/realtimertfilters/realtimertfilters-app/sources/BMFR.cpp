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
		Prepass->ConfigureShader("filter/postprocess_tempAccu.frag.spv");
		Prepass->PushAttachment(AttachmentBinding(Attachment::position, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Prepass->PushAttachment(AttachmentBinding(Attachment::normal, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Prepass->PushAttachment(AttachmentBinding(Attachment::motionvector, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Prepass->PushAttachment(AttachmentBinding(Attachment::rtoutput, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Prepass->PushAttachment(AttachmentBinding(Attachment::prev_accumulatedcolor, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Prepass->PushAttachment(AttachmentBinding(Attachment::intermediate, AttachmentBinding::AccessMode::WriteOnly, AttachmentBinding::BindType::Sampled));
		Prepass->PushAttachment(AttachmentBinding(Attachment::prev_position, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Prepass->PushAttachment(AttachmentBinding(Attachment::prev_normal, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Prepass->PushAttachment(AttachmentBinding(Attachment::prev_historylength, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Prepass->PushAttachment(AttachmentBinding(Attachment::new_historylength, AttachmentBinding::AccessMode::WriteOnly, AttachmentBinding::BindType::Sampled));
		Prepass->PushUBO(std::dynamic_pointer_cast<UBOInterface, ManagedUBO<S_AccuConfig>>(rtfilterdemo->m_UBO_AccuConfig));
		Prepass->Push_PastRenderpass_BufferCopy(Attachment::position, Attachment::prev_position);
		Prepass->Push_PastRenderpass_BufferCopy(Attachment::normal, Attachment::prev_normal);
		Prepass->Push_PastRenderpass_BufferCopy(Attachment::new_historylength, Attachment::prev_historylength);
		Prepass->Push_PastRenderpass_BufferCopy(Attachment::intermediate, Attachment::prev_accumulatedcolor);
		renderpassManager->registerRenderpass(Prepass);

		Computepass = std::make_shared<RenderpassBMFRCompute>();
		renderpassManager->registerRenderpass(std::dynamic_pointer_cast<Renderpass, RenderpassBMFRCompute>(Computepass));

		Postpass = std::make_shared<RenderpassPostProcess>();
		Postpass->ConfigureShader("filter/postprocess_tempAccu.frag.spv");
		Postpass->PushAttachment(AttachmentBinding(Attachment::position, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Postpass->PushAttachment(AttachmentBinding(Attachment::normal, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Postpass->PushAttachment(AttachmentBinding(Attachment::motionvector, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Postpass->PushAttachment(AttachmentBinding(Attachment::rtoutput, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Postpass->PushAttachment(AttachmentBinding(Attachment::prev_accumulatedcolor, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Postpass->PushAttachment(AttachmentBinding(Attachment::intermediate, AttachmentBinding::AccessMode::WriteOnly, AttachmentBinding::BindType::Sampled));
		Postpass->PushAttachment(AttachmentBinding(Attachment::prev_position, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Postpass->PushAttachment(AttachmentBinding(Attachment::prev_normal, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Postpass->PushAttachment(AttachmentBinding(Attachment::prev_historylength, AttachmentBinding::AccessMode::ReadOnly, AttachmentBinding::BindType::Sampled));
		Postpass->PushAttachment(AttachmentBinding(Attachment::new_historylength, AttachmentBinding::AccessMode::WriteOnly, AttachmentBinding::BindType::Sampled));
		Postpass->PushUBO(std::dynamic_pointer_cast<UBOInterface, ManagedUBO<S_AccuConfig>>(rtfilterdemo->m_UBO_AccuConfig));
		Postpass->Push_PastRenderpass_BufferCopy(Attachment::position, Attachment::prev_position);
		Postpass->Push_PastRenderpass_BufferCopy(Attachment::normal, Attachment::prev_normal);
		Postpass->Push_PastRenderpass_BufferCopy(Attachment::new_historylength, Attachment::prev_historylength);
		Postpass->Push_PastRenderpass_BufferCopy(Attachment::intermediate, Attachment::prev_accumulatedcolor);
		renderpassManager->registerRenderpass(Postpass);
	}

}