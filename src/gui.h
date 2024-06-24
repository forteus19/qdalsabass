#ifndef _QDAB_GUI_H
#define _QDAB_GUI_H

#include <optional>
#include <string>

namespace qdab::gui {

bool is_initialized(void);

std::optional<std::string> init_from_settings(void);

int gui_main(void);

}

#endif