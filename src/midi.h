/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#ifndef _QDAB_MIDI_H
#define _QDAB_MIDI_H

#include <optional>
#include <string>

#include <alsa/asoundlib.h>
#include "bass.h"
#include "bassmidi.h"

#include "midi_bass.h"
#include "midi_seq.h"

namespace qdab::midi {

typedef snd_seq_event_t event_t;

int get_client_num(void);
int get_port_num(void);
bool is_ready(void);

float get_cpu(void);
int get_active_voices(void);

void set_volume(float volume);
void set_max_voices(int voices);

int add_soundfont(std::string path, int preset, int bank);
bool init_soundfonts(void);
int reload_soundfonts(void);
void free_font(int font);

int get_bass_error(void);

void gm_reset(void);

bool put_event(event_t *ev);
bool get_event(event_t *ev);

int get_rhead_pos(void);
int get_whead_pos(void);

std::optional<std::string> init_from_settings(void);

void stop(void);

void init(void);

}

#endif