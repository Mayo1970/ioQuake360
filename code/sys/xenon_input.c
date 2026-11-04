/* XInput pad input for Xenon, ported from the PS3 pad logic in ps3_input.c. */

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/keycodes.h"
#include "xsdk/kernel.h"

extern int Key_GetCatcher(void);
extern void Key_SetBinding(int keynum, const char *binding);
extern char *Key_GetBinding(int keynum);

#define XI_PORT_COUNT         4
#define XI_LEFT_DEADZONE      7849
#define XI_RIGHT_DEADZONE     8689
#define XI_TRIGGER_THRESHOLD  30
#define XI_AXIS_MAX           32767
#define XI_MENU_CURSOR_SPEED  5.0f

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
}

static void XI_ToggleConsole(void)
{
	Com_QueueEvent(0, SE_KEY, K_CONSOLE, qtrue, 0, NULL);
	Com_QueueEvent(0, SE_KEY, K_CONSOLE, qfalse, 0, NULL);
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
	/* Archived: the Controls slider copies it into j_pitch/j_yaw, which otherwise reset to 0 each launch. */
	Cvar_Get("joy_threshold", "0.011", CVAR_ARCHIVE);
	Cvar_Set("j_forward", "-0.25");
	Cvar_Set("j_side", "0.25");

	Com_Printf("IN_Init: XInput pad input ready\n");
}

void IN_Frame(void)
{
	INPUT_STATE states[XI_PORT_COUNT];
	int ok[XI_PORT_COUNT];
	int cur[PB_COUNT];
	int swallow[PB_COUNT];
	int i, cand, key, catchers, in_menu, in_text, dx, dy;
	const INPUT_GAMEPAD *pad;
	int lx, ly, rx, ry;
	float fx, fy;

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
		XI_ReleaseHeld();
		xi_active_port = cand;
	}
	if (cand < 0)
		return;

	pad = &states[cand].gamepad;
	XI_ReadButtons(pad, cur);

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
	if (cur[PB_B] && !xi_btn_prev[PB_B] && (catchers & KEYCATCH_CONSOLE)) {
		XI_ToggleConsole();
		swallow[PB_B] = 1;
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
		if (in_menu && i == PB_A)
			key = K_ENTER;
		else if (in_menu && i == PB_B)
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
	XI_ReleaseHeld();
	XI_ResetState();
}

void IN_Restart(void)
{
	IN_Shutdown();
}
