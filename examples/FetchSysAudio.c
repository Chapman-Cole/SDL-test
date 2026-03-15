#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

#include <fftw3.h>
#include <SDL3/SDL.h>

#define AUDIO_BUFFER_LEN 4096
#define FFT_SIZE (AUDIO_BUFFER_LEN)
#define MAIN_AUDIO_BUFFER_LEN ((FFT_SIZE / 2) + 1)

float rawAudioBuffer[AUDIO_BUFFER_LEN];
Uint32 rawAudioBufferCounter = 0;

float mainAudioBuffer[MAIN_AUDIO_BUFFER_LEN];
Uint32 mainAudioBufferCounter = 0;

#define BIN_SIZE 128

float visualizerBars[BIN_SIZE];

double* fftIn;
fftw_complex* fftwOut;
fftw_plan fftwPlan;

typedef struct MainTInput {
    char* objName;
} MainTInput;

struct app {
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    struct spa_audio_info_raw format;
};

static void on_process(void *userdata) {
    struct app *app = userdata;
    struct pw_buffer *b;
    struct spa_buffer *buf;

    b = pw_stream_dequeue_buffer(app->stream);
    if (b == NULL)
        return;

    buf = b->buffer;
    if (buf->n_datas < 1 || buf->datas[0].data == NULL || buf->datas[0].chunk == NULL) {
        pw_stream_queue_buffer(app->stream, b);
        return;
    }

    const void *data = buf->datas[0].data;
    uint32_t offset = buf->datas[0].chunk->offset;
    uint32_t size   = buf->datas[0].chunk->size;
    uint32_t stride = buf->datas[0].chunk->stride;

    if (size == 0) {
        pw_stream_queue_buffer(app->stream, b);
        return;
    }

    const uint8_t *p = (const uint8_t *)data + offset;

    /* This example negotiates F32LE stereo, so interpret as float samples. */
    if (stride == 0)
        stride = sizeof(float) * app->format.channels;

    uint32_t n_frames = size / stride;
    const float *samples = (const float *)p;

    for (int i = 0; i < n_frames && rawAudioBufferCounter < AUDIO_BUFFER_LEN; i++) {
        float left = samples[i * app->format.channels];
        float right = (app->format.channels > 1) ? samples[i * app->format.channels + 1] : left;
        rawAudioBuffer[rawAudioBufferCounter] = 0.5f * left + 0.5f * right;
        rawAudioBufferCounter++;
    }

    if (rawAudioBufferCounter >= AUDIO_BUFFER_LEN) {
        rawAudioBufferCounter = 0;

        for (int i = 0; i < FFT_SIZE; i++) {
            fftIn[i] = rawAudioBuffer[i] * (0.54 - 0.46 * SDL_cos(2 * SDL_PI_F * i / (FFT_SIZE - 1)));
        }

        fftw_execute(fftwPlan);

        for (int i = 0; i < MAIN_AUDIO_BUFFER_LEN; i++) {
            double real = fftwOut[i][0];
            double imag = fftwOut[i][1];
            double magnitude = SDL_sqrt(real * real + imag * imag);
            mainAudioBuffer[i] = magnitude / FFT_SIZE;
            //mainAudioBuffer[i] = 20.0f * SDL_log10(magnitude + 1e-12);
            //mainAudioBuffer[i] = SDL_clamp(mainAudioBuffer[i], -80.0f, 0.0f);
            //mainAudioBuffer[i] = (mainAudioBuffer[i] + 80.0f) / 80.0f;
        }
    }

    //float nyquist = app->format.rate * 0.5f;
    float minFreq = 40.0f;
    float maxFreq = 16000.0f;

    float rawBars[BIN_SIZE];
    float smoothedBars[BIN_SIZE];

    int maxBin = FFT_SIZE / 2 + 1;
    int nextBin = 1;  // skip DC

    for (int b = 0; b < BIN_SIZE; b++) {
        float t0 = (float)b / (float)BIN_SIZE;
        float t1 = (float)(b + 1) / (float)BIN_SIZE;

        float f0 = minFreq * SDL_powf(maxFreq / minFreq, t0);
        float f1 = minFreq * SDL_powf(maxFreq / minFreq, t1);
        float centerFreq = 0.5f * (f0 + f1);

        int targetStart = (int)SDL_floorf(f0 * (float)FFT_SIZE / (float)app->format.rate);
        int targetEnd   = (int)SDL_ceilf (f1 * (float)FFT_SIZE / (float)app->format.rate);

        if (targetStart < 1) targetStart = 1;
        if (targetEnd < 2) targetEnd = 2;

        if (targetStart > maxBin - 1) targetStart = maxBin - 1;
        if (targetEnd > maxBin) targetEnd = maxBin;

        /* enforce non-overlapping monotonic ranges */
        int i0 = targetStart;
        if (i0 < nextBin) i0 = nextBin;

        int i1 = targetEnd;
        if (i1 <= i0) i1 = i0 + 1;
        if (i1 > maxBin) i1 = maxBin;

        if (i0 >= maxBin) {
            i0 = maxBin - 1;
            i1 = maxBin;
        }

        nextBin = i1;

        /* RMS over the band */
        float accum = 0.0f;
        int count = 0;
        for (int i = i0; i < i1; i++) {
            float v = mainAudioBuffer[i];
            accum += v * v;
            count++;
        }

        float bandValue = (count > 0) ? SDL_sqrtf(accum / (float)count) : 0.0f;

        /*
            Gentle frequency compensation:
            - slightly reduce bass dominance
            - slightly boost mids/highs
            Keep this subtle.
        */
        float weight = SDL_powf((centerFreq + 120.0f) / 1000.0f, 0.22f);

        if (weight < 0.75f) weight = 0.75f;
        if (weight > 1.35f) weight = 1.8f;

        bandValue *= weight;

        /* Mild dynamic compression for display */
        rawBars[b] = SDL_powf(bandValue, 0.60f);
    }

    /* Spatial smoothing */
    for (int b = 0; b < BIN_SIZE; b++) {
        float left   = (b > 0) ? rawBars[b - 1] : rawBars[b];
        float center = rawBars[b];
        float right  = (b < BIN_SIZE - 1) ? rawBars[b + 1] : rawBars[b];

        smoothedBars[b] = 0.20f * left + 0.60f * center + 0.20f * right;
    }

    /* Mild per-frame normalization */
    float maxValue = 1e-6f;
    for (int b = 0; b < BIN_SIZE; b++) {
        if (smoothedBars[b] > maxValue) {
            maxValue = smoothedBars[b];
        }
    }

    /*
        Do not normalize fully to 1.0 every frame,
        because that can look too twitchy.
    */
    float norm = 1.0f / maxValue;
    float normalizeStrength = 0.65f;

    for (int b = 0; b < BIN_SIZE; b++) {
        float normalized = smoothedBars[b] * norm;
        visualizerBars[b] =
            smoothedBars[b] * (1.0f - normalizeStrength) +
            normalized * normalizeStrength;
    }

    pw_stream_queue_buffer(app->stream, b);
}

