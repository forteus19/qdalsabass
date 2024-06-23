#define QDALSABASS_VERSION "0.1-DEV"

#include <alsa/asoundlib.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <string>
#include <vector>

#include "bass.h"
#include "bassmidi.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "portable-file-dialogs.h"
#include <SDL2/SDL.h>

#include "font.h"

#define SYSEX_GM_RESET "\xF0\x7E\x7F\x09\x01\xF7"

typedef struct {
    std::string path;
    BASS_MIDI_FONT config;
} loaded_soundfont_t;

static bool running = true;

static snd_seq_t *seq_handle;
static int seq_port;
static HSTREAM bass_stream;
static std::vector<loaded_soundfont_t> soundfonts;

static int rpn_params[16];
static int rpn_vals[16];

static bool show_about_modal = false;
static bool show_sf_error_modal = false;

void clean_exit(int signum) {
    if (!running) {
        return;
    }
    for (auto const &sf : soundfonts) {
        if (sf.config.font != 0) {
            BASS_MIDI_FontFree(sf.config.font);
        }
    }
    if (BASS_IsStarted() != 0) {
        BASS_Free();
    }

    running = false;
}

void stream_alsa_event(snd_seq_event_t *ev) {
    switch (ev->type) {
        case SND_SEQ_EVENT_NOTEON: BASS_MIDI_StreamEvent(bass_stream, ev->data.note.channel, MIDI_EVENT_NOTE, MAKEWORD(ev->data.note.note, (int)ev->data.note.velocity)); break;
        case SND_SEQ_EVENT_NOTEOFF: BASS_MIDI_StreamEvent(bass_stream, ev->data.note.channel, MIDI_EVENT_NOTE, ev->data.note.note); break;
        case SND_SEQ_EVENT_PGMCHANGE: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PROGRAM, ev->data.control.value); break;
        case SND_SEQ_EVENT_CHANPRESS: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_CHANPRES, ev->data.control.value); break;
        case SND_SEQ_EVENT_KEYPRESS: BASS_MIDI_StreamEvent(bass_stream, ev->data.note.channel, MIDI_EVENT_KEYPRES, MAKEWORD(ev->data.note.note, (int)ev->data.note.velocity)); break;
        case SND_SEQ_EVENT_PITCHBEND: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PITCH, ev->data.control.value + 8192); break;

    case SND_SEQ_EVENT_CONTROLLER:
        switch (ev->data.control.param) {
            case 0: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_BANK, ev->data.control.value); break;
            case 1: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_MODULATION, ev->data.control.value); break;
            case 5: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PORTATIME, ev->data.control.value); break;
            case 7: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_VOLUME, ev->data.control.value); break;
            case 10: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PAN, ev->data.control.value); break;
            case 11: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_EXPRESSION, ev->data.control.value); break;
            case 32: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_BANK_LSB, ev->data.control.value); break;
            case 64: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_SUSTAIN, ev->data.control.value); break;
            case 65: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PORTAMENTO, ev->data.control.value); break;
            case 66: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_SOSTENUTO, ev->data.control.value); break;
            case 67: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_SOFT, ev->data.control.value); break;
            case 71: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_RESONANCE, ev->data.control.value); break;
            case 72: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_RELEASE, ev->data.control.value); break;
            case 73: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_ATTACK, ev->data.control.value); break;
            case 74: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_CUTOFF, ev->data.control.value); break;
            case 75: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_DECAY, ev->data.control.value); break;
            case 76: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_VIBRATO_RATE, ev->data.control.value); break;
            case 77: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_VIBRATO_DEPTH, ev->data.control.value); break;
            case 78: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_VIBRATO_DELAY, ev->data.control.value); break;
            case 84: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PORTANOTE, ev->data.control.value); break;
            case 91: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_REVERB, ev->data.control.value); break;
            case 93: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_CHORUS, ev->data.control.value); break;
            case 94: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_USERFX, ev->data.control.value); break;
            case 100: rpn_params[ev->data.control.channel & 0x0F] = (rpn_params[ev->data.control.channel & 0x0F] & 0xFF00) | (ev->data.control.value & 0xFF); break;
            case 101: rpn_params[ev->data.control.channel & 0x0F] = (rpn_params[ev->data.control.channel & 0x0F] & 0x00FF) | ((ev->data.control.value & 0xFF) << 8); break;
            case 120: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_SOUNDOFF, ev->data.control.value); break;
            case 121: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_RESET, ev->data.control.value); break;
            case 123: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_NOTESOFF, ev->data.control.value); break;
            case 126: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_MODE, 1); break;
            case 127: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_MODE, 0); break;

        case 6:
            switch (rpn_params[ev->data.control.channel]) {
                case 0: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_PITCHRANGE, ev->data.control.value); break;
                case 2: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_COARSETUNE, ev->data.control.value); break;
                default: rpn_vals[ev->data.control.channel & 0x0F] = (rpn_vals[ev->data.control.channel & 0x0F] & 0xFF00) | (ev->data.control.value & 0xFF); break;
            }
            break;
        
        case 38:
            switch (rpn_params[ev->data.control.channel]) {
                case 1: BASS_MIDI_StreamEvent(bass_stream, ev->data.control.channel, MIDI_EVENT_FINETUNE, ev->data.control.value | (rpn_vals[ev->data.control.channel & 0x0F] << 7)); break;
            }
            break;
        }
        break;

    case SND_SEQ_EVENT_SYSEX:
        {
            if (ev->data.ext.len != 6) {
                break;
            }
            if (strncmp((char *)ev->data.ext.ptr, SYSEX_GM_RESET, 6) == 0) {
                BASS_MIDI_StreamEvent(bass_stream, 0, MIDI_EVENT_SYSTEMEX, MIDI_SYSTEM_GS);
            }
        }
        break;
    }

}

