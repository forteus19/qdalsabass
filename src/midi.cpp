#include "midi.h"

#include <cstdio>

#include "global.h"
#include "gui.h"
#include "settings.h"

namespace qdab::midi {

static snd_seq_t *seq_handle = NULL;
static int seq_port;
static HSTREAM bass_stream;

static int rpn_params[16];
static int rpn_vals[16];

const char *SYSEX_GM_RESET = "\xF0\x7E\x7F\x09\x01\xF7";

snd_seq_t *get_seq_handle(void) {
    return seq_handle;
}

int get_client_num(void) {
    return snd_seq_client_id(seq_handle);
}

int get_port_num(void) {
    return seq_port;
}

float get_cpu(void) {
    return BASS_GetCPU();
}

int get_active_voices(void) {
    float val;
    BASS_ChannelGetAttribute(bass_stream, BASS_ATTRIB_MIDI_VOICES_ACTIVE, &val);
    return val;
}

void set_volume(float volume) {
    BASS_ChannelSetAttribute(bass_stream, BASS_ATTRIB_MIDI_VOL, volume);
}

void set_max_voices(int voices) {
    BASS_ChannelSetAttribute(bass_stream, BASS_ATTRIB_MIDI_VOICES, (float)voices);
}

int add_soundfont(std::string path, int preset, int bank) {
    BASS_MIDI_FONT sf_config;
    sf_config.font = BASS_MIDI_FontInit(path.c_str(), 0);
    if (sf_config.font == 0) {
        fprintf(stderr, "BASS error: %d\n", BASS_ErrorGetCode());
        return BASS_ErrorGetCode();
    }
    sf_config.preset = -1;
    sf_config.bank = 0;

    global::settings.soundfonts.push_back({ path, sf_config });
    return 0;
}

bool init_soundfonts(void) {
    int num_freed = 0;
    for (settings::soundfont_t sf : global::settings.soundfonts) {
        if (sf.config.font != 0) {
            BASS_MIDI_FontFree(sf.config.font);
            num_freed++;
        }
    }
    if (num_freed != 0) {
        global::settings.soundfonts.erase(global::settings.soundfonts.begin(), global::settings.soundfonts.begin() + num_freed);
    }

    bool all_loaded = true;
    for (auto sf = global::settings.soundfonts.begin(); sf != global::settings.soundfonts.end(); ) {
        sf->config.font = BASS_MIDI_FontInit(sf->path.c_str(), 0);
        if (sf->config.font == 0) {
            fprintf(stderr, "BASS error: %d\n", BASS_ErrorGetCode());
            sf = global::settings.soundfonts.erase(sf);
            all_loaded = false;
        } else {
            sf++;
        }
    }

    return all_loaded;
}

int reload_soundfonts(void) {
    BASS_MIDI_FONT fonts[global::settings.soundfonts.size()];
    for (int i = 0; i < global::settings.soundfonts.size(); i++) {
        fonts[i] = global::settings.soundfonts[i].config;
    }

    if (!BASS_MIDI_StreamSetFonts(bass_stream, fonts, global::settings.soundfonts.size())) {
        fprintf(stderr, "BASS error: %d\n", BASS_ErrorGetCode());
        return BASS_ErrorGetCode();
    }

    return 0;
}

void free_font(HSOUNDFONT font) {
    BASS_MIDI_FontFree(font);
}

std::optional<std::string> init_from_settings(void) {
    set_volume(global::settings.volume);
    set_max_voices(global::settings.max_voices);
    if (!init_soundfonts()) {
        reload_soundfonts();
        return "One or more soundfonts failed to load.";
    }
    reload_soundfonts();
    return std::nullopt;
}

int get_bass_error(void) {
    return BASS_ErrorGetCode();
}

void gm_reset(void) {
    BASS_MIDI_StreamEvent(bass_stream, 0, MIDI_EVENT_SYSTEMEX, MIDI_SYSTEM_GS);
}

void stop_bass(void) {
    if (BASS_IsStarted() != 0) {
        BASS_Free();
    }
}

void stream_alsa_event(snd_seq_event_t *ev) {
    switch (ev->type) {
        case SND_SEQ_EVENT_NOTEON: BASS_MIDI_StreamEvent(bass_stream, ev->data.note.channel, MIDI_EVENT_NOTE, MAKEWORD(ev->data.note.note, (int)ev->data.note.velocity)); break;
        case SND_SEQ_EVENT_NOTEOFF: BASS_MIDI_StreamEvent(bass_stream, ev->data.note.channel, MIDI_EVENT_NOTE, ev->data.note.note); break;
        case SND_SEQ_EVENT_PGMCHANGE: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PROGRAM, ev->data.control.value); break;
        case SND_SEQ_EVENT_CHANPRESS: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_CHANPRES, ev->data.control.value); break;
        case SND_SEQ_EVENT_KEYPRESS: BASS_MIDI_StreamEvent(bass_stream, ev->data.note.channel, MIDI_EVENT_KEYPRES, MAKEWORD(ev->data.note.note, (int)ev->data.note.velocity)); break;
        case SND_SEQ_EVENT_PITCHBEND: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PITCH, ev->data.control.value + 8192); break;

    case SND_SEQ_EVENT_CONTROLLER:
        switch (ev->data.control.param) {
            case 0: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_BANK, ev->data.control.value); break;
            case 1: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_MODULATION, ev->data.control.value); break;
            case 5: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PORTATIME, ev->data.control.value); break;
            case 7: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_VOLUME, ev->data.control.value); break;
            case 10: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PAN, ev->data.control.value); break;
            case 11: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_EXPRESSION, ev->data.control.value); break;
            case 32: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_BANK_LSB, ev->data.control.value); break;
            case 64: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_SUSTAIN, ev->data.control.value); break;
            case 65: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PORTAMENTO, ev->data.control.value); break;
            case 66: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_SOSTENUTO, ev->data.control.value); break;
            case 67: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_SOFT, ev->data.control.value); break;
            case 71: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_RESONANCE, ev->data.control.value); break;
            case 72: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_RELEASE, ev->data.control.value); break;
            case 73: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_ATTACK, ev->data.control.value); break;
            case 74: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_CUTOFF, ev->data.control.value); break;
            case 75: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_DECAY, ev->data.control.value); break;
            case 76: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_VIBRATO_RATE, ev->data.control.value); break;
            case 77: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_VIBRATO_DEPTH, ev->data.control.value); break;
            case 78: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_VIBRATO_DELAY, ev->data.control.value); break;
            case 84: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PORTANOTE, ev->data.control.value); break;
            case 91: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_REVERB, ev->data.control.value); break;
            case 93: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_CHORUS, ev->data.control.value); break;
            case 94: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_USERFX, ev->data.control.value); break;
            case 100: rpn_params[ev->data.control.channel & 0x0F] = (rpn_params[ev->data.control.channel & 0x0F] & 0xFF00) | (ev->data.control.value & 0xFF); break;
            case 101: rpn_params[ev->data.control.channel & 0x0F] = (rpn_params[ev->data.control.channel & 0x0F] & 0x00FF) | ((ev->data.control.value & 0xFF) << 8); break;
            case 120: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_SOUNDOFF, ev->data.control.value); break;
            case 121: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_RESET, ev->data.control.value); break;
            case 123: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_NOTESOFF, ev->data.control.value); break;
            case 126: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_MODE, 1); break;
            case 127: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_MODE, 0); break;

        case 6:
            switch (rpn_params[ev->data.control.channel]) {
                case 0: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PITCHRANGE, ev->data.control.value); break;
                case 2: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_COARSETUNE, ev->data.control.value); break;
                default: rpn_vals[ev->data.control.channel & 0x0F] = (rpn_vals[ev->data.control.channel & 0x0F] & 0xFF00) | (ev->data.control.value & 0xFF); break;
            }
            break;
        
        case 38:
            switch (rpn_params[ev->data.control.channel]) {
                case 1: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_FINETUNE, ev->data.control.value | (rpn_vals[ev->data.control.channel & 0x0F] << 7)); break;
            }
            break;
        }
        break;

    case SND_SEQ_EVENT_SYSEX:
        {
            if (ev->data.ext.len != 6) {
                break;
            }
            if (strncmp((char *)ev->data.ext.ptr, SYSEX_GM_RESET, 6) == 0) {
                BASS_MIDI_StreamEvent(bass_stream, 0, MIDI_EVENT_SYSTEMEX, MIDI_SYSTEM_GS);
            }
        }
        break;
    }

}

