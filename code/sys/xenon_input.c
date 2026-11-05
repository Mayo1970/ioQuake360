/* XInput pad input for Xenon, ported from the PS3 pad logic in ps3_input.c. */

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/keycodes.h"
#include "../client/client.h"
#include "xsdk/kernel.h"
#include "xsdk/xam.h"

extern int Key_GetCatcher(void);
extern void Key_SetBinding(int keynum, const char *binding);
extern char *Key_GetBinding(int keynum);

#define XI_PORT_COUNT         4
#define XI_LEFT_DEADZONE      7849
#define XI_RIGHT_DEADZONE     8689
#define XI_TRIGGER_THRESHOLD  30
#define XI_AXIS_MAX           32767
#define XI_MENU_CURSOR_SPEED  5.0f
#define XI_OSK_TEXT_MAX       256
#define XI_OSK_TITLE_MAX      64
#define XI_ERROR_CANCELLED    1223
#define XI_UI_FOCUS_MS        100
#define XI_TA_ENTER_MS        300
#define XI_XN_SYS_UI          9
#define XI_NOTIFY_MAX_VERSION 10

enum {
	PB_A, PB_B, PB_X, PB_Y,
	PB_LB, PB_RB, PB_LT, PB_RT,
	PB_LS, PB_RS, PB_BACK, PB_START,
	PB_UP, PB_DOWN, PB_LEFT, PB_RIGHT,
	PB_COUNT
};

static const int xi_key_map[PB_COUNT] = {
	K_JOY1, K_JOY2, K_JOY3, K_JOY4,
	K_JOY5, K_JOY6, K_JOY7, K_JOY8,
	K_JOY9, K_JOY10, K_JOY11, K_ESCAPE,
	K_UPARROW, K_DOWNARROW, K_LEFTARROW, K_RIGHTARROW,
};

static int xi_port_connected[XI_PORT_COUNT];
static int xi_active_port = -1;
static int xi_btn_prev[PB_COUNT];
/* Key sent on press, so the release still matches it after the menu state changed. */
static int xi_btn_sent[PB_COUNT];
/* Indexed by axis number: 0 left X, 1 left Y, 3 right Y, 4 right X (PS3 numbering). */
static int xi_axis_prev[5];
static float xi_cursor_accum_x;
static float xi_cursor_accum_y;
static int xi_prev_in_menu;
static HANDLE xi_notify_listener;

static cvar_t *x360_rumbleEnable = NULL;
static cvar_t *x360_rumbleScale  = NULL;
static int     s_rumbleExpiryMs = 0;
static int     s_rumbleActive   = 0;

static XAM_OVERLAPPED xi_osk_ov;
/* Kept apart from the overlapped, which XAM writes until the call is done. */
static uint32_t xi_osk_status;
static uint16_t xi_osk_result[XI_OSK_TEXT_MAX];
static uint16_t xi_osk_title[XI_OSK_TITLE_MAX];
static const uint16_t xi_osk_empty[1] = { 0 };
static qboolean xi_osk_active;
static qboolean xi_osk_resync;
static qboolean xi_osk_auto_submit;
static qboolean xi_osk_prepend_slash;
static qboolean xi_osk_field_clear;
static qboolean xi_osk_enter_on_close;
/* Starts far in the past so XI_UIFieldFocused() is false until the first cursor draw. */
static int xi_ui_cursor_ms = -1000;
#ifdef STANDALONETA
static int xi_ta_enter_ms;
#endif
static qboolean xi_ta_enter_pending;

static void XI_SetDefaultBind(int keynum, const char *binding)
{
	char *existing = Key_GetBinding(keynum);
	if (!existing || !existing[0])
		Key_SetBinding(keynum, binding);
}

static void XI_SendAxis(int axis, int value)
{
	if (value == xi_axis_prev[axis])
		return;
	Com_QueueEvent(0, SE_JOYSTICK_AXIS, axis, value, 0, NULL);
	xi_axis_prev[axis] = value;
}

static void XI_ZeroAxes(void)
{
	XI_SendAxis(0, 0);
	XI_SendAxis(1, 0);
	XI_SendAxis(3, 0);
	XI_SendAxis(4, 0);
}

static void XI_ReleaseHeld(void)
{
	int i;

	for (i = 0; i < PB_COUNT; i++) {
		if (xi_btn_sent[i])
			Com_QueueEvent(0, SE_KEY, xi_btn_sent[i], qfalse, 0, NULL);
		xi_btn_sent[i] = 0;
		xi_btn_prev[i] = 0;
	}
	XI_ZeroAxes();
}

