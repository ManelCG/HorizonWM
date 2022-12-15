#ifndef __BAR_MODULES_H_
#define __BAR_MODULES_H_

#include <stdlib.h>

enum {BAR_DEFAULT_MODULE, BAR_MODULE_DATE, BAR_MODULE_KEYBOARDMAPPING, BAR_MODULE_BATTERYSTATUS, BAR_MODULE_BRIGHTNESS, BAR_MODULE_VOLUME, BAR_MODULE_UPDATES, BAR_MODULE_OPENVPN, BAR_MODULE_WMMODE, BAR_MODULE_WIRELESS, BAR_MODULE_WIRED, BAR_MODULE_MPC_STATUS, BAR_MODULE_MPC_PREV, BAR_MODULE_MPC_PLAYPAUSE, BAR_MODULE_MPC_STOP, BAR_MODULE_MPC_NEXT};

#define BAR_MODULE_ARGUMENTS int bufsize, char *retstring, void *args, char *color

//Bar module functions:
int date_barmodule(BAR_MODULE_ARGUMENTS);
int keyboard_mapping_barmodule(BAR_MODULE_ARGUMENTS);
int battery_status_barmodule(BAR_MODULE_ARGUMENTS);
int brightness_barmodule(BAR_MODULE_ARGUMENTS);
int volume_barmodule(BAR_MODULE_ARGUMENTS);
int updates_barmodule(BAR_MODULE_ARGUMENTS);
int openvpn_barmodule(BAR_MODULE_ARGUMENTS);
int wm_mode_barmodule(BAR_MODULE_ARGUMENTS);
int wireless_barmodule(BAR_MODULE_ARGUMENTS);
int wired_connection_barmodule(BAR_MODULE_ARGUMENTS);

//MPD:
int mpd_status_barmodule(BAR_MODULE_ARGUMENTS);
int mpd_prev_barmodule(BAR_MODULE_ARGUMENTS);
int mpd_playpause_barmodule(BAR_MODULE_ARGUMENTS);
int mpd_stop_barmodule(BAR_MODULE_ARGUMENTS);
int mpd_next_barmodule(BAR_MODULE_ARGUMENTS);


//On bar module clicked functions:
int volume_clicked(int mask, int button);
int keyboard_mapping_clicked(int mask, int button);
int updates_clicked(int mask, int button);
int brightness_clicked(int mask, int button);

typedef int  (*BarModuleFunction)(BAR_MODULE_ARGUMENTS);
typedef int (*BarModuleButtonFunction)(int, int);

typedef struct BarModule {
  BarModuleFunction       function;
  BarModuleButtonFunction functionOnClick;
  unsigned int id;
  unsigned int period;
  unsigned int width;
} BarModule;

extern BarModule bar_modules[];

#endif //_BAR_MODULES_H_
