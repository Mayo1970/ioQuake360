#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"
#include "xsdk/log.h"

#ifdef X360_DEBUG
#define Q360_BUILD_TYPE "debug"
#else
#define Q360_BUILD_TYPE "release"
#endif

int main(void)
{
	char cmdline[MAX_STRING_CHARS];

	xsdk_log_write("ioQuake360: " Q360_TITLE " " Q360_BUILD_TYPE " build");
	Sys_PlatformInit();
	Q_strncpyz(cmdline, "+set net_enabled 0", sizeof(cmdline));
	if (Q360_ARGS[0])
	{
		Q_strcat(cmdline, sizeof(cmdline), " ");
		Q_strcat(cmdline, sizeof(cmdline), Q360_ARGS);
	}
	Com_Init(cmdline);
	NET_Init();

	while (1)
		Com_Frame();
}
