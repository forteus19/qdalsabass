#ifndef _QDAB_GUI_H
#define _QDAB_GUI_H

#include <optional>
#include <string>

namespace qdab::gui {

void add_all_modals(void);

void handle_bass_failure(int error_code);

std::optional<std::string> init_from_settings(void);
void handle_init_all(std::optional<std::string> init_all_out);

int gui_main(void);

}

#endif