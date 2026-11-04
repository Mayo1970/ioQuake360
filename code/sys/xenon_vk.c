#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_public.h"

void VKimp_Init( glconfig_t *config )
{
	config->vidWidth = 1280;
	config->vidHeight = 720;
	config->colorBits = 32;
	config->depthBits = 24;
	config->stencilBits = 8;
	config->windowAspect = 16.0f / 9.0f;
	config->isFullscreen = qtrue;
	config->deviceSupportsGamma = qfalse;
	Com_Printf( "Vulkan platform stub initialized at 1280x720\n" );
}

void VKimp_Shutdown( qboolean unloadDLL )
{
	(void)unloadDLL;
}

void *VK_GetInstanceProcAddr( VkInstance instance, const char *name )
{
	(void)instance;
	(void)name;
	return NULL;
}

qboolean VK_CreateSurface( VkInstance instance, VkSurfaceKHR *pSurface )
{
	(void)instance;
	(void)pSurface;
	return qfalse;
}

void GLimp_InitGamma( glconfig_t *config )
{
	config->deviceSupportsGamma = qfalse;
}

void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	(void)red;
	(void)green;
	(void)blue;
}