void log_alsa_event(snd_seq_event_t *ev) {
    switch (ev->type) {
        case SND_SEQ_EVENT_NOTEON: printf("[%d] NOTEON key: %d vel: %d\n", ev->data.note.channel, ev->data.note.note, ev->data.note.velocity); break;
        case SND_SEQ_EVENT_NOTEOFF: printf("[%d] NOTEOFF key: %d\n", ev->data.note.channel, ev->data.note.note); break;
        case SND_SEQ_EVENT_CONTROLLER: printf("[%d] CC param: %d value: %d\n", ev->data.control.channel, ev->data.control.param, ev->data.control.value); break;
        case SND_SEQ_EVENT_NONREGPARAM: printf("[%d] NRPN param: %d value: %d\n", ev->data.control.channel, ev->data.control.param, ev->data.control.value); break;
        case SND_SEQ_EVENT_REGPARAM: printf("[%d] RPN param: %d value: %d\n", ev->data.control.channel, ev->data.control.param, ev->data.control.value); break;
        case SND_SEQ_EVENT_SYSEX: {
            printf("[G] SYSEX len: %d data: ", ev->data.ext.len);
            for (int i = 0; i < ev->data.ext.len; i++) {
                printf("0x%02X ", ((uint8_t *)ev->data.ext.ptr)[i]);
            }
            printf("\n");
        } break;
    }
}

void *midi_main(void *data) {
    if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
        fprintf(stderr, "BASS version mismatch");
        clean_exit(0);
        return NULL;
    }

    BASS_Init(-1, 44100, 0, NULL, NULL);

    bass_stream = BASS_MIDI_StreamCreate(16, BASS_SAMPLE_FLOAT | BASS_MIDI_ASYNC, 1);

    BASS_ChannelSetAttribute(bass_stream, BASS_ATTRIB_VOL, 0.75);
    BASS_ChannelSetAttribute(bass_stream, BASS_ATTRIB_BUFFER, 0);
    BASS_ChannelSetAttribute(bass_stream, BASS_ATTRIB_MIDI_VOICES, 1000);
    BASS_ChannelFlags(bass_stream, BASS_MIDI_NOFX, BASS_MIDI_NOFX);

    BASS_MIDI_StreamEvent(bass_stream, 0, MIDI_EVENT_SYSTEM, MIDI_SYSTEM_GS);

    BASS_ChannelPlay(bass_stream, FALSE);

    snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);
    snd_seq_set_client_name(seq_handle, "qdalsabass");

    seq_port = snd_seq_create_simple_port(
        seq_handle,
        "qdalsabass input port",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION
    );

    while (running) {
        snd_seq_event_t *ev = NULL;
        if (snd_seq_event_input(seq_handle, &ev) >= 0) {
            stream_alsa_event(ev);
            // log_alsa_event(ev);
        }
    }

    return NULL;
}

