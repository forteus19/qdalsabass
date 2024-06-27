/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#include "modal.h"

#include <cstdio>
#include <unordered_map>

#include "global.h"

namespace qdab::modal {

typedef struct {
    std::string name;
    void (*callback)(void);
    ImVec2 size;
    ImGuiWindowFlags flags;
    bool show;
} modal_t;

static std::unordered_map<int, modal_t> modals;

void add_modal(int id, std::string name, void (*callback)(void), const ImVec2 &size, ImGuiWindowFlags flags) {
    modals[id] = { name, callback, size, flags, false };
}

void show_modal(int id) {
    if (!modals.contains(id)) {
        QDAB_WARN("Invalid modal: {}", id);
        return;
    }
    modals.at(id).show = true;
}

void manage_modals(void) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    for (auto &[id, modal] : modals) {
        if (modal.show) {
            ImGui::OpenPopup(modal.name.c_str());
            modal.show = false;
        }
        ImGui::SetNextWindowSize(modal.size, ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal(modal.name.c_str(), NULL, modal.flags)) {
            modal.callback();
            ImGui::EndPopup();
        }
    }
}

}