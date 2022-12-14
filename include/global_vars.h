#ifndef __GLOBAL_VARS_H_
#define __GLOBAL_VARS_H_

#include <pthread.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <stdbool.h>

#define COLOR_ENABLED   "#6bf799"
#define COLOR_DISABLED  "#Ff0367"
#define COLOR_WARNING   "#F7ec4c"

enum {WMModeNormal, WMModeDraw};
extern int wm_mode;

enum {MPDStopped, MPDPlaying, MPDPaused};
extern int mpd_status;
extern char mpd_song[128];
extern char mpd_songduration[64];
extern char mpd_percentage[8];

extern int n_updates_pacman;
extern int n_updates_aur;

extern bool is_ethernet_connected;
extern bool is_wifi_connected;
extern char wifi_ssid[128];

extern bool checking_updates;

extern int keyboard_mapping;
extern const char *keyboard_mappings[];

extern bool shall_fetch_updates;

extern pthread_mutex_t mutex_mpc;
extern pthread_mutex_t mutex_drawbar;
extern pthread_mutex_t mutex_fetchupdates;
extern pthread_mutex_t mutex_connection_checker;

extern Display *dpy;

#endif //_GLOBAL_VARS_H_
