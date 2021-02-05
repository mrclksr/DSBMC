/*-
 * Copyright (c) 2019 Marcel Kaiser. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include "lib/config.h"

#define DEFAULT_WIDTH        300
#define DEFAULT_HEIGHT       300
#define DFLT_CMD_PLAY_CDDA   DSBCFG_VAL("vlc cdda://%d")
#define DFLT_CMD_PLAY_DVD    DSBCFG_VAL("vlc dvd://%d")
#define DFLT_CMD_PLAY_VCD    DSBCFG_VAL("vlc vcd://%d")
#define DFLT_CMD_PLAY_SVCD   DSBCFG_VAL("vlc vcd://%d")
#define DFLT_CMD_FILEMANAGER DSBCFG_VAL("xdg-open \"%m\"")

dsbcfg_vardef_t vardefs[] = {
  { "win_pos_x",   DSBCFG_VAR_INTEGER, CFG_POS_X,       DSBCFG_VAL(0)        },
  { "win_pos_y",   DSBCFG_VAR_INTEGER, CFG_POS_Y,       DSBCFG_VAL(0)        },
  { "win_width",   DSBCFG_VAR_INTEGER, CFG_WIDTH,       DSBCFG_VAL(346)      },
  { "win_height",  DSBCFG_VAR_INTEGER, CFG_HEIGHT,      DSBCFG_VAL(408)      },
  { "filemanager", DSBCFG_VAR_STRING,  CFG_FILEMANAGER, DFLT_CMD_FILEMANAGER },
  { "play_cdda",   DSBCFG_VAR_STRING,  CFG_PLAY_CDDA,   DFLT_CMD_PLAY_CDDA   },
  { "play_dvd",    DSBCFG_VAR_STRING,  CFG_PLAY_DVD,    DFLT_CMD_PLAY_DVD    },
  { "play_vcd",    DSBCFG_VAR_STRING,  CFG_PLAY_VCD,    DFLT_CMD_PLAY_VCD    },
  { "play_svcd",   DSBCFG_VAR_STRING,  CFG_PLAY_SVCD,   DFLT_CMD_PLAY_SVCD   },
  { "tray_icon",   DSBCFG_VAR_STRING,  CFG_TRAY_ICON,   DSBCFG_VAL("")       },
  { "dvd_auto",    DSBCFG_VAR_BOOLEAN, CFG_DVD_AUTO,    DSBCFG_VAL(false)    },
  { "vcd_auto",    DSBCFG_VAR_BOOLEAN, CFG_VCD_AUTO,    DSBCFG_VAL(false)    },
  { "svcd_auto",   DSBCFG_VAR_BOOLEAN, CFG_SVCD_AUTO,   DSBCFG_VAL(false)    },
  { "cdda_auto",   DSBCFG_VAR_BOOLEAN, CFG_CDDA_AUTO,   DSBCFG_VAL(false)    },
  { "show_msgwin", DSBCFG_VAR_BOOLEAN, CFG_MSGWIN,      DSBCFG_VAL(true)     },
  { "popup",       DSBCFG_VAR_BOOLEAN, CFG_POPUP,       DSBCFG_VAL(false)    },
  { "hide_on_open",DSBCFG_VAR_BOOLEAN, CFG_HIDE_ON_OPEN,DSBCFG_VAL(true)     },
  { "ignore",      DSBCFG_VAR_STRINGS, CFG_HIDE,        DSBCFG_VAL((char **)NULL) },
  { "tray_theme",  DSBCFG_VAR_STRING,  CFG_TRAY_THEME,  DSBCFG_VAL((char *)0) }
};