static void XI_ResetState(void)
{
	int i;

	xi_active_port = -1;
	for (i = 0; i < XI_PORT_COUNT; i++)
		xi_port_connected[i] = 0;
	for (i = 0; i < PB_COUNT; i++) {
		xi_btn_prev[i] = 0;
		xi_btn_sent[i] = 0;
	}
	for (i = 0; i < 5; i++)
		xi_axis_prev[i] = 0;
	xi_cursor_accum_x = 0.0f;
	xi_cursor_accum_y = 0.0f;
	xi_prev_in_menu = 0;
	xi_ta_enter_pending = qfalse;
	s_rumbleActive = 0;
	s_rumbleExpiryMs = 0;
}

static void XI_ToggleConsole(void)
{
	Com_QueueEvent(0, SE_KEY, K_CONSOLE, qtrue, 0, NULL);
	Com_QueueEvent(0, SE_KEY, K_CONSOLE, qfalse, 0, NULL);
}

static void XI_TapKey(int key)
{
	Com_QueueEvent(0, SE_KEY, key, qtrue, 0, NULL);
	Com_QueueEvent(0, SE_KEY, key, qfalse, 0, NULL);
}

void XI_NoteUIFieldCursor(void)
{
	xi_ui_cursor_ms = Sys_Milliseconds();
}

static qboolean XI_UIFieldFocused(void)
{
	return Sys_Milliseconds() - xi_ui_cursor_ms <= XI_UI_FOCUS_MS ? qtrue : qfalse;
}

static void XI_AsciiToUtf16(const char *src, uint16_t *dst, int count)
{
	int i;

	/* XAM reads UTF-16 big endian, the PPC order, so no byte swap. */
	for (i = 0; i < count - 1 && src[i]; i++)
		dst[i] = (uint16_t)(unsigned char)src[i];
	dst[i] = 0;
}

static void XI_KeyboardOpen(const char *title, int maxlen, qboolean autoSubmit, qboolean prependSlash,
	qboolean fieldClear, qboolean enterOnClose)
{
	uint32_t ret;

	if (xi_osk_active || xi_active_port < 0)
		return;

	if (maxlen < 1)
		maxlen = 1;
	if (maxlen > XI_OSK_TEXT_MAX - 1)
		maxlen = XI_OSK_TEXT_MAX - 1;

	XI_AsciiToUtf16(title, xi_osk_title, XI_OSK_TITLE_MAX);
	memset(xi_osk_result, 0, sizeof(xi_osk_result));
	memset(&xi_osk_ov, 0, sizeof(xi_osk_ov));

	ret = xsdk_xam_keyboard(xi_active_port, xi_osk_title, xi_osk_empty, xi_osk_empty, xi_osk_result,
		maxlen + 1, &xi_osk_ov);
	if (ret != ERROR_IO_PENDING && ret != ERROR_SUCCESS) {
		Com_Printf("xsdk_xam_keyboard failed: 0x%X\n", ret);
		return;
	}
	xi_osk_status = ret;

	xi_osk_auto_submit = autoSubmit;
	xi_osk_prepend_slash = prependSlash;
	xi_osk_field_clear = fieldClear;
	xi_osk_enter_on_close = enterOnClose;
	xi_osk_active = qtrue;
	XI_ReleaseHeld();
	Com_Printf("xsdk_xam_keyboard opened\n");
}

static void XI_KeyboardClose(void)
{
	qboolean failed = (xi_osk_status != ERROR_SUCCESS || xi_osk_ov.extended_error == XI_ERROR_CANCELLED);
	int i;

	xi_osk_active = qfalse;
	xi_osk_resync = qtrue;

	if (failed) {
		if (xi_osk_enter_on_close)
			XI_TapKey(K_ENTER);
		return;
	}

	if (xi_osk_field_clear) {
		XI_TapKey(K_END);
		for (i = 0; i < 80; i++)
			Com_QueueEvent(0, SE_CHAR, 8, 0, 0, NULL);
	}
	if (xi_osk_prepend_slash && xi_osk_result[0] != '/' && xi_osk_result[0] != '\\')
		Com_QueueEvent(0, SE_CHAR, '/', 0, 0, NULL);
	for (i = 0; i < XI_OSK_TEXT_MAX && xi_osk_result[i]; i++) {
		if (xi_osk_result[i] >= 32 && xi_osk_result[i] <= 126)
			Com_QueueEvent(0, SE_CHAR, xi_osk_result[i], 0, 0, NULL);
	}
	if (xi_osk_auto_submit || xi_osk_enter_on_close)
		XI_TapKey(K_ENTER);
}

