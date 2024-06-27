/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#ifndef _QDAB_GLOBAL_H
#define _QDAB_GLOBAL_H

#include <spdlog/spdlog.h>

#define QDAB_TRACE(...) spdlog::get("stdout_logger")->trace(__VA_ARGS__)
#define QDAB_DEBUG(...) spdlog::get("stdout_logger")->debug(__VA_ARGS__)
#define QDAB_INFO(...) spdlog::get("stdout_logger")->info(__VA_ARGS__)
#define QDAB_WARN(...) spdlog::get("stdout_logger")->warn(__VA_ARGS__)
#define QDAB_ERROR(...) spdlog::get("stdout_logger")->error(__VA_ARGS__)
#define QDAB_CRIT(...) SPDLOG_LOGGER_CRITICAL(spdlog::get("stderr_logger"), __VA_ARGS__)

#include "settings.h"

namespace qdab::global {

extern bool running;
extern settings::settings_t settings;

const char *get_qdab_version(void);

void clean_exit(int signum);

}

#endif