/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#ifndef _QDAB_SETTINGS_H
#define _QDAB_SETTINGS_H

#include <optional>
#include <string>

#include "bassmidi.h"
#include "nlohmann/json.hpp"

namespace qdab::settings {

static const int sample_rate_vals[] = {
    8000, 11025, 16000, 22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000
};
static const char *sample_rate_strs[] = {
    "8000", "11025", "16000", "22050", "32000", "44100", "48000", "88200", "96000", "176400", "192000"
};
static const int sample_rate_count = 11;

typedef struct {
    std::string path;
    BASS_MIDI_FONT config = { 0, -1, 0 };
} soundfont_t;

typedef struct {
    enum {
        APPEARANCE_DARK = 0,
        APPEARANCE_LIGHT = 1,
        APPEARANCE_CLASSIC = 2
    } appearance = APPEARANCE_DARK;
    float volume = 1.0f;
    float max_rendering_time = 0.0f;
    int max_voices = 1000;
    int sample_rate = 5;
    int ev_buffer_size = 8192;
    struct {
        bool enable;
        int range[2];
        int &operator[](int index) {
            return range[index];
        }
    } ignore_range = { false, { 1, 1 } };
    bool enable_effects = true;
    bool release_oldest_note = false;
    float channel_volumes[16] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
    struct {
        bool enable;
        float gain;
        float threshold;
        float ratio;
        float attack;
        float release;
    } limiter = { true, -1.0f, -10.0f, 25.0f, 5.0f, 50.0f };

    std::vector<soundfont_t> soundfonts;
} settings_t;

std::string get_config_filename(void);

void write_settings(std::string path);
bool read_settings(std::string path);

std::optional<std::string> init_all(void);

void reset(void);

}

#endif