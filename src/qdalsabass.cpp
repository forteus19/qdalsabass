#include <alsa/asoundlib.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <string>

#include "bass.h"
#include "bassmidi.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "portable-file-dialogs.h"
#include <SDL2/SDL.h>

#define SYSEX_GM_RESET "\xF0\x7E\x7F\x09\x01\xF7"

static bool running = true;

static snd_seq_t *seq_handle;
static int seq_port;
static HSTREAM bass_stream;
static HSOUNDFONT bass_soundfont;
static std::string bass_soundfont_path;

static int rpn_params[16];
static int rpn_vals[16];

void clean_exit(int signum) {
    if (!running) {
        return;
    }
    if (bass_soundfont != 0) {
        BASS_MIDI_FontFree(bass_soundfont);
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
    bass_soundfont = 0;

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

int main(int argc, char** argv) {
    pthread_t gui_thread;
    pthread_create(&gui_thread, NULL, midi_main, NULL);
    pthread_detach(gui_thread);

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
        800, 480,
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

    ImGui::StyleColorsLight();

    ImGui_ImplSDL2_InitForSDLRenderer(sdl_window, sdl_renderer);
    ImGui_ImplSDLRenderer2_Init(sdl_renderer);

    bool show_debug_window = false;
    bool show_imgui_demo_window = false;
    
    int max_voices_buf = 1000;

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

        ImGui::Begin("qdalsabass main window", NULL, ImGuiWindowFlags_MenuBar);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Debug window", "", show_debug_window)) {
                    show_debug_window ^= 1;
                }
                if (ImGui::MenuItem("ImGui demo window", "", show_imgui_demo_window)) {
                    show_imgui_demo_window ^= 1;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        ImGui::Text("Draw FPS: %.0f", io.Framerate);
        if (seq_handle != NULL) {
            ImGui::Text("Listening on %d:%d", snd_seq_client_id(seq_handle), seq_port);
        } else {
            ImGui::Text("Waiting...");
        }

        ImGui::Separator();

        if (ImGui::Button("Open soundfont")) {
            pfd::open_file f = pfd::open_file(
                "Choose a soundfont",
                pfd::path::home(),
                {
                    "Soundfonts (sf2, sf2pack, sf3, sfz)", "*.sf2 *.sf2pack *.sf3 *.sfz",
                    "All files", "*"
                }
            );
            if (f.result().empty()) {
                goto cancel_open_sf;
            }
            std::string result = f.result()[0];

            HSOUNDFONT new_bass_soundfont = BASS_MIDI_FontInit(result.c_str(), 0);
            if (new_bass_soundfont == 0) {
                goto cancel_open_sf;
            }
            BASS_MIDI_FONT sf_config;
            sf_config.font = new_bass_soundfont;
            sf_config.preset = -1;
            sf_config.bank = 0;
            BASS_MIDI_StreamSetFonts(bass_stream, &sf_config, 1);
            BASS_MIDI_FontFree(bass_soundfont);
            bass_soundfont = new_bass_soundfont;

            bass_soundfont_path = result.substr(result.rfind("/") + 1);
        }

cancel_open_sf:
        ImGui::Text("Current soundfont: %s", bass_soundfont_path.c_str());

        ImGui::Separator();

        if (ImGui::Button("Apply settings")) {
            BASS_ChannelSetAttribute(bass_stream, BASS_ATTRIB_MIDI_VOICES, (float)max_voices_buf);
        }

        ImGui::InputInt("Max voices", &max_voices_buf);
        if (max_voices_buf > 100000) {
            max_voices_buf = 100000;
        }

        ImGui::End();

        if (!show_debug_window) {
            goto skip_debug_window;
        }

        ImGui::Begin("qdalsabass debug window", &show_debug_window);

        ImGui::Text("Rendering time: %.1f%%", BASS_GetCPU());
        float active_voices;
        BASS_ChannelGetAttribute(bass_stream, BASS_ATTRIB_MIDI_VOICES_ACTIVE, &active_voices);
        ImGui::Text("Active voices: %.0f", active_voices);

        ImGui::End();

skip_debug_window:

        if (show_imgui_demo_window) {
            ImGui::ShowDemoWindow(&show_imgui_demo_window);
        }

        ImGui::Render();
        SDL_RenderSetScale(sdl_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(sdl_renderer, 180, 180, 180, 255);
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