static void on_param_changed(void *userdata, uint32_t id, const struct spa_pod *param) {
    struct app *app = userdata;

    if (param == NULL || id != SPA_PARAM_Format)
        return;

    if (spa_format_audio_raw_parse(param, &app->format) < 0)
        return;

    fprintf(stderr, "negotiated format: rate=%u channels=%u format=%d\n",
            app->format.rate, app->format.channels, app->format.format);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .param_changed = on_param_changed,
    .process = on_process,
};

static void do_quit(void *userdata, int signal_number) {
    struct app *app = userdata;
    pw_main_loop_quit(app->loop);
}

int FetchAudioMainT(void* funcInput) {   
    fftIn = (double*)fftw_malloc(sizeof(double) * FFT_SIZE);
    fftwOut = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (FFT_SIZE / 2 + 1));
    fftwPlan = fftw_plan_dft_r2c_1d(FFT_SIZE, fftIn, fftwOut, FFTW_ESTIMATE);

    struct app app = {0};
    struct pw_properties *props;
    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    pw_init(NULL, NULL);

    app.loop = pw_main_loop_new(NULL);

    props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Music",
        PW_KEY_TARGET_OBJECT, ((MainTInput*)funcInput)->objName,
        PW_KEY_NODE_LATENCY, "128/48000",
        NULL
    );

    app.stream = pw_stream_new_simple(
        pw_main_loop_get_loop(app.loop),
        "system-audio-capture",
        props,
        &stream_events,
        &app
    );

    params[0] = spa_format_audio_raw_build(&b,
        SPA_PARAM_EnumFormat,
        &(struct spa_audio_info_raw) {
            .format = SPA_AUDIO_FORMAT_F32_LE,
            .channels = 2,
            .rate = 48000,
            .position = { SPA_AUDIO_CHANNEL_FL, SPA_AUDIO_CHANNEL_FR }
        });

    if (pw_stream_connect(app.stream,
            PW_DIRECTION_INPUT,
            PW_ID_ANY,
            PW_STREAM_FLAG_AUTOCONNECT |
            PW_STREAM_FLAG_MAP_BUFFERS |
            PW_STREAM_FLAG_RT_PROCESS,
            params,
            1) < 0) {
        fprintf(stderr, "failed to connect stream\n");
        return 1;
    }

    pw_loop_add_signal(pw_main_loop_get_loop(app.loop), SIGINT, do_quit, &app);
    pw_loop_add_signal(pw_main_loop_get_loop(app.loop), SIGTERM, do_quit, &app);

    pw_main_loop_run(app.loop);

    pw_stream_destroy(app.stream);
    pw_main_loop_destroy(app.loop);
    pw_deinit();

    fftw_destroy_plan(fftwPlan);
    fftw_free(fftIn);
    fftw_free(fftwOut);
    return 0;
}