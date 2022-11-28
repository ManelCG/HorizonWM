#ifndef __SPAWN_PROGRAMS_H_
#define __SPAWN_PROGRAMS_H_

#include <stdlib.h>
#include <horizonwm_type_definitions.h>

//PROGRAM FLAGS
#define PROGRAM_RUN_STARTUP     0b0000000000000001
#define PROGRAM_RERUN_RESTART   0b0000000000000010

typedef struct ProgramService {
  const char **cmd;
  unsigned int flags;
  unsigned int pid;
} ProgramService;


void spawn_programs_list(ProgramService *l);
void spawn(const Arg *arg);
unsigned int spawn_pid(const Arg *arg);

/* commands */
static const char *roficmd[] = { "rofi", "-show", "drun", NULL};
static const char *termcmd[]  = { "alacritty", NULL };
static const char *browsercmd[]  = { "brave", NULL };
static const char *picomcmd[] = {"picom", NULL};
static const char *claviscmd[] = {"clavis", NULL};
static const char *calculatorcmd[] = {"gnome-calculator", NULL};

static const char *dunstcmd[] = {"dunst", NULL};
static const char *configkeyboardcmd[] = {"xset", "r", "rate", "300", "50", NULL };
static const char *wallpapercmd[] = {WALLPAPERCMD, "bg", NULL};

//Monitor brightness
static const char *downbrightnesscmd[] = {"brightnessctl", "s", "5%-", NULL};
static const char *upbrightnesscmd[] = {"brightnessctl", "s", "5%+", NULL};
static const char *monitoroffcmd[] = {"xset", "dpms", "force", "off", NULL};

//Volume
static const char *upvolumecmd[] = {"pamixer", "-i", "5", NULL};
static const char *downvolume[]  = {"pamixer", "-d", "5", NULL};
static const char *mutevolume[]  = {"pamixer", "-t", NULL};

//Keyboard brightness
static const char *KBdownbrightnesscmd[] = {"brightnessctl", "-q", "-d='asus::kbd_backlight'", "s", "1-", NULL};
static const char *KBupbrightnesscmd[] = {"brightnessctl", "-q", "-d='asus::kbd_backlight'", "s", "1+", NULL};

// static const char **startup_programs[] = {
static ProgramService startup_programs[] = {
  {picomcmd, 0, 0},
  {wallpapercmd, 0, 0},
  {dunstcmd, 0, 0},
  {configkeyboardcmd, 0, 0},
  {0, 0, 0}
};

#endif //_SPAWN_PROGRAMS_H_
