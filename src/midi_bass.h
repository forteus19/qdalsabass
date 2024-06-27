/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#ifndef _QDAB_MIDI_BASS_H
#define _QDAB_MIDI_BASS_H

#include <optional>
#include <string>

namespace qdab::midi::bass {

float get_cpu(void);
int get_active_voices(void);

void set_volume(float volume);
void set_max_cpu(float cpu);
void set_max_voices(int voices);
void set_effects(bool enable);
void set_ron(bool enable);

void set_channel_volume(int channel, float volume);

int add_soundfont(std::string path, int preset, int bank);
bool init_soundfonts(void);
int reload_soundfonts(void);
void free_font(int font);

int get_bass_error(void);

void gm_reset(void);

void stop(void);

std::optional<std::string> init_from_settings(void);

void bass_main(void);

}

#endif