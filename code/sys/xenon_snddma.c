#include "../client/snd_local.h"
#include "xsdk/kernel.h"

enum {
	XENON_AUDIO_RATE = 48000,
	XENON_AUDIO_CHANNELS = 2,
	XENON_AUDIO_FULLSAMPLES = 4096,
	XENON_AUDIO_SAMPLES = XENON_AUDIO_FULLSAMPLES * XENON_AUDIO_CHANNELS,
	XENON_AUDIO_SUBMISSION_CHUNK = 1024,
	XENON_AUDIO_FRAME_CHANNELS = 6,
	XENON_AUDIO_FRAME_SAMPLES = 256,
	XENON_AUDIO_FRAME_FLOATS = XENON_AUDIO_FRAME_CHANNELS * XENON_AUDIO_FRAME_SAMPLES,
	XENON_AUDIO_FRAME_BYTES = XENON_AUDIO_FRAME_FLOATS * sizeof(float),
	/* Rotating slots keep a submitted frame unchanged while XAudio may still be reading it. */
	XENON_AUDIO_QUEUE = 4,
};

static float s_ring[XENON_AUDIO_SAMPLES] __attribute__((aligned(16)));
static float *s_frames;
static uint32_t s_driver;
static uint32_t s_read_pairs;
static uint32_t s_next_frame_slot;
static XAUDIO_RENDER_DRIVER_CLIENT s_client;

static cvar_t *s_sndDebug;
static uint32_t s_callbacks_entered;
static uint32_t s_callbacks_driver_zero;
static uint32_t s_submit_errors;
static uint32_t s_last_submit_result;
static uint32_t s_last_frame_peak_bits;
static uint32_t s_last_frame_pointer;
static uint32_t s_last_frame_slot;
static int s_audio_init_time;
static int s_last_debug_time;
static qboolean s_first_callback_reported;
static qboolean s_no_callback_reported;

static void Xenon_AudioDebugThink(void)
{
	const int now = Sys_Milliseconds();
	const uint32_t callbacks = __atomic_load_n(&s_callbacks_entered, __ATOMIC_ACQUIRE);

	if (!callbacks && !s_no_callback_reported && now - s_audio_init_time >= 3000) {
		Com_Printf("Xenon audio: no render callback after 3 s\n");
		s_no_callback_reported = qtrue;
	}

	if (!s_first_callback_reported && callbacks) {
		Com_Printf("Xenon audio: first render callback after %d ms\n", now - s_audio_init_time);
		s_first_callback_reported = qtrue;
	}

	if (!s_sndDebug || !s_sndDebug->integer || now - s_last_debug_time < 1000)
		return;

	s_last_debug_time = now;
	{
		union { uint32_t u; float f; } peak;
		peak.u = __atomic_load_n(&s_last_frame_peak_bits, __ATOMIC_RELAXED);
		Com_Printf("Xenon audio debug: callbacks=%u driver0=%u submitErrors=%u lastResult=0x%08X peak=%.6f lastFrame=0x%08X slot=%u readPairs=%u\n",
			(unsigned)callbacks,
			(unsigned)__atomic_load_n(&s_callbacks_driver_zero, __ATOMIC_RELAXED),
			(unsigned)__atomic_load_n(&s_submit_errors, __ATOMIC_RELAXED),
			(unsigned)__atomic_load_n(&s_last_submit_result, __ATOMIC_RELAXED),
			peak.f,
			(unsigned)__atomic_load_n(&s_last_frame_pointer, __ATOMIC_RELAXED),
			(unsigned)__atomic_load_n(&s_last_frame_slot, __ATOMIC_RELAXED),
			(unsigned)__atomic_load_n(&s_read_pairs, __ATOMIC_ACQUIRE));
	}
}

static void Xenon_FillFrame(float *frame)
{
	uint32_t pair = __atomic_load_n(&s_read_pairs, __ATOMIC_RELAXED) % XENON_AUDIO_FULLSAMPLES;

	for (int i = 0; i < XENON_AUDIO_FRAME_SAMPLES; i++) {
		frame[i] = s_ring[pair * XENON_AUDIO_CHANNELS];
		frame[XENON_AUDIO_FRAME_SAMPLES + i] = s_ring[pair * XENON_AUDIO_CHANNELS + 1];
		pair = (pair + 1) % XENON_AUDIO_FULLSAMPLES;
	}
	__atomic_fetch_add(&s_read_pairs, XENON_AUDIO_FRAME_SAMPLES, __ATOMIC_RELEASE);
}

