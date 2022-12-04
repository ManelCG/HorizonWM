#ifndef __MENU_SCRIPTS_H_
#define __MENU_SCRIPTS_H_

#include <horizonwm_type_definitions.h>

enum {ShowMenuTopRight, ShowMenuFullscreen};

//Menu functions
int show_menu_rofi(int position, const char *prompt, const char *options[]);

//Menu scripts
void menuscripts_powermenu(const Arg *a);

typedef int (*ShowMenuFunction)(int position, const char *prompt, const char *options[]);
// extern ShowMenuFunction menuFunction;  //Unused for now

#endif //_MENU_SCRIPTS_H_
