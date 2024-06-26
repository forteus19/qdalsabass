/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#ifndef _QDAB_MIDI_SEQ_H
#define _QDAB_MIDI_SEQ_H

#include <optional>
#include <string>

#include <alsa/asoundlib.h>

namespace qdab::midi::seq {

int get_client_num(void);
int get_port_num(void);
bool is_ready(void);

void stop(void);

std::optional<std::string> init_from_settings(void);

void *seq_main(void *data);

}

#endif