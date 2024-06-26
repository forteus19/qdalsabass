/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#include "midi.h"

#include <atomic>
#include <cstdio>
#include <thread>

#include "global.h"
#include "gui.h"
#include "settings.h"

namespace qdab::midi {

static std::atomic<event_t *> ev_buffer = NULL;
static int ev_buffer_size;
static std::atomic<int> ev_rhead = 0;
static std::atomic<int> ev_whead = 0;

bool put_event(event_t *ev) {
    if ((ev_whead + 1) % ev_buffer_size == ev_rhead) {
        return false;
    }
    ev_buffer[ev_whead] = *ev;
    ev_whead = (ev_whead + 1) % ev_buffer_size;
    return true;
}

bool get_event(event_t *ev) {
    if (ev_rhead == ev_whead) {
        return false;
    }
    *ev = ev_buffer[ev_rhead];
    ev_rhead = (ev_rhead + 1) % ev_buffer_size;
    return true;
}

int get_rhead_pos(void) {
    return ev_rhead;
}

int get_whead_pos(void) {
    return ev_whead;
}

std::optional<std::string> init_from_settings(void) {
    std::string output;
    std::optional<std::string> init_bass_out = bass::init_from_settings();
    if (init_bass_out.has_value()) {
        output.append(init_bass_out.value());
    }
    std::optional<std::string> init_seq_out = seq::init_from_settings();
    if (init_seq_out.has_value()) {
        output.append(init_seq_out.value());
    }
    return output;
}

void stop(void) {
    seq::stop();
    bass::stop();
    if (ev_buffer != NULL) {
        delete[] ev_buffer;
    }
}

void init(void) {
    ev_buffer_size = global::settings.ev_buffer_size;
    ev_buffer = new event_t[ev_buffer_size];

    std::thread bass_thread(bass::bass_main);
    bass_thread.detach();
    std::thread seq_thread(seq::seq_main);
    seq_thread.detach();
}

}