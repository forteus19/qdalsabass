/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#include <csignal>
#include <cstring>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "global.h"
#include "gui.h"
#include "midi.h"
#include "settings.h"

namespace qdab::main {

void handle_args(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            spdlog::set_level(spdlog::level::debug);
        }
        if (strcmp(argv[i], "--trace") == 0) {
            spdlog::set_level(spdlog::level::trace);
        }
    }
}

int main(int argc, char **argv) {
    signal(SIGINT, global::clean_exit);
    signal(SIGTERM, global::clean_exit);

    auto stdout_logger = spdlog::stdout_color_mt("stdout_logger");
    auto stderr_logger = spdlog::stderr_color_mt("stderr_logger");

    stdout_logger->set_pattern("[%Y-%m-%d %T.%e] [%t] [%^%l%$] %v");
    stderr_logger->set_pattern("[%Y-%m-%d %T.%e] [%t] [%^%l%$] (%s:%#) %v");

    handle_args(argc, argv);

    settings::read_settings(settings::get_config_filename());

    gui::add_all_modals();

    midi::init();

    int gui_result = gui::gui_main();

    if (gui::should_save_settings()) {
        settings::write_settings(settings::get_config_filename());
    }
    
    global::clean_exit(0);
    return gui_result;
}

}

int main(int argc, char **argv) {
    return qdab::main::main(argc, argv);
}