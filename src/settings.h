#ifndef _QDAB_SETTINGS_H
#define _QDAB_SETTINGS_H

#include <optional>
#include <string>

#include "bassmidi.h"
#include "nlohmann/json.hpp"

namespace qdab::settings {

typedef struct {
    std::string path;
    BASS_MIDI_FONT config = { 0, -1, 0 };
} soundfont_t;

typedef struct {
    enum appearance_value_ {
        APPEARANCE_DARK = 0,
        APPEARANCE_LIGHT = 1,
        APPEARANCE_CLASSIC = 2
    } appearance = APPEARANCE_DARK;
    float volume = 1.0f;
    int max_voices = 1000;
    
    std::vector<soundfont_t> soundfonts;
} settings_t;

std::string get_config_filename(void);

void write_settings(std::string path);
bool read_settings(std::string path);

std::optional<std::string> init_all(void);

}

#endif