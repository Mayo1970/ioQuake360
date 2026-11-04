#include "xsdk/log.h"

#ifdef X360_DEBUG
#define Q360_BUILD_TYPE "debug"
#else
#define Q360_BUILD_TYPE "release"
#endif

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	xsdk_log_write("ioQuake360 stub: " Q360_TITLE " " Q360_BUILD_TYPE " build");
	return 0;
}
