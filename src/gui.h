/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#ifndef _QDAB_GUI_H
#define _QDAB_GUI_H

#include <optional>
#include <string>

namespace qdab::gui {

void add_all_modals(void);

void handle_bass_failure(int error_code);

std::optional<std::string> init_from_settings(void);
void handle_init_all(std::optional<std::string> init_all_out);

bool should_save_settings(void);

int gui_main(void);

}

#endif