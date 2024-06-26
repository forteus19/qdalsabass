/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#include "midi_seq.h"

#include <cstdio>

#include "global.h"
#include "midi.h"

namespace qdab::midi::seq {

static snd_seq_t *seq_handle = NULL;
static int seq_port;

int get_client_num(void) {
    return snd_seq_client_id(seq_handle);
}

int get_port_num(void) {
    return seq_port;
}

bool is_ready(void) {
    return seq_handle != NULL;
}

void stop(void) {
}

std::optional<std::string> init_from_settings(void) {
    return std::nullopt;
}

void *seq_main(void *data) {
    snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);
    snd_seq_set_client_name(seq_handle, "qdalsabass");

    seq_port = snd_seq_create_simple_port(
        seq_handle,
        "qdalsabass input port",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION
    );

    event_t *ev = NULL;
    while (global::running) {
        while (snd_seq_event_input(seq_handle, &ev) >= 0) {
            midi::put_event(ev);
        }
    }

    return NULL;
}

}