static void XI_ReadButtons(const INPUT_GAMEPAD *pad, int out[PB_COUNT])
{
	out[PB_A]     = (pad->buttons & INPUT_GAMEPAD_A) != 0;
	out[PB_B]     = (pad->buttons & INPUT_GAMEPAD_B) != 0;
	out[PB_X]     = (pad->buttons & INPUT_GAMEPAD_X) != 0;
	out[PB_Y]     = (pad->buttons & INPUT_GAMEPAD_Y) != 0;
	out[PB_LB]    = (pad->buttons & INPUT_GAMEPAD_LEFT_SHOULDER) != 0;
	out[PB_RB]    = (pad->buttons & INPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
	out[PB_LS]    = (pad->buttons & INPUT_GAMEPAD_LEFT_THUMB) != 0;
	out[PB_RS]    = (pad->buttons & INPUT_GAMEPAD_RIGHT_THUMB) != 0;
	out[PB_BACK]  = (pad->buttons & INPUT_GAMEPAD_BACK) != 0;
	out[PB_START] = (pad->buttons & INPUT_GAMEPAD_START) != 0;
	out[PB_UP]    = (pad->buttons & INPUT_GAMEPAD_DPAD_UP) != 0;
	out[PB_DOWN]  = (pad->buttons & INPUT_GAMEPAD_DPAD_DOWN) != 0;
	out[PB_LEFT]  = (pad->buttons & INPUT_GAMEPAD_DPAD_LEFT) != 0;
	out[PB_RIGHT] = (pad->buttons & INPUT_GAMEPAD_DPAD_RIGHT) != 0;
	out[PB_LT]    = pad->left_trigger > XI_TRIGGER_THRESHOLD;
	out[PB_RT]    = pad->right_trigger > XI_TRIGGER_THRESHOLD;
}

/* Zero inside the deadzone, otherwise the raw value clamped to the Q3 axis range. */
static int XI_Deadzone(int value, int deadzone)
{
	if (value <= deadzone && value >= -deadzone)
		return 0;
	if (value > XI_AXIS_MAX)
		return XI_AXIS_MAX;
	if (value < -XI_AXIS_MAX)
		return -XI_AXIS_MAX;
	return value;
}

static float XI_Normalise(int value, int deadzone)
{
	float f;

	if (value == 0)
		return 0.0f;
	f = (float)(value > 0 ? value - deadzone : value + deadzone) / (float)(XI_AXIS_MAX - deadzone);
	if (f > 1.0f)
		f = 1.0f;
	if (f < -1.0f)
		f = -1.0f;
	return f;
}

static float XI_ClampUnit(float f)
{
	if (f > 1.0f)
		return 1.0f;
	if (f < -1.0f)
		return -1.0f;
	return f;
}

static void XI_RumbleStop(void)
{
	INPUT_VIBRATION vib;

	if (s_rumbleActive && xi_active_port >= 0) {
		memset(&vib, 0, sizeof(vib));
		XamInputSetState(xi_active_port, 0, &vib);
	}
	s_rumbleActive = 0;
	s_rumbleExpiryMs = 0;
}

static void XI_RumbleTick(void)
{
	if (!s_rumbleActive)
		return;
	if (Sys_Milliseconds() < s_rumbleExpiryMs)
		return;
	XI_RumbleStop();
}

void XI_SetRumble(int large, int small, int durationMs)
{
	INPUT_VIBRATION vib;
	float scale;
	int lg, sm, expiry;

	if (!x360_rumbleEnable || !x360_rumbleEnable->integer || xi_active_port < 0)
		return;

	scale = x360_rumbleScale ? x360_rumbleScale->value : 1.0f;
	if (scale < 0.0f)
		scale = 0.0f;
	if (scale > 1.0f)
		scale = 1.0f;

	/* Unlike the PS3 small motor (on/off), the 360 one is analog, so its value is passed through. */
	lg = (int)(large * scale);
	sm = (int)(small * scale);
	if (lg > 255)
		lg = 255;
	if (sm > 255)
		sm = 255;

	memset(&vib, 0, sizeof(vib));
	vib.left_motor_speed  = (uint16_t)(lg * 257);
	vib.right_motor_speed = (uint16_t)(sm * 257);
	XamInputSetState(xi_active_port, 0, &vib);

	expiry = Sys_Milliseconds() + durationMs;
	if (expiry > s_rumbleExpiryMs)
		s_rumbleExpiryMs = expiry;
	s_rumbleActive = 1;
}

static void XI_OpenNotifyListener(void)
{
	xi_notify_listener = XamNotifyCreateListener(UINT64_MAX, XI_NOTIFY_MAX_VERSION);
	if (!xi_notify_listener)
		Com_Printf("XamNotifyCreateListener failed\n");
}

void IN_Init(void *windowData)
{
	char *ctrl;

	(void)windowData;

	XI_ResetState();

	XI_SetDefaultBind(K_JOY1,  "+moveup");
	XI_SetDefaultBind(K_JOY2,  "+movedown");
	XI_SetDefaultBind(K_JOY3,  "weapprev");
	XI_SetDefaultBind(K_JOY4,  "weapnext");
	XI_SetDefaultBind(K_JOY5,  "+moveleft");
	XI_SetDefaultBind(K_JOY6,  "+moveright");
	XI_SetDefaultBind(K_JOY7,  "+zoom");
	/* RT always fires, over the saved config; a CTRL +attack made B fire too on remote servers (seen on PS3 HW). */
	Key_SetBinding(K_JOY8, "+attack");
	ctrl = Key_GetBinding(K_CTRL);
	if (ctrl && !Q_stricmp(ctrl, "+attack"))
		Key_SetBinding(K_CTRL, "");
	XI_SetDefaultBind(K_JOY9,  "+speed");
	XI_SetDefaultBind(K_JOY10, "+scores");
	XI_SetDefaultBind(K_JOY11, "+scores");

	Cvar_Set("in_joystick", "1");
	Cvar_Set("in_joystickUseAnalog", "1");
	Cvar_Set("j_pitch_axis", "3");
	Cvar_Set("j_yaw_axis", "4");
	Cvar_Set("j_forward_axis", "1");
	Cvar_Set("j_side_axis", "0");
	/* Archived so the stock Controls menu's joystick threshold slider keeps its value. */
	Cvar_Get("joy_threshold", "0.011", CVAR_ARCHIVE);
	Cvar_Set("j_forward", "-0.25");
	Cvar_Set("j_side", "0.25");

	x360_rumbleEnable = Cvar_Get("x360_rumbleEnable", "1", CVAR_ARCHIVE);
	x360_rumbleScale  = Cvar_Get("x360_rumbleScale", "1.0", CVAR_ARCHIVE);

	XI_OpenNotifyListener();

	Com_Printf("IN_Init: XInput pad input ready\n");
}

void IN_Frame(void)
{
	INPUT_STATE states[XI_PORT_COUNT];
	int ok[XI_PORT_COUNT];
	int cur[PB_COUNT];
	int swallow[PB_COUNT];
	int i, b, cand, key, catchers, in_menu, in_text, dx, dy;
	const INPUT_GAMEPAD *pad;
	int lx, ly, rx, ry;
	float fx, fy;
	uint32_t notify_id, notify_param;

	/* Polled before the pad: XAM keeps the pad from the title while the Guide shows. */
	while (xi_notify_listener && XNotifyGetNext(xi_notify_listener, 0, &notify_id, &notify_param)) {
		if (notify_id != XI_XN_SYS_UI)
			continue;
		Com_Printf("Guide %s\n", notify_param ? "opened" : "closed");
		if (notify_param) {
			XI_RumbleStop();
			XI_ReleaseHeld();
			/* Escape outside an active game disconnects, so only an active game gets the menu. */
			if (Key_GetCatcher() == 0 && clc.state == CA_ACTIVE && !clc.demoplaying)
				XI_TapKey(K_ESCAPE);
		}
	}

	for (i = 0; i < XI_PORT_COUNT; i++) {
		ok[i] = XamInputGetState(i, 0, &states[i]) == ERROR_SUCCESS;
		if (ok[i] != xi_port_connected[i]) {
			xi_port_connected[i] = ok[i];
			Com_Printf("Controller on port %d %s\n", i, ok[i] ? "connected" : "disconnected");
		}
	}

	/* Sticky port: keep the current one while it is connected, else take the lowest connected. */
	cand = -1;
	if (xi_active_port >= 0 && ok[xi_active_port]) {
		cand = xi_active_port;
	} else {
		for (i = 0; i < XI_PORT_COUNT && cand < 0; i++) {
			if (ok[i])
				cand = i;
		}
	}
	if (cand != xi_active_port) {
		XI_RumbleStop();
		XI_ReleaseHeld();
		xi_active_port = cand;
	}
	if (cand < 0)
		return;

	/* XAM owns the pad while the keyboard is up, so no pad events are sent. */
	if (xi_osk_active) {
		if (xi_osk_status == ERROR_IO_PENDING) {
			uint32_t result = xsdk_xam_wait(&xi_osk_ov, 0, NULL);
			if (result == ERROR_IO_INCOMPLETE)
				return;
			xi_osk_status = result;
		}
		XI_KeyboardClose();
		return;
	}
	XI_RumbleTick();

	pad = &states[cand].gamepad;
	XI_ReadButtons(pad, cur);
	/* A held A that closed the keyboard must not fire again. */
	if (xi_osk_resync) {
		memcpy(xi_btn_prev, cur, sizeof(xi_btn_prev));
		xi_osk_resync = qfalse;
	}

	catchers = Key_GetCatcher();
	in_menu = (catchers & (KEYCATCH_UI | KEYCATCH_CGAME)) ? 1 : 0;
	in_text = (catchers & (KEYCATCH_CONSOLE | KEYCATCH_MESSAGE)) ? 1 : 0;

	if (in_menu && !xi_prev_in_menu)
		XI_ZeroAxes();
	xi_prev_in_menu = in_menu;

	for (i = 0; i < PB_COUNT; i++)
		swallow[i] = 0;

	if (cur[PB_Y] && !xi_btn_prev[PB_Y] && cur[PB_BACK]) {
		XI_ToggleConsole();
		swallow[PB_Y] = 1;
	}
	if (cur[PB_A] && !xi_btn_prev[PB_A] && cur[PB_BACK] && !in_menu && !in_text) {
		Cbuf_ExecuteText(EXEC_APPEND, "messagemode\n");
		swallow[PB_A] = 1;
	}
	/* BACK+START quits, as PS3 Select+Start does; swallowed so Escape and +scores stay quiet. */
	if (cur[PB_BACK] && cur[PB_START] && (!xi_btn_prev[PB_BACK] || !xi_btn_prev[PB_START])) {
		Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
		for (b = PB_BACK; b <= PB_START; b++) {
			if (xi_btn_sent[b])
				Com_QueueEvent(0, SE_KEY, xi_btn_sent[b], qfalse, 0, NULL);
			xi_btn_sent[b] = 0;
			swallow[b] = 1;
		}
	}
	if (cur[PB_B] && !xi_btn_prev[PB_B] && (catchers & KEYCATCH_CONSOLE)) {
		XI_ToggleConsole();
		swallow[PB_B] = 1;
	}

#ifdef STANDALONETA
	/* TA enters the field on A one frame before its cursor draws, so the open waits for focus. */
	if (xi_ta_enter_pending) {
		if (!in_menu || Sys_Milliseconds() - xi_ta_enter_ms > XI_TA_ENTER_MS) {
			xi_ta_enter_pending = qfalse;
		} else if (XI_UIFieldFocused()) {
			xi_ta_enter_pending = qfalse;
			XI_KeyboardOpen("Enter text", 80, qfalse, qfalse, qtrue, qtrue);
		}
	}
#endif

	if (cur[PB_A] && !xi_btn_prev[PB_A] && !cur[PB_BACK]) {
		if (catchers & KEYCATCH_CONSOLE) {
			XI_KeyboardOpen("Console Command", 128, qtrue, qtrue, qfalse, qfalse);
		} else if (catchers & KEYCATCH_MESSAGE) {
			XI_KeyboardOpen("Chat", 128, qtrue, qfalse, qfalse, qfalse);
		} else if (in_menu && XI_UIFieldFocused()) {
#ifdef STANDALONETA
			/* TA reports focus only in edit mode, which Enter must end. */
			XI_KeyboardOpen("Enter text", 80, qfalse, qfalse, qtrue, qtrue);
#else
			XI_KeyboardOpen("Enter text", 80, qfalse, qfalse, qtrue, qfalse);
#endif
		}
		if (xi_osk_active)
			swallow[PB_A] = 1;
	}
	if (xi_osk_active)
		return;

	/* LS+RS (edge-detected): toggle rumble enable; both presses are swallowed so +speed/+scores stay quiet. */
	if (cur[PB_LS] && cur[PB_RS] && (!xi_btn_prev[PB_LS] || !xi_btn_prev[PB_RS])) {
		int on = x360_rumbleEnable ? !x360_rumbleEnable->integer : 1;

		Cvar_SetValue("x360_rumbleEnable", on ? 1.0f : 0.0f);
		Com_Printf("Rumble: %s\n", on ? "ON" : "OFF");
		if (on)
			XI_SetRumble(200, 1, 200);
		else
			XI_RumbleStop();
		/* Release whichever stick button was pressed first and already sent its key. */
		for (b = PB_LS; b <= PB_RS; b++) {
			if (xi_btn_sent[b])
				Com_QueueEvent(0, SE_KEY, xi_btn_sent[b], qfalse, 0, NULL);
			xi_btn_sent[b] = 0;
			swallow[b] = 1;
		}
	}

	for (i = 0; i < PB_COUNT; i++) {
		if (cur[i] == xi_btn_prev[i])
			continue;
		xi_btn_prev[i] = cur[i];

		if (!cur[i]) {
			if (xi_btn_sent[i])
				Com_QueueEvent(0, SE_KEY, xi_btn_sent[i], qfalse, 0, NULL);
			xi_btn_sent[i] = 0;
			continue;
		}
		if (swallow[i])
			continue;

		key = xi_key_map[i];
		if (in_menu && i == PB_A) {
			key = K_ENTER;
#ifdef STANDALONETA
			if (!XI_UIFieldFocused()) {
				xi_ta_enter_ms = Sys_Milliseconds();
				xi_ta_enter_pending = qtrue;
			}
#endif
		} else if (in_menu && i == PB_B)
			key = K_ESCAPE;
		xi_btn_sent[i] = key;
		Com_QueueEvent(0, SE_KEY, key, qtrue, 0, NULL);
	}

	/* Y is flipped so down is positive, as on the PS3 pad, which keeps the PS3 cvar values valid. */
	lx = XI_Deadzone(pad->thumb_lx, XI_LEFT_DEADZONE);
	ly = -XI_Deadzone(pad->thumb_ly, XI_LEFT_DEADZONE);
	rx = XI_Deadzone(pad->thumb_rx, XI_RIGHT_DEADZONE);
	ry = -XI_Deadzone(pad->thumb_ry, XI_RIGHT_DEADZONE);

	if (in_menu) {
		/* Linear, not squared: a squared curve underflows the float accumulator at small tilts and stutters. */
		/* Clamped after summing, so full tilt on both sticks is still one full cursor speed. */
		fx = XI_ClampUnit(XI_Normalise(lx, XI_LEFT_DEADZONE) + XI_Normalise(rx, XI_RIGHT_DEADZONE));
		fy = XI_ClampUnit(XI_Normalise(ly, XI_LEFT_DEADZONE) + XI_Normalise(ry, XI_RIGHT_DEADZONE));

		xi_cursor_accum_x += fx * XI_MENU_CURSOR_SPEED;
		xi_cursor_accum_y += fy * XI_MENU_CURSOR_SPEED;
		dx = (int)xi_cursor_accum_x;
		dy = (int)xi_cursor_accum_y;
		xi_cursor_accum_x -= (float)dx;
		xi_cursor_accum_y -= (float)dy;

		if (dx != 0 || dy != 0)
			Com_QueueEvent(0, SE_MOUSE, dx, dy, 0, NULL);
	} else {
		XI_SendAxis(0, lx);
		XI_SendAxis(1, ly);
		XI_SendAxis(3, ry);
		XI_SendAxis(4, rx);
	}
}

void IN_Shutdown(void)
{
	/* No XAM call to cancel the keyboard; none is imported. */
	xi_osk_active = qfalse;
	XI_RumbleStop();
	XI_ReleaseHeld();
	XI_ResetState();
	if (xi_notify_listener) {
		NtClose(xi_notify_listener);
		xi_notify_listener = 0;
	}
}

void IN_Restart(void)
{
	IN_Shutdown();
	XI_OpenNotifyListener();
}