static void Xenon_RenderCallback(const uint32_t *context)
{
	(void)context;
	__atomic_fetch_add(&s_callbacks_entered, 1, __ATOMIC_RELAXED);

	const uint32_t driver = __atomic_load_n(&s_driver, __ATOMIC_ACQUIRE);
	if (!driver) {
		__atomic_fetch_add(&s_callbacks_driver_zero, 1, __ATOMIC_RELAXED);
		return;
	}

	const uint32_t slot = s_next_frame_slot++ % XENON_AUDIO_QUEUE;
	float *const frame = s_frames + slot * XENON_AUDIO_FRAME_FLOATS;
	Xenon_FillFrame(frame);
	float peak = 0.0f;
	union { float f; uint32_t u; } peak_bits;
	for (uint32_t i = 0; i < XENON_AUDIO_FRAME_SAMPLES; i++) {
		const float left = frame[i] < 0.0f ? -frame[i] : frame[i];
		const float right = frame[XENON_AUDIO_FRAME_SAMPLES + i] < 0.0f ? -frame[XENON_AUDIO_FRAME_SAMPLES + i] : frame[XENON_AUDIO_FRAME_SAMPLES + i];
		if (left > peak) peak = left;
		if (right > peak) peak = right;
	}
	peak_bits.f = peak;
	__atomic_store_n(&s_last_frame_peak_bits, peak_bits.u, __ATOMIC_RELAXED);
	__atomic_store_n(&s_last_frame_pointer, (uint32_t)(uintptr_t)frame, __ATOMIC_RELAXED);
	__atomic_store_n(&s_last_frame_slot, slot, __ATOMIC_RELAXED);

	const uint32_t submit_result = XAudioSubmitRenderDriverFrame(driver, frame);
	if (submit_result != ERROR_SUCCESS) {
		__atomic_fetch_add(&s_submit_errors, 1, __ATOMIC_RELAXED);
		__atomic_store_n(&s_last_submit_result, submit_result, __ATOMIC_RELAXED);
	}
}

qboolean SNDDMA_Init(void)
{
	NTSTATUS status;
	uint32_t driver = 0;
	if (!s_sndDebug) {
		s_sndDebug = Cvar_Get("x360_sndDebug", "0", 0);
		Cvar_SetDescription(s_sndDebug, "Print Xenon audio callback counters once per second.");
	}
	s_audio_init_time = Sys_Milliseconds();
	s_last_debug_time = s_audio_init_time;
	s_first_callback_reported = qfalse;
	s_no_callback_reported = qfalse;
	__atomic_store_n(&s_callbacks_entered, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_callbacks_driver_zero, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_submit_errors, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_last_submit_result, ERROR_SUCCESS, __ATOMIC_RELAXED);
	__atomic_store_n(&s_last_frame_peak_bits, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_last_frame_pointer, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_last_frame_slot, UINT32_MAX, __ATOMIC_RELAXED);

	memset(s_ring, 0, sizeof(s_ring));
	s_frames = MmAllocatePhysicalMemoryEx(0, XENON_AUDIO_FRAME_BYTES * XENON_AUDIO_QUEUE, PAGE_READWRITE, 0,
		UINT32_MAX, 0);
	if (!s_frames) {
		Com_Printf("Xenon audio: frame allocation failed\n");
		return qfalse;
	}
	memset(s_frames, 0, XENON_AUDIO_FRAME_BYTES * XENON_AUDIO_QUEUE);
	s_read_pairs = 0;
	s_next_frame_slot = 0;

	s_client.callback = Xenon_RenderCallback;
	s_client.context = 0;
	status = XAudioRegisterRenderDriverClient(&s_client, &driver);
	Com_Printf("Xenon audio: render driver id=%u frames virtual=0x%08X physical=0x%08X frameBytes=%u\n",
		(unsigned)driver, (unsigned)(uintptr_t)s_frames, (unsigned)MmGetPhysicalAddress(s_frames), (unsigned)XENON_AUDIO_FRAME_BYTES);
	if (status != ERROR_SUCCESS) {
		Com_Printf("Xenon audio: XAudioRegisterRenderDriverClient failed: 0x%08X\n", (unsigned)status);
		goto fail;
	}

	memset(&dma, 0, sizeof(dma));
	dma.channels         = XENON_AUDIO_CHANNELS;
	dma.samples          = XENON_AUDIO_SAMPLES;
	dma.fullsamples      = XENON_AUDIO_FULLSAMPLES;
	dma.submission_chunk = XENON_AUDIO_SUBMISSION_CHUNK;
	dma.samplebits       = 32;
	dma.isfloat          = 1;
	dma.speed            = XENON_AUDIO_RATE;
	dma.buffer           = (byte *)s_ring;

	__atomic_store_n(&s_driver, driver, __ATOMIC_RELEASE);
	Com_Printf("Xenon audio: %d Hz, %d channels, 32-bit float, filled in the render callback\n",
		XENON_AUDIO_RATE, XENON_AUDIO_CHANNELS);
	return qtrue;

fail:
	SNDDMA_Shutdown();
	return qfalse;
}

int SNDDMA_GetDMAPos(void)
{
	Xenon_AudioDebugThink();
	const uint32_t pairs = __atomic_load_n(&s_read_pairs, __ATOMIC_ACQUIRE) % XENON_AUDIO_FULLSAMPLES;
	return (int)(pairs * XENON_AUDIO_CHANNELS);
}

/* Unpublish and unregister before freeing the frames a callback may still read. */
void SNDDMA_Shutdown(void)
{
	const uint32_t driver = __atomic_exchange_n(&s_driver, 0, __ATOMIC_ACQ_REL);

	if (driver)
		(void)XAudioUnregisterRenderDriverClient(driver);
	if (s_frames)
		MmFreePhysicalMemory(0, s_frames);

	s_frames = NULL;
}

void SNDDMA_BeginPainting(void)
{
}

void SNDDMA_Submit(void)
{
}
