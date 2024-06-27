/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#include "midi_seq.h"

#include <thread>

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

bool within_filter(event_t *ev) {
    if (!global::settings.ignore_range.enable) {
        return true;
    }
    if (ev->type != SND_SEQ_EVENT_NOTEON) {
        return true;
    }
    if (ev->data.note.velocity == 0) {
        return true;
    }
    if (ev->data.note.velocity >= global::settings.ignore_range[0] && ev->data.note.velocity <= global::settings.ignore_range[1]) {
        return false;
    } else {
        return true;
    }
}

void seq_main(void) {
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
        while (snd_seq_event_input(seq_handle, &ev) < 0) {
            std::this_thread::yield();
        }
        if (within_filter(ev)) {
            midi::put_event(ev);
        }
    }
}

}