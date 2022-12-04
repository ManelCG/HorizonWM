#include <menu_scripts.h>
#include <spawn_programs.h>
#include <helper_scripts.h>

#include <string.h>

void menuscripts_powermenu(const Arg *a){
  const char *prompt = "PowerMenu";
  const char *menu_main[]       = {"Session", "Net Utilities", "Programs", "Misc", "Close", NULL};

  const char *menu_session[]    = {"Lock Screen", "Exit to TTY", "Power Off", "Reboot", "Back", NULL};
  const char *menu_TTY[]        = {"Exit to TTY", "Stay in HorizonWM", NULL};
  const char *menu_poweroff[]   = {"Power Off", "Power Offn't", NULL};
  const char *menu_reboot[]     = {"Reboot", "Rebootn't", NULL};

  const char *menu_net[]        = {"Connect to VPN", "Enable Hotspot", "Back", NULL};

  Arg arg;

  int selection;

  main_menu:
    selection = show_menu_rofi(ShowMenuTopRight, prompt, menu_main);
    switch(selection){
      case 0:
        goto session_menu;
        break;
      case 1:
        goto network_menu;
        break;
      case 2:
        arg.v = rofibarcmd;
        spawn(&arg);
        goto end;
        break;
      default:
        goto end;
        break;
    }
    goto end;

  network_menu:
    selection = show_menu_rofi(ShowMenuTopRight, prompt, menu_net);
    switch(selection){
      case 0:
        //TODO VPN
        goto end;
        break;
      case 1:
        //TODO HOTSPOT
        goto end;
        break;
      default:
        goto main_menu;
        break;
    }
    goto end;

  session_menu:
    selection = show_menu_rofi(ShowMenuTopRight, prompt, menu_session);
    switch(selection){
      case 0:
        arg.v = lockscreencmd;
        spawn(&arg);
        goto end;
        break;
      case 1:
        goto tty_menu;
        break;
      case 2:
        goto poweroff_menu;
        break;
      case 3:
        goto reboot_menu;
        break;
      default:
        goto main_menu;
        break;
    }
    goto end;

  tty_menu:
    selection = show_menu_rofi(ShowMenuTopRight, prompt, menu_TTY);
    switch(selection){
      case 0:
        //TODO QUIT
        goto end;
        break;
      default:
        goto session_menu;
        break;
    }
    goto end;

  poweroff_menu:
    selection = show_menu_rofi(ShowMenuTopRight, prompt, menu_poweroff);
    switch(selection){
      case 0:
        arg.v = poweroffcmd;
        spawn(&arg);
        break;
      default:
        goto session_menu;
        break;
    }
    goto end;

  reboot_menu:
    selection = show_menu_rofi(ShowMenuTopRight, prompt, menu_reboot);
    switch(selection){
      case 0:
        arg.v = rebootcmd;
        spawn(&arg);
        break;
      default:
        goto session_menu;
        break;
    }
    goto end;

  end:
      return;
}

int show_menu_rofi(int position, const char *prompt, const char *options[]){
  size_t optslen = 0;
  int numopts;
  Arg a;
  const char *selected_config = "";
  switch(position){
    case ShowMenuTopRight:
      selected_config = ROFIBARCNFG;
      break;
    case ShowMenuFullscreen:
      selected_config = ROFIFULLCNFG;
      break;
  }

  const char *rofi_command[] = {"rofi", "-dmenu", "-format", "i", "-config", selected_config, "-p", prompt, NULL};

  for (numopts = 0; options[numopts] != NULL; numopts++){
    optslen += strlen(options[numopts]) + 1;
  }

  if (optslen == 0){
    return -1;
  }

  char buffer[optslen];
  buffer[0] = '\0';

  for (int i = 0; i < numopts; i++){
    strcat(buffer, options[i]);
    strcat(buffer, "\n");
  }

  buffer[optslen -1] = '\0';

  a.v = rofi_command;
  return spawn_readint_feedstdin(&a, buffer);
}