void log_alsa_event(snd_seq_event_t *ev) {
    switch (ev->type) {
        case SND_SEQ_EVENT_NOTEON: printf("[%d] NOTEON key: %d vel: %d\n", ev->data.note.channel, ev->data.note.note, ev->data.note.velocity); break;
        case SND_SEQ_EVENT_NOTEOFF: printf("[%d] NOTEOFF key: %d\n", ev->data.note.channel, ev->data.note.note); break;
        case SND_SEQ_EVENT_CONTROLLER: printf("[%d] CC param: %d value: %d\n", ev->data.control.channel, ev->data.control.param, ev->data.control.value); break;
        case SND_SEQ_EVENT_NONREGPARAM: printf("[%d] NRPN param: %d value: %d\n", ev->data.control.channel, ev->data.control.param, ev->data.control.value); break;
        case SND_SEQ_EVENT_REGPARAM: printf("[%d] RPN param: %d value: %d\n", ev->data.control.channel, ev->data.control.param, ev->data.control.value); break;
        case SND_SEQ_EVENT_SYSEX: {
            printf("[G] SYSEX len: %d data: ", ev->data.ext.len);
            for (int i = 0; i < ev->data.ext.len; i++) {
                printf("0x%02X ", ((uint8_t *)ev->data.ext.ptr)[i]);
            }
            printf("\n");
        } break;
    }
}

