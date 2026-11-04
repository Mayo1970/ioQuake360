// Quake3e types, macros and helpers renderervk uses and ioq3 lacks (docs/RENDERER.md).
#ifndef TR_IOQ3_H
#define TR_IOQ3_H

#include <stdint.h>

typedef union {
	byte rgba[4];
	uint32_t u32;
} color4ub_t;

typedef enum {
	REF_KEEP_CONTEXT, // don't destroy window and context
	REF_KEEP_WINDOW,  // destroy context, keep window
	REF_DESTROY_WINDOW,
	REF_UNLOAD_DLL
} refShutdownCode_t;

typedef enum {
	TK_GENERIC = 0, // for single-char tokens
	TK_STRING,
	TK_QUOTED,
	TK_EQ,
	TK_NEQ,
	TK_GT,
	TK_GTE,
	TK_LT,
	TK_LTE,
	TK_MATCH,
	TK_OR,
	TK_AND,
	TK_SCOPE_OPEN,
	TK_SCOPE_CLOSE,
	TK_NEWLINE,
	TK_EOF,
} tokenType_t;

typedef enum {
	CV_NONE = 0,
	CV_FLOAT,
	CV_INTEGER,
	CV_FSPATH,
	CV_MAX,
} cvarValidator_t;

typedef enum {
	CVG_NONE = 0,
	CVG_RENDERER,
	CVG_SERVER,
	CVG_MAX,
} cvarGroup_t;

extern refimport_t ri;

#define QALIGN(x) __attribute__((aligned(x)))
#define	MAX_UINT			((unsigned)(~0))
#define MAX_VIDEO_HANDLES	16
#define	CONTENTS_NODE           0xFFFFFFFF
#define VIS_HEADER			8
// ioq3 has neither flag: developer cvars become plain ones, and ioq3 archives defaults anyway.
#define CVAR_DEVELOPER		0
#define CVAR_ARCHIVE_ND		CVAR_ARCHIVE
#define SGN(x) (((x) >= 0) ? !!(x) : -1)
#define Vector4Set(v,x,y,z,w)	((v)[0]=(x), (v)[1]=(y), (v)[2]=(z), v[3]=(w))
#define DotProduct4(a,b)		((a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]*(b)[2] + (a)[3]*(b)[3])
#define VectorScale4(a,b,c)		((c)[0]=(a)[0]*(b),(c)[1]=(a)[1]*(b),(c)[2]=(a)[2]*(b),(c)[3]=(a)[3]*(b))
#define	myftol(x) ((int)(x))

// tr_local.h sets its own 12-bit entity numbers; ioq3's tr_types.h values only bind the client.
#undef REFENTITYNUM_BITS
#undef REFENTITYNUM_MASK
#undef MAX_REFENTITIES
#undef REFENTITYNUM_WORLD

static ID_INLINE unsigned int log2pad( unsigned int v, int roundup )
{
	unsigned int x = 1;

	while ( x < v ) x <<= 1;

	if ( roundup == 0 ) {
		if ( x > v ) {
			x >>= 1;
		}
	}

	return x;
}

// tr_ioq3.c holds Quake3e's copies; the TR_ names keep them apart from the engine's ioq3 functions.
#define Q_atof TR_Q_atof
#define Q_stradd TR_Q_stradd
#define Com_Split TR_Com_Split
#define Com_GenerateHashValue TR_Com_GenerateHashValue
#define crc32_buffer TR_crc32_buffer
#define COM_BeginParseSession TR_COM_BeginParseSession
#define COM_GetCurrentParseLine TR_COM_GetCurrentParseLine
#define COM_Parse TR_COM_Parse
#define COM_ParseExt TR_COM_ParseExt
#define COM_ParseComplex TR_COM_ParseComplex
#define SkipBracedSection TR_SkipBracedSection
#define SkipRestOfLine TR_SkipRestOfLine
#define com_tokentype tr_com_tokentype

extern tokenType_t com_tokentype;

float Q_atof( const char *str );
char *Q_stradd( char *dst, const char *src );
int Com_Split( char *in, char **out, int outsz, int delim );
unsigned long Com_GenerateHashValue( const char *fname, const unsigned int size );
unsigned int crc32_buffer( const byte *buf, unsigned int len );
void COM_BeginParseSession( const char *name );
int COM_GetCurrentParseLine( void );
const char *COM_Parse( const char **data_p );
const char *COM_ParseExt( const char **data_p, qboolean allowLineBreaks );
char *COM_ParseComplex( const char **data_p, qboolean allowLineBreaks );
qboolean SkipBracedSection( const char **program, int depth );
void SkipRestOfLine( const char **data );

// Stand-ins for Quake3e refimport_t fields that ioq3's refimport_t does not have.
void TR_FreeAll( void );
void TR_CvarSetGroup( cvar_t *var, cvarGroup_t group );
int TR_CvarCheckGroup( cvarGroup_t group );
void TR_CvarResetGroup( cvarGroup_t group, qboolean resetModifiedFlags );
void TR_CvarCheckRange( cvar_t *cv, const char *minVal, const char *maxVal, cvarValidator_t type );
qboolean TR_CL_IsMinimized( void );
void TR_CL_SetScaling( float factor, int captureWidth, int captureHeight );
void TR_Sys_SetClipboardBitmap( const byte *bitmap, int size );

#endif
