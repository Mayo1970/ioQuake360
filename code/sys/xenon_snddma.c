#include "../client/snd_local.h"
#include "xsdk/kernel.h"
#include "xsdk/thread.h"

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
	/* Frames the feeder may run ahead of the kernel; one more silent frame is allocated after them. */
	XENON_AUDIO_QUEUE = 4,
	XENON_AUDIO_FEEDER_HWTHREAD = 4,
};

static float s_ring[XENON_AUDIO_SAMPLES] __attribute__((aligned(16)));
static float *s_frames;
static float *s_silence;
static HANDLE s_event;
static HANDLE s_started;
static HANDLE s_feeder_thread;
static uint32_t s_quit;
static uint32_t s_driver;
static uint32_t s_feeder_hwthread;
static uint32_t s_ready_head;
static uint32_t s_ready_tail;
/* Monotonic count of sample pairs the feeder has taken from the ring; wraps cleanly since 2^32 is a multiple of the ring size. */
static uint32_t s_read_pairs;
static XAUDIO_RENDER_DRIVER_CLIENT s_client;

static cvar_t *s_sndDebug;
static uint32_t s_callbacks_entered;
static uint32_t s_callbacks_driver_zero;
static uint32_t s_frames_submitted;
static uint32_t s_silent_frames_submitted;
static uint32_t s_submit_errors;
static uint32_t s_last_submit_result;
static uint32_t s_last_queue_peak_bits;
static uint32_t s_last_frame_pointer;
static uint32_t s_last_frame_slot;
static uint32_t s_inflight_overwrites;
static uint32_t s_frames_filled;
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
		peak.u = __atomic_load_n(&s_last_queue_peak_bits, __ATOMIC_RELAXED);
		Com_Printf("Xenon audio debug: callbacks=%u driver0=%u queued=%u silent=%u submitErrors=%u lastResult=0x%08X peakQueue=%.6f lastFrame=0x%08X slot=%d inflightOverwrites=%u filled=%u head=%u tail=%u readPairs=%u\n",
			(unsigned)callbacks,
			(unsigned)__atomic_load_n(&s_callbacks_driver_zero, __ATOMIC_RELAXED),
			(unsigned)__atomic_load_n(&s_frames_submitted, __ATOMIC_RELAXED),
			(unsigned)__atomic_load_n(&s_silent_frames_submitted, __ATOMIC_RELAXED),
			(unsigned)__atomic_load_n(&s_submit_errors, __ATOMIC_RELAXED),
			(unsigned)__atomic_load_n(&s_last_submit_result, __ATOMIC_RELAXED),
			peak.f,
			(unsigned)__atomic_load_n(&s_last_frame_pointer, __ATOMIC_RELAXED),
			(int32_t)__atomic_load_n(&s_last_frame_slot, __ATOMIC_RELAXED),
			(unsigned)__atomic_load_n(&s_inflight_overwrites, __ATOMIC_RELAXED),
			(unsigned)__atomic_load_n(&s_frames_filled, __ATOMIC_RELAXED),
			(unsigned)__atomic_load_n(&s_ready_head, __ATOMIC_ACQUIRE),
			(unsigned)__atomic_load_n(&s_ready_tail, __ATOMIC_ACQUIRE),
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

static void Xenon_FillFreeFrames(void)
{
	uint32_t tail = s_ready_tail;

	while (tail - __atomic_load_n(&s_ready_head, __ATOMIC_ACQUIRE) < XENON_AUDIO_QUEUE) {
		// the slot the callback just handed to XAudio is refilled here; counted in case XAudio reads it later
		if (tail % XENON_AUDIO_QUEUE == __atomic_load_n(&s_last_frame_slot, __ATOMIC_RELAXED))
			__atomic_fetch_add(&s_inflight_overwrites, 1, __ATOMIC_RELAXED);
		Xenon_FillFrame(s_frames + (tail % XENON_AUDIO_QUEUE) * XENON_AUDIO_FRAME_FLOATS);
		__atomic_fetch_add(&s_frames_filled, 1, __ATOMIC_RELAXED);
		tail++;
		__atomic_store_n(&s_ready_tail, tail, __ATOMIC_RELEASE);
	}
}

static uint32_t Xenon_FeederMain(void *context)
{
	(void)context;

	s_feeder_hwthread = xsdk_thread_hardware_thread();
	(void)NtSetEvent(s_started, NULL);

	while (!__atomic_load_n(&s_quit, __ATOMIC_ACQUIRE)) {
		Xenon_FillFreeFrames();
		(void)NtWaitForSingleObjectEx(s_event, USER_MODE, 0, NULL);
	}
	return 0;
}

/* Runs on the kernel's audio thread once per frame; a silent frame on underrun avoids replaying stale audio. */
static void Xenon_RenderCallback(const uint32_t *context)
{
	(void)context;
	__atomic_fetch_add(&s_callbacks_entered, 1, __ATOMIC_RELAXED);

	const uint32_t driver = __atomic_load_n(&s_driver, __ATOMIC_ACQUIRE);
	if (!driver) {
		__atomic_fetch_add(&s_callbacks_driver_zero, 1, __ATOMIC_RELAXED);
		return;
	}

	const uint32_t head = __atomic_load_n(&s_ready_head, __ATOMIC_RELAXED);
	const uint32_t tail = __atomic_load_n(&s_ready_tail, __ATOMIC_ACQUIRE);
	const int ready = tail != head;
	const uint32_t slot = ready ? head % XENON_AUDIO_QUEUE : UINT32_MAX;
	const float *frame = ready ? s_frames + slot * XENON_AUDIO_FRAME_FLOATS : s_silence;
	if (ready) {
		float peak = 0.0f;
		union { float f; uint32_t u; } peak_bits;
		for (uint32_t i = 0; i < XENON_AUDIO_FRAME_SAMPLES; i++) {
			const float left = frame[i] < 0.0f ? -frame[i] : frame[i];
			const float right = frame[XENON_AUDIO_FRAME_SAMPLES + i] < 0.0f ? -frame[XENON_AUDIO_FRAME_SAMPLES + i] : frame[XENON_AUDIO_FRAME_SAMPLES + i];
			if (left > peak) peak = left;
			if (right > peak) peak = right;
		}
		peak_bits.f = peak;
		__atomic_store_n(&s_last_queue_peak_bits, peak_bits.u, __ATOMIC_RELAXED);
		__atomic_fetch_add(&s_frames_submitted, 1, __ATOMIC_RELAXED);
	} else {
		__atomic_fetch_add(&s_silent_frames_submitted, 1, __ATOMIC_RELAXED);
	}
	__atomic_store_n(&s_last_frame_pointer, (uint32_t)(uintptr_t)frame, __ATOMIC_RELAXED);
	__atomic_store_n(&s_last_frame_slot, slot, __ATOMIC_RELAXED);

	const uint32_t submit_result = XAudioSubmitRenderDriverFrame(driver, frame);
	if (submit_result != ERROR_SUCCESS) {
		__atomic_fetch_add(&s_submit_errors, 1, __ATOMIC_RELAXED);
		__atomic_store_n(&s_last_submit_result, submit_result, __ATOMIC_RELAXED);
	}
	if (ready)
		__atomic_store_n(&s_ready_head, head + 1, __ATOMIC_RELEASE);
	(void)NtSetEvent(s_event, NULL);
}

static void Xenon_StopFeeder(void)
{
	if (!s_feeder_thread)
		return;

	__atomic_store_n(&s_quit, 1, __ATOMIC_RELEASE);
	(void)NtSetEvent(s_event, NULL);
	(void)NtWaitForSingleObjectEx(s_feeder_thread, USER_MODE, 0, NULL);
	(void)NtClose(s_feeder_thread);
	s_feeder_thread = 0;
}

qboolean SNDDMA_Init(void)
{
	NTSTATUS status;
	uint32_t driver = 0;
	if (!s_sndDebug) {
		s_sndDebug = Cvar_Get("x360_sndDebug", "0", 0);
		Cvar_SetDescription(s_sndDebug, "Print Xenon audio callback and queue counters once per second.");
	}
	s_audio_init_time = Sys_Milliseconds();
	s_last_debug_time = s_audio_init_time;
	s_first_callback_reported = qfalse;
	s_no_callback_reported = qfalse;
	__atomic_store_n(&s_callbacks_entered, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_callbacks_driver_zero, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_frames_submitted, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_silent_frames_submitted, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_submit_errors, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_last_submit_result, ERROR_SUCCESS, __ATOMIC_RELAXED);
	__atomic_store_n(&s_last_queue_peak_bits, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_last_frame_pointer, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_last_frame_slot, UINT32_MAX, __ATOMIC_RELAXED);
	__atomic_store_n(&s_inflight_overwrites, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s_frames_filled, 0, __ATOMIC_RELAXED);

	memset(s_ring, 0, sizeof(s_ring));
	s_frames = MmAllocatePhysicalMemoryEx(0, XENON_AUDIO_FRAME_BYTES * (XENON_AUDIO_QUEUE + 1), PAGE_READWRITE, 0,
		UINT32_MAX, 0);
	if (!s_frames) {
		Com_Printf("Xenon audio: frame allocation failed\n");
		return qfalse;
	}
	memset(s_frames, 0, XENON_AUDIO_FRAME_BYTES * (XENON_AUDIO_QUEUE + 1));
	s_silence = s_frames + XENON_AUDIO_QUEUE * XENON_AUDIO_FRAME_FLOATS;
	s_ready_head = 0;
	s_ready_tail = 0;
	s_read_pairs = 0;
	s_quit = 0;

	status = NtCreateEvent(&s_event, NULL, SYNCHRONIZATION_EVENT, 0);
	if (!NT_SUCCESS(status)) {
		Com_Printf("Xenon audio: NtCreateEvent failed: 0x%08X\n", (unsigned)status);
		goto fail;
	}
	status = NtCreateEvent(&s_started, NULL, SYNCHRONIZATION_EVENT, 0);
	if (!NT_SUCCESS(status)) {
		Com_Printf("Xenon audio: NtCreateEvent failed: 0x%08X\n", (unsigned)status);
		goto fail;
	}

	status = xsdk_thread_create_on(&s_feeder_thread, 0, Xenon_FeederMain, NULL, XENON_AUDIO_FEEDER_HWTHREAD);
	if (!NT_SUCCESS(status)) {
		Com_Printf("Xenon audio: feeder thread creation failed: 0x%08X\n", (unsigned)status);
		goto fail;
	}
	(void)NtWaitForSingleObjectEx(s_started, USER_MODE, 0, NULL);

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
	Com_Printf("Xenon audio: %d Hz, %d channels, 32-bit float, feeder on hardware thread %u\n",
		XENON_AUDIO_RATE, XENON_AUDIO_CHANNELS, (unsigned)s_feeder_hwthread);
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

/* Order matters: the driver is unhooked before the feeder stops, and the client is unregistered before frames are freed. */
void SNDDMA_Shutdown(void)
{
	const uint32_t driver = __atomic_exchange_n(&s_driver, 0, __ATOMIC_ACQ_REL);

	Xenon_StopFeeder();
	if (driver)
		(void)XAudioUnregisterRenderDriverClient(driver);
	if (s_frames)
		MmFreePhysicalMemory(0, s_frames);
	if (s_event)
		(void)NtClose(s_event);
	if (s_started)
		(void)NtClose(s_started);

	s_frames = NULL;
	s_silence = NULL;
	s_event = 0;
	s_started = 0;
}

void SNDDMA_BeginPainting(void)
{
}

void SNDDMA_Submit(void)
{
}