void *midi_main(void *data) {
    if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
        fprintf(stderr, "BASS version mismatch");
        global::clean_exit(0);
        return NULL;
    }

    if (!BASS_Init(-1, settings::sample_rate_vals[global::settings.sample_rate], 0, NULL, NULL)) {
        gui::handle_bass_failure(BASS_ErrorGetCode());
        return NULL;
    }

    bass_stream = BASS_MIDI_StreamCreate(16, BASS_SAMPLE_FLOAT | BASS_MIDI_ASYNC, 1);

    BASS_ChannelSetAttribute(bass_stream, BASS_ATTRIB_VOL, 0.75);
    BASS_ChannelSetAttribute(bass_stream, BASS_ATTRIB_BUFFER, 0);
    BASS_ChannelSetAttribute(bass_stream, BASS_ATTRIB_MIDI_VOICES, 1000);
    BASS_ChannelFlags(bass_stream, BASS_MIDI_NOFX, BASS_MIDI_NOFX);

    BASS_MIDI_StreamEvent(bass_stream, 0, MIDI_EVENT_SYSTEM, MIDI_SYSTEM_GS);

    BASS_ChannelPlay(bass_stream, FALSE);

    snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);
    snd_seq_set_client_name(seq_handle, "qdalsabass");

    seq_port = snd_seq_create_simple_port(
        seq_handle,
        "qdalsabass input port",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION
    );

    init_from_settings();

    while (global::running) {
        snd_seq_event_t *ev = NULL;
        if (snd_seq_event_input(seq_handle, &ev) >= 0) {
            stream_alsa_event(ev);
            // log_alsa_event(ev);
        }
    }

    return NULL;
}

}