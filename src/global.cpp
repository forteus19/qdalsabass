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
    for (auto const &sf : global::settings.soundfonts) {
        if (sf.config.font != 0) {
            midi::free_font(sf.config.font);
        }
    }
    midi::stop_bass();

    global::running = false;
}

}