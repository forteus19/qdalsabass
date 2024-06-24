#ifndef _QDAB_MODAL_H
#define _QDAB_MODAL_H

#include <string>

#include "imgui.h"

namespace qdab::modal {

void add_modal(int id, std::string name, void (*callback)(void), const ImVec2 &size = ImVec2(0, 0), ImGuiWindowFlags flags = 0);
void show_modal(int id);
void manage_modals(void);

}

#endif