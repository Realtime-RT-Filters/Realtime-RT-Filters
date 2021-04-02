// realtimertfilters-app.cpp : Defines the entry point for the application.
//

#include "realtimertfilters-app.h"
#include <VulkanRaytracingSample.h>

class Test : VulkanRaytracingSample
{
public:

	// Inherited via VulkanRaytracingSample
	virtual void render() override
	{}
};

#if WIN32
int WINAPI wWinMain(
	_In_		HINSTANCE	hInstance,
	_In_opt_	HINSTANCE	hPrevInstance,
	_In_		LPWSTR		lpCmdLine,
	_In_		int			nShowCmd
)
{
	return 420;
}
#else
int main(
	int		argc, 
	char*	argv[]
)
{
	return 420;
}
#endif

