#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"
#include "xsdk/log.h"
#include "xsdk/kernel.h"

#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static char binaryPath[MAX_OSPATH] = "game:";
static char installPath[MAX_OSPATH];

void Sys_SetBinaryPath( const char *path )
{
	Q_strncpyz( binaryPath, path ? path : "game:", sizeof( binaryPath ) );
}

char *Sys_BinaryPath( void )
{
	return binaryPath;
}

void Sys_SetDefaultInstallPath( const char *path )
{
	Q_strncpyz( installPath, path ? path : "", sizeof( installPath ) );
}

char *Sys_DefaultInstallPath( void )
{
	return *installPath ? installPath : Sys_BinaryPath();
}

char *Sys_DefaultAppPath( void )
{
	return Sys_BinaryPath();
}

char *Sys_DefaultHomeConfigPath( void )
{
	return "game:";
}

char *Sys_DefaultHomeDataPath( void )
{
	return "game:";
}

char *Sys_DefaultHomeStatePath( void )
{
	return "game:";
}

char *Sys_SteamPath( void )
{
	return "";
}

char *Sys_GogPath( void )
{
	return "";
}

char *Sys_MicrosoftStorePath( void )
{
	return "";
}

char *Sys_Cwd( void )
{
	static char cwd[MAX_OSPATH];

	return getcwd( cwd, sizeof( cwd ) );
}

static qboolean Sys_IsPathSeparator( char c )
{
	return c == '/' || c == '\\';
}

const char *Sys_Basename( char *path )
{
	static char dot[] = ".";
	static char root[] = "/";
	char *end;
	char *base;

	if( !path || !*path )
		return dot;

	end = path + strlen( path );
	while( end > path && Sys_IsPathSeparator( end[-1] ) )
		end--;
	if( end == path )
		return root;

	*end = '\0';
	base = end;
	while( base > path && !Sys_IsPathSeparator( base[-1] ) )
		base--;

	return base;
}

const char *Sys_Dirname( char *path )
{
	static char dot[] = ".";
	static char root[] = "/";
	size_t length;
	size_t originalLength;

	if( !path || !*path )
		return dot;

	originalLength = strlen( path );
	length = originalLength;
	while( length && Sys_IsPathSeparator( path[length - 1] ) )
		length--;
	if( !length )
		return root;

	path[length] = '\0';
	if( length < originalLength && path[length - 1] == ':' )
		return path;

	while( length && !Sys_IsPathSeparator( path[length - 1] ) )
		length--;
	if( !length )
		return dot;

	while( length > 1 && Sys_IsPathSeparator( path[length - 1] ) )
		length--;
	path[length] = '\0';
	return path;
}

int Sys_Milliseconds( void )
{
	static int64_t timeBase = -1;
	struct timespec now;
	int64_t milliseconds;

	// The SDK's monotonic clock counts the CPU time base at 49.875 MHz, not the 50 MHz the kernel reports
	// (x360-sdk docs/HARDWARE.md).
	if( clock_gettime( CLOCK_MONOTONIC, &now ) != 0 )
		return 0;

	if( timeBase < 0 )
		timeBase = (int64_t)now.tv_sec;

	milliseconds = ((int64_t)now.tv_sec - timeBase) * 1000 + now.tv_nsec / 1000000;
	return (int)milliseconds;
}

// newlib's clock() calls times(), which the SDK lacks; botlib times itself with clock().
clock_t times( struct tms *buf )
{
	clock_t ticks = (clock_t)( (int64_t)Sys_Milliseconds() * CLOCKS_PER_SEC / 1000 );

	if( buf )
	{
		buf->tms_utime = ticks;
		buf->tms_stime = 0;
		buf->tms_cutime = 0;
		buf->tms_cstime = 0;
	}
	return ticks;
}

void Sys_Sleep( int msec )
{
	if( msec > 0 )
	{
		struct timespec delay;

		delay.tv_sec = msec / 1000;
		delay.tv_nsec = (long)( msec % 1000 ) * 1000000L;
		nanosleep( &delay, NULL );
	}
	else
	{
		(void)NtYieldExecution();
	}
}

FILE *Sys_FOpen( const char *ospath, const char *mode )
{
	struct stat info;

	if( !stat( ospath, &info ) && S_ISDIR( info.st_mode ) )
		return NULL;

	return fopen( ospath, mode );
}

qboolean Sys_Mkdir( const char *path )
{
	if( mkdir( path, 0750 ) != 0 )
		return errno == EEXIST;

	return qtrue;
}

