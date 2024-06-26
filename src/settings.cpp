/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#include "settings.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <unistd.h>

#include "global.h"
#include "gui.h"
#include "midi.h"

namespace qdab::settings {

std::string ss_getenv(const char *name) {
    char *value = getenv(name);
    if (value == NULL) {
        return std::string();
    } else {
        return std::string(value);
    }
}

std::string get_config_home(void) {
    std::string xdg_config_home = ss_getenv("XDG_CONFIG_HOME");
    if (xdg_config_home.empty()) {
        std::string home_dir = ss_getenv("HOME");
        if (home_dir.empty()) {
            home_dir = getpwuid(getuid())->pw_dir;
        }
        return std::format("{}/.config", home_dir);
    } else {
        return xdg_config_home;
    }
}

std::string get_config_filename(void) {
    std::string config_home = get_config_home();
    std::string config_folder = std::format("{}/qdalsabass", config_home);
    if (!std::filesystem::exists(config_folder)) {
        std::filesystem::create_directory(config_folder);
    }

    return std::format("{}/qdalsabass/config.json", config_home);
}

void write_settings(std::string path) {
    nlohmann::json jsettings;

    jsettings["appearance"] = global::settings.appearance;
    jsettings["volume"] = global::settings.volume;
    jsettings["max_voices"] = global::settings.max_voices;
    jsettings["sample_rate"] = global::settings.sample_rate;
    jsettings["ev_buffer_size"] = global::settings.ev_buffer_size;

    nlohmann::json jsoundfonts = nlohmann::json::array();
    for (const auto &[sf_path, sf_config] : global::settings.soundfonts) {
        jsoundfonts.push_back({
            { "path", sf_path },
            { "preset", sf_config.preset },
            { "bank", sf_config.bank }
        });
    }
    jsettings["soundfonts"] = jsoundfonts;

    std::ofstream config_outfile(path);
    if (!config_outfile.is_open()) {
        fprintf(stderr, "Failed to write config file.\n");
        return;
    }
    config_outfile << std::setw(4) << jsettings << std::endl;
}

bool read_settings(std::string path) {
    settings_t settings = settings_t();

    if (!std::filesystem::exists(path)) {
        global::settings = settings_t();
        return true;
    }

    std::ifstream config_infile(path);
    nlohmann::json jsettings;
    try {
        config_infile >> jsettings;

        if (jsettings.contains("appearance") && jsettings["appearance"] >= 0 && jsettings["appearance"] <= 2) {
            global::settings.appearance = jsettings["appearance"];
        }
        if (jsettings.contains("volume") && jsettings["volume"] >= 0.0f && jsettings["volume"] <= 1.0f) {
            global::settings.volume = jsettings["volume"];
        }
        if (jsettings.contains("max_voices")) {
            global::settings.max_voices = std::clamp((int)jsettings["max_voices"], 1, 100000);
        }
        if (jsettings.contains("sample_rate")) {
            global::settings.sample_rate = std::clamp((int)jsettings["sample_rate"], 0, sample_rate_count - 1);
        }
        if (jsettings.contains("ev_buffer_size")) {
            global::settings.ev_buffer_size = std::clamp((int)jsettings["ev_buffer_size"], 1, 262144);
        }

        if (jsettings.contains("soundfonts")) {
            for (auto jsf : jsettings["soundfonts"]) {
                if (!jsf.contains("path")) {
                    continue;
                }
                soundfont_t sf;
                sf.path = jsf["path"];
                if (jsf.contains("preset")) {
                    sf.config.preset = jsf["preset"];
                }
                if (jsf.contains("bank")) {
                    sf.config.bank = jsf["bank"];
                }
                global::settings.soundfonts.push_back(sf);
            }
        }
    } catch (std::exception e) {
        fprintf(stderr, "Parse error: %s\n", e.what());
        return false;
    }

    return true;
}

std::optional<std::string> init_all(void) {
    std::string error_out;

    std::optional<std::string> gui_out = gui::init_from_settings();
    if (gui_out.has_value()) {
        error_out += std::format("GUI: {}\n", gui_out.value());
    }
    std::optional<std::string> midi_out = midi::init_from_settings();
    if (midi_out.has_value()) {
        error_out += std::format("MIDI: {}\n", midi_out.value());
    }

    if (error_out.empty()) {
        return std::nullopt;
    } else {
        return error_out;
    }
}

}