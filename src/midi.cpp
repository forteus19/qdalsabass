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

int get_client_num(void) {
    return seq::get_client_num();
}

int get_port_num(void) {
    return seq::get_port_num();
}

bool is_ready(void) {
    return seq::is_ready();
}

float get_cpu(void) {
    return bass::get_cpu();
}

int get_active_voices(void) {
    return bass::get_active_voices();
}

void set_volume(float volume) {
    return bass::set_volume(volume);
}

void set_max_voices(int voices) {
    return bass::set_max_voices(voices);
}

int add_soundfont(std::string path, int preset, int bank) {
    return bass::add_soundfont(path, preset, bank);
}

bool init_soundfonts(void) {
    return bass::init_soundfonts();
}

int reload_soundfonts(void) {
    return bass::reload_soundfonts();
}

void free_font(int font) {
    return bass::free_font(font);
}

int get_bass_error(void) {
    return bass::get_bass_error();
}

void gm_reset(void) {
    return bass::gm_reset();
}

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