FILE *Sys_Mkfifo( const char *ospath )
{
	(void)ospath;
	return NULL;
}

#define MAX_FOUND_FILES 0x1000

void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **list, int *numfiles )
{
	char search[MAX_OSPATH];
	char newsubdirs[MAX_OSPATH];
	char filename[MAX_OSPATH];
	DIR *fdir;
	struct dirent *entry;
	struct stat info;

	if( *numfiles >= MAX_FOUND_FILES - 1 || !basedir[0] )
		return;

	if( *subdirs )
		Com_sprintf( search, sizeof( search ), "%s/%s", basedir, subdirs );
	else
		Com_sprintf( search, sizeof( search ), "%s", basedir );

	fdir = opendir( search );
	if( !fdir )
		return;

	while( ( entry = readdir( fdir ) ) != NULL )
	{
		Com_sprintf( filename, sizeof( filename ), "%s/%s", search, entry->d_name );
		if( stat( filename, &info ) == -1 )
			continue;

		if( S_ISDIR( info.st_mode ) && Q_stricmp( entry->d_name, "." ) &&
			Q_stricmp( entry->d_name, ".." ) )
		{
			if( *subdirs )
				Com_sprintf( newsubdirs, sizeof( newsubdirs ), "%s/%s", subdirs, entry->d_name );
			else
				Com_sprintf( newsubdirs, sizeof( newsubdirs ), "%s", entry->d_name );
			Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
		}

		if( *numfiles >= MAX_FOUND_FILES - 1 )
			break;

		Com_sprintf( filename, sizeof( filename ), "%s/%s", subdirs, entry->d_name );
		if( !Com_FilterPath( filter, filename, qfalse ) )
			continue;

		list[*numfiles] = CopyString( filename );
		(*numfiles)++;
	}

	closedir( fdir );
}

char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs )
{
	struct dirent *entry;
	DIR *fdir;
	qboolean dironly = wantsubs;
	char search[MAX_OSPATH];
	int nfiles;
	char **listCopy;
	char *list[MAX_FOUND_FILES];
	int i;
	struct stat info;
	int extLen;

	if( filter )
	{
		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );
		list[nfiles] = NULL;
		*numfiles = nfiles;
		if( !nfiles )
			return NULL;

		listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
		for( i = 0; i < nfiles; i++ )
			listCopy[i] = list[i];
		listCopy[i] = NULL;
		return listCopy;
	}

	if( !directory[0] )
	{
		*numfiles = 0;
		return NULL;
	}

	if( !extension )
		extension = "";
	if( extension[0] == '/' && extension[1] == '\0' )
	{
		extension = "";
		dironly = qtrue;
	}
	extLen = strlen( extension );
	nfiles = 0;

	fdir = opendir( directory );
	if( !fdir )
	{
		*numfiles = 0;
		return NULL;
	}

	while( ( entry = readdir( fdir ) ) != NULL )
	{
		Com_sprintf( search, sizeof( search ), "%s/%s", directory, entry->d_name );
		if( stat( search, &info ) == -1 )
			continue;
		if( ( dironly && !S_ISDIR( info.st_mode ) ) ||
			( !dironly && S_ISDIR( info.st_mode ) ) )
			continue;

		if( *extension && ( strlen( entry->d_name ) < (size_t)extLen ||
			Q_stricmp( entry->d_name + strlen( entry->d_name ) - extLen, extension ) ) )
			continue;

		if( nfiles == MAX_FOUND_FILES - 1 )
			break;
		list[nfiles++] = CopyString( entry->d_name );
	}

	list[nfiles] = NULL;
	closedir( fdir );
	*numfiles = nfiles;
	if( !nfiles )
		return NULL;

	listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
	for( i = 0; i < nfiles; i++ )
		listCopy[i] = list[i];
	listCopy[i] = NULL;
	return listCopy;
}

void Sys_FreeFileList( char **list )
{
	int i;

	if( !list )
		return;

	for( i = 0; list[i]; i++ )
		Z_Free( list[i] );
	Z_Free( list );
}

void Sys_Print( const char *msg )
{
	xsdk_log_write_bytes( msg, strlen( msg ) );
}

