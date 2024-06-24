#include <csignal>
#include <pthread.h>

#include "global.h"
#include "gui.h"
#include "midi.h"

namespace qdab::main {

int main(int argc, char** argv) {
    signal(SIGINT, global::clean_exit);
    signal(SIGTERM, global::clean_exit);

    pthread_t midi_thread;
    pthread_create(&midi_thread, NULL, midi::midi_main, NULL);
    pthread_detach(midi_thread);

    int gui_result = gui::gui_main();
    
    global::clean_exit(0);
    return gui_result;
}

}

int main(int argc, char **argv) {
    return qdab::main::main(argc, argv);
}