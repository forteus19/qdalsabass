#ifndef _QDAB_MIDI_H
#define _QDAB_MIDI_H

#include <optional>
#include <string>

#include <alsa/asoundlib.h>
#include "bass.h"
#include "bassmidi.h"

namespace qdab::midi {

snd_seq_t *get_seq_handle(void);
int get_client_num(void);
int get_port_num(void);

float get_cpu(void);
int get_active_voices(void);

void set_volume(float volume);
void set_max_voices(int voices);

int add_soundfont(std::string path, int preset, int bank);
bool init_soundfonts(void);
int reload_soundfonts(void);
void free_font(HSOUNDFONT font);

std::optional<std::string> init_from_settings(void);

int get_bass_error(void);
void stop_bass(void);

void gm_reset(void);

void *midi_main(void *data);

}

#endif