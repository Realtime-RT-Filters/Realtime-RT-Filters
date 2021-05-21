// realtimertfilters-app.cpp : Defines the entry point for the application.
//

#include "realtimertfilters-app.h"
#include <VulkanRaytracingSample.h>
#include "headers/RTFilterDemo.hpp"



rtf::RTFilterDemo* rtFilterDemoInstance;

rtf::RTFilterDemo* GetRTDemoInstance()
{
	return rtFilterDemoInstance;
}

// OS specific macros for the example main entry points
#if defined(_WIN32)
// Windows entry point


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (rtFilterDemoInstance != nullptr)
	{
		rtFilterDemoInstance->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int WinMain(
	_In_		HINSTANCE	hInstance,
	_In_opt_	HINSTANCE	hPrevInstance,
	_In_		LPSTR		lpCmdLine,
	_In_		int			nShowCmd
)
{
	for (int32_t i = 0; i < __argc; i++) { rtf::RTFilterDemo::args.push_back(__argv[i]); };
	rtFilterDemoInstance = new rtf::RTFilterDemo();
	rtFilterDemoInstance->initVulkan();
	rtFilterDemoInstance->setupWindow(hInstance, WndProc);
	rtFilterDemoInstance->prepare();
	rtFilterDemoInstance->renderLoop();
	delete(rtFilterDemoInstance);
	return 0;
}
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
// Android entry point
void android_main(android_app* state)
{
	rtFilterDemoInstance = new rtf::RTFilterDemo();
	state->userData = rtFilterDemoInstance;
	state->onAppCmd = rtf::RTFilterDemo::handleAppCommand;
	state->onInputEvent = rtf::RTFilterDemo::handleAppInput;
	androidApp = state;
	vks::android::getDeviceConfig();
	rtFilterDemoInstance->renderLoop();
	delete(rtFilterDemoInstance);
}
#elif defined(_DIRECT2DISPLAY)
// Linux entry point with direct to display wsi
static void handleEvent()
{}
int main(const int argc, const char* argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
static void handleEvent(const DFBWindowEvent* event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
int main(const int argc, const char* argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
int main(const int argc, const char* argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_XCB_KHR)
static void handleEvent(const xcb_generic_event_t* event)
{
    if (rtFilterDemoInstance != NULL)
	{
        rtFilterDemoInstance->handleEvent(event);
	}
}
int main(const int argc, const char* argv[])
{
    for (size_t i = 0; i < argc; i++) { rtf::RTFilterDemo::args.push_back(argv[i]); };
    rtFilterDemoInstance = new rtf::RTFilterDemo();
    rtFilterDemoInstance->initVulkan();
    rtFilterDemoInstance->setupWindow();
    rtFilterDemoInstance->prepare();
    rtFilterDemoInstance->renderLoop();
    delete(rtFilterDemoInstance);
	return 0;
}
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
#if defined(VK_EXAMPLE_XCODE_GENERATED)
int main(const int argc, const char* argv[])
{
	@autoreleasepool
	{
		for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
		vulkanExample = new VulkanExample();
		vulkanExample->initVulkan();
		vulkanExample->setupWindow(nullptr);
		vulkanExample->prepare();
		vulkanExample->renderLoop();
		delete(vulkanExample);
	}
	return 0;
}
#else
#define VULKAN_EXAMPLE_MAIN()
#endif
#endif
