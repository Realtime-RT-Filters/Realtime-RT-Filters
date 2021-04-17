#ifndef RTFilterDemo_h
#define RTFilterDemo_h

#include <VulkanRaytracingSample.h>
#include "VulkanglTFModel.h"

namespace rtf
{
	class RTFilterDemo : public VulkanRaytracingSample
	{
	public:

		struct {
			struct {
				vks::Texture2D colorMap;
				vks::Texture2D normalMap;
			} model;
			struct {
				vks::Texture2D colorMap;
				vks::Texture2D normalMap;
			} floor;
		} textures;

		struct {
			vkglTF::Model model;
			vkglTF::Model floor;
		} models;

		bool m_debugDisplay = true;

		RTFilterDemo();

		virtual void render() override;
		virtual void prepare() override;
		virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay) override;

	protected:
		void loadAssets();
	};
}

#endif RTFilterDemo_h