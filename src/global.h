/*
    qdalsabass: ALSA sequencer implementation for BASSMIDI.
    Copyright (C) 2024  forteus19

    See NOTICE.txt for the full notice.
*/

#ifndef _QDAB_GLOBAL_H
#define _QDAB_GLOBAL_H

#include "settings.h"

namespace qdab::global {

extern bool running;
extern settings::settings_t settings;

const char *get_qdab_version(void);

void clean_exit(int signum);

}

#endif