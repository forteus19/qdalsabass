/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#include <csignal>
#include <pthread.h>

#include "global.h"
#include "gui.h"
#include "midi.h"
#include "settings.h"

namespace qdab::main {

int main(int argc, char** argv) {
    signal(SIGINT, global::clean_exit);
    signal(SIGTERM, global::clean_exit);

    settings::read_settings(settings::get_config_filename());

    gui::add_all_modals();

    pthread_t midi_thread;
    pthread_create(&midi_thread, NULL, midi::midi_main, NULL);
    pthread_detach(midi_thread);

    int gui_result = gui::gui_main();

    settings::write_settings(settings::get_config_filename());
    
    global::clean_exit(0);
    return gui_result;
}

}

int main(int argc, char **argv) {
    return qdab::main::main(argc, argv);
}