void Sys_AnsiColorPrint( const char *msg )
{
	char buffer[MAXPRINTMSG];
	size_t length = 0;

	while( *msg )
	{
		if( Q_IsColorString( msg ) )
		{
			msg += 2;
			continue;
		}

		buffer[length++] = *msg++;
		if( length == sizeof( buffer ) - 1 )
		{
			buffer[length] = '\0';
			Sys_Print( buffer );
			length = 0;
		}
	}

	if( length )
	{
		buffer[length] = '\0';
		Sys_Print( buffer );
	}
}

void Sys_ErrorDialog( const char *error )
{
	xsdk_log_write( "Sys_Error: %s", error );
}

// The SDK's exit() closes the log and returns to the dashboard through XamLoaderTerminateTitle.
static Q_NO_RETURN void Sys_Exit( int exitCode )
{
	NET_Shutdown();
	Sys_PlatformExit();
	exit( exitCode );
}

void Sys_Error( const char *error, ... )
{
	va_list argptr;
	char text[MAXPRINTMSG];

	va_start( argptr, error );
	Q_vsnprintf( text, sizeof( text ), error, argptr );
	va_end( argptr );

	Sys_ErrorDialog( text );
	Sys_Exit( 3 );
}

void Sys_Quit( void )
{
	Sys_Exit( 0 );
}

void Sys_SigHandler( int signal )
{
	static qboolean signalcaught = qfalse;

	if( signalcaught )
		xsdk_log_write( "DOUBLE SIGNAL FAULT: Received signal %d, exiting...", signal );
	else
	{
		signalcaught = qtrue;
		xsdk_log_write( "Received signal %d, exiting...", signal );
		VM_Forced_Unload_Start();
#ifndef DEDICATED
		CL_Shutdown( va( "Received signal %d", signal ), qtrue, qtrue );
#endif
		SV_Shutdown( va( "Received signal %d", signal ) );
		VM_Forced_Unload_Done();
	}

	Sys_Exit( signal == SIGTERM || signal == SIGINT ? 1 : 2 );
}

dialogResult_t Sys_Dialog( dialogType_t type, const char *message, const char *title )
{
	xsdk_log_write( "Dialog (%d) %s: %s", (int)type, title ? title : "", message ? message : "" );
	return DR_OK;
}

qboolean Sys_OpenFolderInFileManager( const char *path, qboolean create )
{
	(void)path;
	(void)create;
	return qfalse;
}

qboolean Sys_OpenFolderInPlatformFileManager( const char *path )
{
	(void)path;
	return qfalse;
}

char *Sys_GetClipboardData( void )
{
	return NULL;
}

char *Sys_ConsoleInput( void )
{
	return NULL;
}

void Sys_InitPIDFile( const char *gamedir )
{
	(void)gamedir;
}

void Sys_RemovePIDFile( const char *gamedir )
{
	(void)gamedir;
}

void Sys_Init( void )
{
	Cvar_Set( "arch", OS_STRING " " ARCH_STRING );
	Cvar_Set( "username", Sys_GetCurrentUser() );
}

char *Sys_GetCurrentUser( void )
{
	return "player";
}

cpuFeatures_t Sys_GetProcessorFeatures( void )
{
	return CF_ALTIVEC;
}

qboolean Sys_RandomBytes( byte *string, int len )
{
	if( len < 0 || ( !string && len ) )
		return qfalse;
	if( len )
		XeCryptRandom( string, (uint32_t)len );
	return qtrue;
}

qboolean Sys_LowPhysicalMemory( void )
{
	return qfalse;
}

void Sys_SetEnv( const char *name, const char *value )
{
	if( value && *value )
		setenv( name, value, 1 );
	else
		unsetenv( name );
}

void Sys_PlatformInit( void )
{
}

void Sys_PlatformExit( void )
{
}

void Sys_GLimpInit( void )
{
}

void Sys_GLimpSafeInit( void )
{
}

qboolean Sys_DllExtension( const char *name )
{
	return name && COM_CompareExtension( name, DLL_EXT );
}

void * QDECL Sys_LoadDll( const char *name, qboolean useSystemLib )
{
	(void)name;
	(void)useSystemLib;
	return NULL;
}

void * QDECL Sys_LoadGameDll( const char *name, vmMainProc *entryPoint,
	intptr_t (QDECL *systemcalls)(intptr_t, ...) )
{
	Com_DPrintf( "Sys_LoadGameDll(%s): no dynamic libraries on xbox360\n", name ? name : "(null)" );
	(void)entryPoint;
	(void)systemcalls;
	return NULL;
}

void Sys_UnloadDll( void *dllHandle )
{
	(void)dllHandle;
}
