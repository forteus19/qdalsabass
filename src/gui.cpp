/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#include "gui.h"

#include <algorithm>
#include <format>

#include "font.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "portable-file-dialogs.h"
#include <SDL2/SDL.h>

#include "global.h"
#include "midi.h"
#include "modal.h"

namespace qdab::gui {

static SDL_Window *sdl_window;
static SDL_Renderer *sdl_renderer;

static bool initialized = false;
static bool save_settings = true;

static ImFont *imfont;
static ImFont *imfont_big;
static ImFont *imfont_mono;

static std::string config_error_modal_msg;
static int bass_failed_modal_msg;

static int selected_soundfont_row = -1;

enum modal_id_ {
    ABOUT_MODAL_ID = 0,
    BASS_ERROR_MODAL_ID = 1,
    CONFIG_ERROR_MODAL_ID = 2,
    BASS_FAILED_MODAL_ID = 3
};

void add_all_modals(void) {
    void show_about_modal(void);
    void show_bass_error_modal(void);
    void show_config_error_modal(void);
    void show_bass_failed_modal(void);

    modal::add_modal(ABOUT_MODAL_ID, "About qdalsabass", show_about_modal, ImVec2(320, 0));
    modal::add_modal(BASS_ERROR_MODAL_ID, "BASS error", show_bass_error_modal, ImVec2(320, 0));
    modal::add_modal(CONFIG_ERROR_MODAL_ID, "Config error", show_config_error_modal, ImVec2(320, 0));
    modal::add_modal(BASS_FAILED_MODAL_ID, "BASS fatal error", show_bass_failed_modal, ImVec2(320, 0));
}

void handle_bass_failure(int error_code) {
    bass_failed_modal_msg = error_code;
    modal::show_modal(BASS_FAILED_MODAL_ID);
}

int prompt_soundfont(void) {
    pfd::open_file f = pfd::open_file(
        "Choose a soundfont",
        pfd::path::home(),
        {
            "Soundfonts (sf2, sf2pack, sf3, sfz)", "*.sf2 *.sf2pack *.sf3 *.sfz",
            "All files", "*"
        }
    );
    if (f.result().empty()) {
        return 0;
    }
    return midi::add_soundfont(f.result()[0], -1, 0);
}

bool prompt_load_config(void) {
    pfd::open_file f = pfd::open_file(
        "Choose a config file",
        pfd::path::home(),
        {
            "JSON files (json)", "*.json",
            "All files", "*"
        }
    );
    if (f.result().empty()) {
        return true;
    }
    return settings::read_settings(f.result()[0]);
}

void prompt_save_config(void) {
    pfd::save_file f = pfd::save_file(
        "Choose a config file",
        pfd::path::home(),
        {
            "JSON files (json)", "*.json",
            "All files", "*"
        }
    );
    if (f.result().empty()) {
        return;
    }
    settings::write_settings(f.result());
}

void set_theme_from_settings(void) {
    switch (global::settings.appearance) {
        default:
        case global::settings.APPEARANCE_DARK: ImGui::StyleColorsDark(); break;
        case global::settings.APPEARANCE_LIGHT: ImGui::StyleColorsLight(); break;
        case global::settings.APPEARANCE_CLASSIC: ImGui::StyleColorsClassic(); break;
    }
}

std::optional<std::string> init_from_settings(void) {
    if (initialized) {
        set_theme_from_settings();
    }
    return std::nullopt;
}

void handle_init_all(std::optional<std::string> init_all_out) {
    if (init_all_out.has_value()) {
        config_error_modal_msg = init_all_out.value();
        modal::show_modal(CONFIG_ERROR_MODAL_ID);
    }
}

bool should_save_settings(void) {
    return save_settings;
}

void warning_marker(const char *text) {
    ImGui::TextDisabled("(!)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool slider_percent(const char *label, float *v, float v_min, float v_max, const char *format = "%.0f%%", ImGuiSliderFlags flags = 0) {
    float perc_value = *v * 100;
    bool result = ImGui::SliderFloat(label, &perc_value, v_min * 100.0f, v_max * 100.0f, format, flags);
    *v = perc_value / 100.0f;
    return result;
}

bool v_slider_percent(const char *label, const ImVec2 &size, float *v, float v_min, float v_max, const char *format = "%.0f%%", ImGuiSliderFlags flags = 0) {
    float perc_value = *v * 100;
    bool result = ImGui::VSliderFloat(label, size, &perc_value, v_min * 100.0f, v_max * 100.0f, format, flags);
    *v = perc_value / 100.0f;
    return result;
}

void show_about_modal(void) {
    ImGui::PushFont(imfont_big);
    ImGui::Text("qdalsabass");
    ImGui::PopFont();

    ImGui::Spacing();

    ImGui::Text("Created by forteus19");
    ImGui::Text("Version: %s", global::get_qdab_version());
    ImGui::Text("License: GPL-3.0");

    ImGui::Separator();

    if (ImGui::Button("Close")) {
        ImGui::CloseCurrentPopup();
    }
}

void show_bass_error_modal(void) {
    ImGui::Text("BASS error code: %d", midi::get_bass_error());
    ImGui::Spacing();
    ImGui::TextWrapped("Continuing execution can result in errors and crashes. It is recommended to restart the program.");

    ImGui::Separator();

    if (ImGui::Button("Ignore")) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Exit")) {
        ImGui::CloseCurrentPopup();
        global::clean_exit(0);
    }
}

void show_config_error_modal(void) {
    ImGui::Text("An error occured while parsing the config.");

    ImGui::Spacing();

    ImGui::Text("Errors:");
    ImGui::TextWrapped(config_error_modal_msg.c_str());
    
    ImGui::Separator();

    if (ImGui::Button("Close")) {
        ImGui::CloseCurrentPopup();
    }
}

void show_bass_failed_modal(void) {
    ImGui::Text("BASS failed to initialize.");
    ImGui::Spacing();
    ImGui::TextWrapped("This could be because of your settings (try editing ~/.config/qdalsabass/config.json).");
    ImGui::Spacing();
    ImGui::TextWrapped("If you are still unable to launch successfully, create an issue on GitHub including your config.json file and the error code and you should receive support.");
    ImGui::Spacing();
    ImGui::TextWrapped("BASS error: %d", bass_failed_modal_msg);

    ImGui::Separator();

    if (ImGui::Button("Exit")) {
        ImGui::CloseCurrentPopup();
        global::clean_exit(0);
    }
}

void draw_gui(void) {
    int wx, wy;
    SDL_GetWindowSize(sdl_window, &wx, &wy);

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(wx, wy));
    ImGui::Begin(
        "qdalsabass main window",
        NULL,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_MenuBar
    );

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load config from...")) {
                if (!prompt_load_config()) {
                    config_error_modal_msg = "Failed to parse JSON file.";
                    modal::show_modal(CONFIG_ERROR_MODAL_ID);
                } else {
                    handle_init_all(settings::init_all());
                }
            }
            if (ImGui::MenuItem("Save config as...")) {
                prompt_save_config();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset settings")) {
                settings::reset();
                handle_init_all(settings::init_all());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                global::clean_exit(0);
            }
            if (ImGui::MenuItem("Exit without saving settings")) {
                save_settings = false;
                global::clean_exit(0);
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::BeginMenu("Appearance")) {
                if (ImGui::MenuItem("Dark")) {
                    ImGui::StyleColorsDark();
                    global::settings.appearance = global::settings.APPEARANCE_DARK;
                }
                if (ImGui::MenuItem("Light")) {
                    ImGui::StyleColorsLight();
                    global::settings.appearance = global::settings.APPEARANCE_LIGHT;
                }
                if (ImGui::MenuItem("Classic")) {
                    ImGui::StyleColorsClassic();
                    global::settings.appearance = global::settings.APPEARANCE_CLASSIC;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("GitHub")) {
                system("xdg-open https://github.com/forteus19/qdalsabass");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("About qdalsabass")) {
                modal::show_modal(ABOUT_MODAL_ID);
            }

            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }

    if (ImGui::BeginTabBar("mainBar")) {
        if (ImGui::BeginTabItem("General")) {
            ImGui::Text("Draw FPS: %.0f", ImGui::GetIO().Framerate);
            if (midi::is_ready()) {
                ImGui::Text("Listening on %d:%d", midi::get_client_num(), midi::get_port_num());

                ImGui::Separator();

                ImGui::Text("Rendering time: %.1f%%", midi::get_cpu());
                ImGui::Text("Active voices: %d", midi::get_active_voices());
                ImGui::Text("RH/WH position:");
                ImGui::SameLine();
                ImGui::PushFont(imfont_mono);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%04X/%04X", midi::get_rhead_pos(), midi::get_whead_pos());
                ImGui::PopFont();

                ImGui::Separator();

                if (ImGui::Button("Send GM Reset")) {
                    midi::gm_reset();
                }
            } else {
                ImGui::Text("Waiting...");
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Soundfonts")) {
            if (ImGui::Button("Add soundfont")) {
                if (prompt_soundfont() != 0) {
                    modal::show_modal(BASS_ERROR_MODAL_ID);
                }
                if (midi::reload_soundfonts() != 0) {
                    modal::show_modal(BASS_ERROR_MODAL_ID);
                }
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(selected_soundfont_row < 0);
            if (ImGui::Button("Remove soundfont")) {
                midi::free_font(global::settings.soundfonts[selected_soundfont_row].config.font);
                global::settings.soundfonts.erase(global::settings.soundfonts.begin() + selected_soundfont_row);
                if (midi::reload_soundfonts() != 0) {
                    modal::show_modal(BASS_ERROR_MODAL_ID);
                }
                selected_soundfont_row = -1;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Reload soundfonts")) {
                if (midi::reload_soundfonts() != 0) {
                    modal::show_modal(BASS_ERROR_MODAL_ID);
                }
            }
            ImGui::BeginDisabled(selected_soundfont_row < 1);
            ImGui::SameLine();
            if (ImGui::Button("Move up")) {
                std::swap(global::settings.soundfonts[selected_soundfont_row], global::settings.soundfonts[selected_soundfont_row - 1]);
                if (midi::reload_soundfonts() != 0) {
                    modal::show_modal(BASS_ERROR_MODAL_ID);
                }
                selected_soundfont_row--;
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(selected_soundfont_row >= (global::settings.soundfonts.size() - 1));
            ImGui::SameLine();
            if (ImGui::Button("Move down")) {
                std::swap(global::settings.soundfonts[selected_soundfont_row], global::settings.soundfonts[selected_soundfont_row + 1]);
                if (midi::reload_soundfonts() != 0) {
                    modal::show_modal(BASS_ERROR_MODAL_ID);
                }
                selected_soundfont_row++;
            }
            ImGui::EndDisabled();

            int dummy_preset_val = -1;
            int dummy_bank_val = 0;

            ImGui::BeginDisabled(selected_soundfont_row < 0);
            ImGui::InputInt("Preset", selected_soundfont_row < 0 ? &dummy_preset_val : &global::settings.soundfonts[selected_soundfont_row].config.preset);
            ImGui::InputInt("Bank", selected_soundfont_row < 0 ? &dummy_bank_val : &global::settings.soundfonts[selected_soundfont_row].config.bank);
            ImGui::EndDisabled();

            if (ImGui::BeginTable("soundfontsTable", 3, ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 0.8f);
                ImGui::TableSetupColumn("Preset", ImGuiTableColumnFlags_None, 0.1f);
                ImGui::TableSetupColumn("Bank", ImGuiTableColumnFlags_None, 0.1f);
                ImGui::TableHeadersRow();
                int row = 0;
                for (auto const &sf : global::settings.soundfonts) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(sf.path.c_str(), selected_soundfont_row == row, ImGuiSelectableFlags_SpanAllColumns)) {
                        selected_soundfont_row = row;
                    }
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", sf.config.preset);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", sf.config.bank);
                    row++;
                }
                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        } else {
            selected_soundfont_row = -1;
        }
        if (ImGui::BeginTabItem("Settings")) {
            ImGui::SeparatorText("Non-live settings");

            ImGui::Combo("Sample rate", &global::settings.sample_rate, settings::sample_rate_strs, settings::sample_rate_count);

            ImGui::InputInt("Event buffer size", &global::settings.ev_buffer_size);
            global::settings.ev_buffer_size = std::clamp(global::settings.ev_buffer_size, 128, 65536);
            ImGui::SameLine();
            warning_marker("Changing this setting is dangerous. Make sure you know what you are doing!");

            ImGui::SeparatorText("Live settings");

            if (slider_percent("Master volume", &global::settings.volume, 0.0f, 1.0f)) {
                midi::set_volume(global::settings.volume);
            }

            if (ImGui::SliderFloat("Max rendering time", &global::settings.max_rendering_time, 0.0f, 100.0f, global::settings.max_rendering_time == 0.0f ? "Automatic" : "%.0f%%")) {
                midi::set_max_cpu(global::settings.max_rendering_time);
            }

            if (ImGui::InputInt("Max voices", &global::settings.max_voices)) {
                global::settings.max_voices = std::clamp(global::settings.max_voices, 1, 100000);
                midi::set_max_voices(global::settings.max_voices);
            }

            ImGui::Checkbox("Enable ignore range", &global::settings.ignore_range.enable);

            ImGui::BeginDisabled(!global::settings.ignore_range.enable);
            ImGui::DragIntRange2("Ignore range", &global::settings.ignore_range[0], &global::settings.ignore_range[1], 0.2f, 1, 127);
            ImGui::EndDisabled();

            if (ImGui::Checkbox("Enable effects", &global::settings.enable_effects)) {
                midi::set_effects(global::settings.enable_effects);
            }

            if (ImGui::Checkbox("Release oldest note", &global::settings.release_oldest_note)) {
                midi::set_ron(global::settings.release_oldest_note);
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Mixer")) {
            if (ImGui::Button("Set all to 100%")) {
                for (int i = 0; i < 16; i++) {
                    global::settings.channel_volumes[i] = 1.0f;
                    midi::set_channel_volume(i, 1.0f);
                }
            }

            if (ImGui::BeginTable("mixerTable", 8, ImGuiTableFlags_Borders, ImVec2(-1.0f, 0.0f))) {
                for (int i = 0; i < 8; i++) {
                    ImGui::TableNextColumn();
                    if (v_slider_percent(std::format("#{}##vol{}", i + 1, i).c_str(), ImVec2(40, 160), &global::settings.channel_volumes[i], 0.0f, 1.0f)) {
                        midi::set_channel_volume(i, global::settings.channel_volumes[i]);
                    }
                }
                ImGui::TableNextRow();
                for (int i = 8; i < 16; i++) {
                    ImGui::TableNextColumn();
                    if (v_slider_percent(std::format("#{}##vol{}", i + 1, i).c_str(), ImVec2(40, 160), &global::settings.channel_volumes[i], 0.0f, 1.0f)) {
                        midi::set_channel_volume(i, global::settings.channel_volumes[i]);
                    }
                }

                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Limiter")) {
            if (ImGui::Checkbox("Enable limiter", &global::settings.limiter.enable)) {
                midi::set_limiter(global::settings.limiter.enable);
            }
            ImGui::BeginDisabled(!global::settings.limiter.enable);

            bool changed = false;
            changed |= ImGui::SliderFloat("Gain", &global::settings.limiter.gain, -60.0f, 60.0f, "%.1f dB", ImGuiSliderFlags_Logarithmic);
            changed |= ImGui::SliderFloat("Threshold", &global::settings.limiter.threshold, -60.0f, 0.0f, "%.1f dB", ImGuiSliderFlags_Logarithmic);
            changed |= ImGui::InputFloat("Ratio", &global::settings.limiter.ratio, 0.0f, 0.0f, "%.2f:1");
            changed |= ImGui::InputFloat("Attack", &global::settings.limiter.attack, 0.0f, 0.0f, global::settings.limiter.attack < 1.0f ? global::settings.limiter.attack < 0.1f ? "%.2fms" : "%.1fms" : "%.0fms");
            changed |= ImGui::InputFloat("Release", &global::settings.limiter.release, 0.0f, 0.0f, global::settings.limiter.release < 1.0f ? global::settings.limiter.release < 0.1f ? "%.2fms" : "%.1fms" : "%.0fms");
            if (changed) {
                global::settings.limiter.attack = std::clamp(global::settings.limiter.attack, 0.01f, 1000.0f);
                global::settings.limiter.ratio = std::max(global::settings.limiter.ratio, 1.0f);
                global::settings.limiter.release = std::clamp(global::settings.limiter.release, 0.01f, 5000.0f);
                midi::update_limiter_settings();
            }
            
            ImGui::EndDisabled();

            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

int gui_main(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        QDAB_CRIT("SDL_Init() failed!");
        QDAB_CRIT("Error: {}", SDL_GetError());
        global::clean_exit(0);
        return 1;
    }

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    sdl_window = SDL_CreateWindow(
        "qdalsabass window",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640, 480,
        SDL_WINDOW_RESIZABLE
    );
    if (sdl_window == NULL) {
        QDAB_CRIT("SDL_CreateWindow() failed!");
        QDAB_CRIT("Error: {}", SDL_GetError());
        global::clean_exit(0);
        return 1;
    }

    sdl_renderer = SDL_CreateRenderer(
        sdl_window, -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
    );
    if (sdl_renderer == NULL) {
        QDAB_CRIT("SDL_CreateRenderer() failed!");
        QDAB_CRIT("Error: {}", SDL_GetError());
        global::clean_exit(0);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    initialized = true;

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = NULL;
    
    imfont = io.Fonts->AddFontFromMemoryCompressedTTF(font::font_compressed_data, font::font_compressed_size, 17);
    imfont_big = io.Fonts->AddFontFromMemoryCompressedTTF(font::font_compressed_data, font::font_compressed_size, 17 * 2);
    imfont_mono = io.Fonts->AddFontDefault();

    ImGui_ImplSDL2_InitForSDLRenderer(sdl_window, sdl_renderer);
    ImGui_ImplSDLRenderer2_Init(sdl_renderer);

    init_from_settings();

    while (global::running) {
        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event)) {
            ImGui_ImplSDL2_ProcessEvent(&sdl_event);
            if (sdl_event.type == SDL_QUIT) {
                global::clean_exit(0);
            }
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::PushFont(imfont);

        draw_gui();

        modal::manage_modals();

        ImGui::PopFont();

        ImGui::Render();
        SDL_RenderSetScale(sdl_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
        SDL_RenderClear(sdl_renderer);

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), sdl_renderer);
        SDL_RenderPresent(sdl_renderer);
    }

    initialized = false;

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return 0;
}

}