#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_public.h"

#ifdef Q360_RENDERER_VK
#include <vulkan/vulkan.h>
#endif

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
	Com_Printf( "Vulkan platform initialized at 1280x720\n" );
}

void VKimp_Shutdown( qboolean unloadDLL )
{
	(void)unloadDLL;
}

void *VK_GetInstanceProcAddr( VkInstance instance, const char *name )
{
#ifdef Q360_RENDERER_VK
	return (void *)vkGetInstanceProcAddr( instance, name );
#else
	(void)instance;
	(void)name;
	return NULL;
#endif
}

qboolean VK_CreateSurface( VkInstance instance, VkSurfaceKHR *pSurface )
{
#ifdef Q360_RENDERER_VK
	VkPhysicalDevice physical_device;
	VkDisplayPropertiesKHR display;
	VkDisplayModePropertiesKHR mode;
	VkDisplaySurfaceCreateInfoKHR surface_info;
	VkResult result;
	uint32_t count;

	*pSurface = VK_NULL_HANDLE;
	count = 0;
	result = vkEnumeratePhysicalDevices( instance, &count, NULL );
	if ( result != VK_SUCCESS || count == 0 ) {
		if ( result == VK_SUCCESS )
			result = VK_ERROR_INITIALIZATION_FAILED;
		Com_Printf( "Vulkan: vkEnumeratePhysicalDevices failed with VkResult %d\n", result );
		return qfalse;
	}

	count = 1;
	result = vkEnumeratePhysicalDevices( instance, &count, &physical_device );
	if ( result != VK_SUCCESS ) {
		Com_Printf( "Vulkan: vkEnumeratePhysicalDevices failed with VkResult %d\n", result );
		return qfalse;
	}

	count = 1;
	result = vkGetPhysicalDeviceDisplayPropertiesKHR( physical_device, &count, &display );
	if ( result != VK_SUCCESS ) {
		Com_Printf( "Vulkan: vkGetPhysicalDeviceDisplayPropertiesKHR failed with VkResult %d\n", result );
		return qfalse;
	}

	count = 1;
	result = vkGetDisplayModePropertiesKHR( physical_device, display.display, &count, &mode );
	if ( result != VK_SUCCESS ) {
		Com_Printf( "Vulkan: vkGetDisplayModePropertiesKHR failed with VkResult %d\n", result );
		return qfalse;
	}

	Com_Memset( &surface_info, 0, sizeof( surface_info ) );
	surface_info.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	surface_info.displayMode = mode.displayMode;
	surface_info.planeIndex = 0;
	surface_info.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	surface_info.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
	surface_info.imageExtent = mode.parameters.visibleRegion;
	result = vkCreateDisplayPlaneSurfaceKHR( instance, &surface_info, NULL, pSurface );
	if ( result != VK_SUCCESS ) {
		Com_Printf( "Vulkan: vkCreateDisplayPlaneSurfaceKHR failed with VkResult %d\n", result );
		return qfalse;
	}

	Com_Printf( "Vulkan display: %s, %ux%u at %u mHz\n", display.displayName,
		(unsigned)mode.parameters.visibleRegion.width, (unsigned)mode.parameters.visibleRegion.height,
		(unsigned)mode.parameters.refreshRate );
	return qtrue;
#else
	(void)instance;
	(void)pSurface;
	return qfalse;
#endif
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