void prompt_soundfont(void) {
    pfd::open_file f = pfd::open_file(
        "Choose a soundfont",
        pfd::path::home(),
        {
            "Soundfonts (sf2, sf2pack, sf3, sfz)", "*.sf2 *.sf2pack *.sf3 *.sfz",
            "All files", "*"
        }
    );
    if (f.result().empty()) {
        return;
    }
    std::string result = f.result()[0];
    
    BASS_MIDI_FONT sf_config;
    sf_config.font = BASS_MIDI_FontInit(result.c_str(), 0);
    if (sf_config.font == 0) {
        fprintf(stderr, "prompt_soundfont() BASS error: %d\n", BASS_ErrorGetCode());
        show_sf_error_modal = true;
        return;
    }
    sf_config.preset = -1;
    sf_config.bank = 0;

    soundfonts.push_back({ result, sf_config });
    return;
}

void reload_soundfonts(void) {
    BASS_MIDI_FONT fonts[soundfonts.size()];
    for (int i = 0; i < soundfonts.size(); i++) {
        fonts[i] = soundfonts[i].config;
    }

    if (!BASS_MIDI_StreamSetFonts(bass_stream, fonts, soundfonts.size())) {
        fprintf(stderr, "reload_soundfonts() BASS error: %d\n", BASS_ErrorGetCode());
        show_sf_error_modal = true;
    }
}

