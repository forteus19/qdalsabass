/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#include "global.h"

#include "midi.h"

namespace qdab::global {

bool running = true;
settings::settings_t settings;

const char *get_qdab_version(void) {
    return "0.1-DEV";
}

void clean_exit(int signum) {
    if (!global::running) {
        return;
    }
    QDAB_INFO("Exiting normally.");
    for (auto const &sf : global::settings.soundfonts) {
        if (sf.config.font != 0) {
            midi::free_font(sf.config.font);
        }
    }
    midi::stop();

    global::running = false;
}

}