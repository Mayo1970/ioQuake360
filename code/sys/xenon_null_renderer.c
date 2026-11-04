#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_public.h"

#include <string.h>

static refexport_t nullRe;

static void Null_Shutdown(qboolean destroyWindow) { (void)destroyWindow; }
static void Null_BeginRegistration(glconfig_t *config)
{
	if (config)
	{
		memset(config, 0, sizeof(*config));
		config->vidWidth = 640;
		config->vidHeight = 480;
		config->windowAspect = 640.0f / 480.0f;
		config->isFullscreen = qtrue;
	}
}
// Non-zero: cgame drops the server when the default player model fails to register.
static qhandle_t Null_Register(const char *name) { (void)name; return 1; }
static void Null_LoadWorld(const char *name) { (void)name; }
static void Null_SetWorldVisData(const byte *vis) { (void)vis; }
static void Null_Void(void) { }
static void Null_AddRefEntity(const refEntity_t *entity) { (void)entity; }
static void Null_AddPoly(qhandle_t shader, int numVerts, const polyVert_t *verts, int num)
{
	(void)shader; (void)numVerts; (void)verts; (void)num;
}
static int Null_LightForPoint(vec3_t point, vec3_t ambient, vec3_t directed, vec3_t lightDir)
{
	(void)point;
	VectorClear(ambient);
	VectorClear(directed);
	VectorClear(lightDir);
	return 0;
}
static void Null_AddLight(const vec3_t org, float intensity, float r, float g, float b)
{
	(void)org; (void)intensity; (void)r; (void)g; (void)b;
}
static void Null_RenderScene(const refdef_t *fd) { (void)fd; }
static void Null_SetColor(const float *rgba) { (void)rgba; }
static void Null_DrawStretchPic(float x, float y, float w, float h, float s1, float t1,
	float s2, float t2, qhandle_t shader)
{
	(void)x; (void)y; (void)w; (void)h; (void)s1; (void)t1; (void)s2; (void)t2; (void)shader;
}
static void Null_DrawStretchRaw(int x, int y, int w, int h, int cols, int rows,
	const byte *data, int client, qboolean dirty)
{
	(void)x; (void)y; (void)w; (void)h; (void)cols; (void)rows; (void)data; (void)client; (void)dirty;
}
static void Null_UploadCinematic(int w, int h, int cols, int rows, const byte *data,
	int client, qboolean dirty)
{
	(void)w; (void)h; (void)cols; (void)rows; (void)data; (void)client; (void)dirty;
}
static void Null_BeginFrame(stereoFrame_t stereoFrame) { (void)stereoFrame; }
static void Null_EndFrame(int *frontEndMsec, int *backEndMsec)
{
	if (frontEndMsec) *frontEndMsec = 0;
	if (backEndMsec) *backEndMsec = 0;
}
static int Null_MarkFragments(int numPoints, const vec3_t *points, const vec3_t projection,
	int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer)
{
	(void)numPoints; (void)points; (void)projection; (void)maxPoints;
	(void)pointBuffer; (void)maxFragments; (void)fragmentBuffer;
	return 0;
}
static int Null_LerpTag(orientation_t *tag, qhandle_t model, int startFrame, int endFrame,
	float frac, const char *tagName)
{
	(void)model; (void)startFrame; (void)endFrame; (void)frac; (void)tagName;
	if (tag)
	{
		memset(tag, 0, sizeof(*tag));
		tag->axis[0][0] = 1.0f;
		tag->axis[1][1] = 1.0f;
		tag->axis[2][2] = 1.0f;
	}
	return 1;
}
static void Null_ModelBounds(qhandle_t model, vec3_t mins, vec3_t maxs)
{
	(void)model;
	VectorClear(mins);
	VectorClear(maxs);
}
#ifdef __USEA3D
static void Null_A3DRenderGeometry(void *a, void *b, void *c, void *d)
{
	(void)a; (void)b; (void)c; (void)d;
}
#endif
static void Null_RegisterFont(const char *name, int pointSize, fontInfo_t *font)
{
	(void)name; (void)pointSize;
	if (font) memset(font, 0, sizeof(*font));
}
static void Null_RemapShader(const char *oldShader, const char *newShader, const char *offsetTime)
{
	(void)oldShader; (void)newShader; (void)offsetTime;
}
static qboolean Null_GetEntityToken(char *buffer, int size)
{
	(void)buffer; (void)size;
	return qfalse;
}
static qboolean Null_InPVS(const vec3_t p1, const vec3_t p2) { (void)p1; (void)p2; return qtrue; }
static void Null_TakeVideoFrame(int h, int w, byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg)
{
	(void)h; (void)w; (void)captureBuffer; (void)encodeBuffer; (void)motionJpeg;
}

refexport_t *GetRefAPI(int apiVersion, refimport_t *rimp)
{
	(void)rimp;
	if (apiVersion != REF_API_VERSION)
		return NULL;

	memset(&nullRe, 0, sizeof(nullRe));
	nullRe.Shutdown = Null_Shutdown;
	nullRe.BeginRegistration = Null_BeginRegistration;
	nullRe.RegisterModel = Null_Register;
	nullRe.RegisterSkin = Null_Register;
	nullRe.RegisterShader = Null_Register;
	nullRe.RegisterShaderNoMip = Null_Register;
	nullRe.LoadWorld = Null_LoadWorld;
	nullRe.SetWorldVisData = Null_SetWorldVisData;
	nullRe.EndRegistration = Null_Void;
	nullRe.ClearScene = Null_Void;
	nullRe.AddRefEntityToScene = Null_AddRefEntity;
	nullRe.AddPolyToScene = Null_AddPoly;
	nullRe.LightForPoint = Null_LightForPoint;
	nullRe.AddLightToScene = Null_AddLight;
	nullRe.AddAdditiveLightToScene = Null_AddLight;
	nullRe.RenderScene = Null_RenderScene;
	nullRe.SetColor = Null_SetColor;
	nullRe.DrawStretchPic = Null_DrawStretchPic;
	nullRe.DrawStretchRaw = Null_DrawStretchRaw;
	nullRe.UploadCinematic = Null_UploadCinematic;
	nullRe.BeginFrame = Null_BeginFrame;
	nullRe.EndFrame = Null_EndFrame;
	nullRe.MarkFragments = Null_MarkFragments;
	nullRe.LerpTag = Null_LerpTag;
	nullRe.ModelBounds = Null_ModelBounds;
#ifdef __USEA3D
	nullRe.A3D_RenderGeometry = Null_A3DRenderGeometry;
#endif
	nullRe.RegisterFont = Null_RegisterFont;
	nullRe.RemapShader = Null_RemapShader;
	nullRe.GetEntityToken = Null_GetEntityToken;
	nullRe.inPVS = Null_InPVS;
	nullRe.TakeVideoFrame = Null_TakeVideoFrame;
	return &nullRe;
}