int main(int argc, char** argv) {
    signal(SIGINT, clean_exit);
    signal(SIGTERM, clean_exit);

    pthread_t midi_thread;
    pthread_create(&midi_thread, NULL, midi_main, NULL);
    pthread_detach(midi_thread);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        clean_exit(0);
        return 1;
    }

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    SDL_Window *sdl_window = SDL_CreateWindow(
        "qdalsabass window",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640, 480,
        SDL_WINDOW_RESIZABLE
    );
    if (sdl_window == NULL) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        clean_exit(0);
        return 1;
    }

    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(
        sdl_window, -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
    );
    if (sdl_renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        clean_exit(0);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = NULL;
    
    ImFont *imfont = io.Fonts->AddFontFromMemoryCompressedTTF(font_compressed_data, font_compressed_size, 17);
    ImFont *imfont_big = io.Fonts->AddFontFromMemoryCompressedTTF(font_compressed_data, font_compressed_size, 17 * 2);

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(sdl_window, sdl_renderer);
    ImGui_ImplSDLRenderer2_Init(sdl_renderer);
    
    int max_voices_buf = 1000;
    int selected_soundfont_row = -1;

    while (running) {
        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event)) {
            ImGui_ImplSDL2_ProcessEvent(&sdl_event);
            if (sdl_event.type == SDL_QUIT) {
                running = false;
            }
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::PushFont(imfont);

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
                if (ImGui::MenuItem("Load config from...")) {}
                if (ImGui::MenuItem("Save config as...")) {}

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::BeginMenu("Appearance")) {
                    if (ImGui::MenuItem("Dark mode")) {
                        ImGui::StyleColorsDark();
                    }
                    if (ImGui::MenuItem("Light mode")) {
                        ImGui::StyleColorsLight();
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
                    show_about_modal = true;
                }

                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }

        if (ImGui::BeginTabBar("mainBar")) {
            if (ImGui::BeginTabItem("Info")) {
                ImGui::Text("Draw FPS: %.0f", io.Framerate);
                if (seq_handle != NULL) {
                    ImGui::Text("Listening on %d:%d", snd_seq_client_id(seq_handle), seq_port);

                    ImGui::Separator();

                    ImGui::Text("Rendering time: %.1f%%", BASS_GetCPU());
                    float active_voices;
                    BASS_ChannelGetAttribute(bass_stream, BASS_ATTRIB_MIDI_VOICES_ACTIVE, &active_voices);
                    ImGui::Text("Active voices: %.0f", active_voices);
                } else {
                    ImGui::Text("Waiting...");
                }

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Settings")) {
                if (ImGui::Button("Apply settings")) {
                    BASS_ChannelSetAttribute(bass_stream, BASS_ATTRIB_MIDI_VOICES, (float)max_voices_buf);
                }

                ImGui::InputInt("Max voices", &max_voices_buf);
                if (max_voices_buf > 100000) {
                    max_voices_buf = 100000;
                }

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Soundfonts")) {
                if (ImGui::Button("Add soundfont")) {
                    prompt_soundfont();
                    reload_soundfonts();
                }
                ImGui::SameLine();
                ImGui::BeginDisabled(selected_soundfont_row < 0);
                if (ImGui::Button("Remove soundfont")) {
                    BASS_MIDI_FontFree(soundfonts[selected_soundfont_row].config.font);
                    soundfonts.erase(soundfonts.begin() + selected_soundfont_row);
                    reload_soundfonts();
                    selected_soundfont_row = -1;
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                if (ImGui::Button("Reload soundfonts")) {
                    reload_soundfonts();
                }
                ImGui::BeginDisabled(selected_soundfont_row < 1);
                ImGui::SameLine();
                if (ImGui::Button("Move up")) {
                    std::swap(soundfonts[selected_soundfont_row], soundfonts[selected_soundfont_row - 1]);
                    reload_soundfonts();
                    selected_soundfont_row--;
                }
                ImGui::EndDisabled();
                ImGui::BeginDisabled(selected_soundfont_row >= (soundfonts.size() - 1));
                ImGui::SameLine();
                if (ImGui::Button("Move down")) {
                    std::swap(soundfonts[selected_soundfont_row], soundfonts[selected_soundfont_row + 1]);
                    reload_soundfonts();
                    selected_soundfont_row++;
                }
                ImGui::EndDisabled();

                int dummy_preset_val = -1;
                int dummy_bank_val = 0;

                ImGui::BeginDisabled(selected_soundfont_row < 0);
                ImGui::InputInt("Preset", selected_soundfont_row < 0 ? &dummy_preset_val : &soundfonts[selected_soundfont_row].config.preset);
                ImGui::InputInt("Bank", selected_soundfont_row < 0 ? &dummy_bank_val : &soundfonts[selected_soundfont_row].config.bank);
                ImGui::EndDisabled();

                if (ImGui::BeginTable("soundfontsTable", 3, ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 0.8f);
                    ImGui::TableSetupColumn("Preset", ImGuiTableColumnFlags_None, 0.1f);
                    ImGui::TableSetupColumn("Bank", ImGuiTableColumnFlags_None, 0.1f);
                    ImGui::TableHeadersRow();
                    int row = 0;
                    for (auto const &sf : soundfonts) {
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
            ImGui::EndTabBar();
        }
        
        ImGui::End();

        if (show_about_modal) {
            ImGui::OpenPopup("About qdalsabass");
            show_about_modal = false;
        }
        if (show_sf_error_modal) {
            ImGui::OpenPopup("Soundfont error");
            show_sf_error_modal = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("About qdalsabass", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::PushFont(imfont_big);
            ImGui::Text("qdalsabass");
            ImGui::PopFont();

            ImGui::Spacing();

            ImGui::Text("Created by forteus19");
            ImGui::Text("Version: " QDALSABASS_VERSION);
            ImGui::Text("License: GPL-3.0");

            ImGui::Separator();

            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("Soundfont error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to load soundfont");

            ImGui::Separator();

            ImGui::SetItemDefaultFocus();
            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PopFont();

        ImGui::Render();
        SDL_RenderSetScale(sdl_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
        SDL_RenderClear(sdl_renderer);

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), sdl_renderer);
        SDL_RenderPresent(sdl_renderer);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
    
    clean_exit(0);
    return 0;
}