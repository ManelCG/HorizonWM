#include <bar_modules.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <helper_scripts.h>
#include <spawn_programs.h>
#include <stdbool.h>
#include <global_vars.h>

#define BATTERY_STATUS_FOLDER "/sys/class/power_supply/BAT"
#define AC_ADAPTER_FOLDER     "/sys/class/power_supply/AC"
#define BRIGHTNESS_FOLDER     "/sys/class/backlight/amdgpu_bl1"

#define BATTERY_HEALTHY 0
#define BATTERY_LOW 1
#define BATTERY_CRITICAL 2

void setmpcstatus(const Arg *arg_unused); //In horizonwm, static

int battery_status = 0;

int n_updates_pacman = 0;
int n_updates_aur = 0;

int old_updates = 0;

extern BarModule bar_modules[];
BarModule bar_modules[] = {
    //module function               onClick function              module ID (can be 0)    period of module (s)   pixel width on bar
    {date_barmodule,                NULL,                         BAR_MODULE_DATE,              0,                0},
    {keyboard_mapping_barmodule,    keyboard_mapping_clicked,     BAR_MODULE_KEYBOARDMAPPING,   0,                0},
    {battery_status_barmodule,      NULL,                         BAR_MODULE_BATTERYSTATUS,     0,                0},
    {wired_connection_barmodule,    NULL,                         BAR_MODULE_WIRED,             0,                0},
    {wireless_barmodule,            NULL,                         BAR_MODULE_WIRELESS,          0,                0},
    {brightness_barmodule,          brightness_clicked,           BAR_MODULE_BRIGHTNESS,        0,                0},
    {volume_barmodule,              volume_clicked,               BAR_MODULE_VOLUME,            0,                0},

    {openvpn_barmodule,             NULL,                         BAR_MODULE_OPENVPN,           0,                0},

    {wm_mode_barmodule,             NULL,                         BAR_MODULE_WMMODE,            0,                0},

    {updates_barmodule,             updates_clicked,              BAR_MODULE_UPDATES,           1,                0},

    //MPD
    {mpd_next_barmodule,            mpd_next_clicked,             BAR_MODULE_MPC,               0,                0},
    {mpd_stop_barmodule,            mpd_stop_clicked,             BAR_MODULE_MPC,               0,                0},
    {mpd_playpause_barmodule,       mpd_playpause_clicked,        BAR_MODULE_MPC,               0,                0},
    {mpd_prev_barmodule,            mpd_prev_clicked,             BAR_MODULE_MPC,               0,                0},
    {mpd_status_barmodule,          mpd_status_clicked,           BAR_MODULE_MPC,               0,                0},

    {NULL, NULL, 0, 0, 0}
};

int wired_connection_barmodule(BAR_MODULE_ARGUMENTS){
  bool is_con;

  pthread_mutex_lock(&mutex_connection_checker);
  is_con = is_ethernet_connected;
  pthread_mutex_unlock(&mutex_connection_checker);


  if (!is_con){
    strcpy(color, COLOR_DISABLED);
    strcpy(retstring, "");
    return -1;
  }

  strcpy(retstring, "");
  return 0;
}

int wireless_barmodule(BAR_MODULE_ARGUMENTS){
  bool is_con;

  pthread_mutex_lock(&mutex_connection_checker);
  is_con = is_wifi_connected;
  pthread_mutex_unlock(&mutex_connection_checker);

  if (!is_con){
    strcpy(color, COLOR_DISABLED);
    strcpy(retstring, "");
    return -1;
  }

  snprintf(retstring, bufsize, " %s", wifi_ssid);
  return 0;
}

int mpd_prev_barmodule(BAR_MODULE_ARGUMENTS){
  int localstatus;

  pthread_mutex_lock(&mutex_mpc);
  localstatus = mpd_status;
  pthread_mutex_unlock(&mutex_mpc);

  if (localstatus == MPDPlaying || localstatus == MPDPaused){
    strcpy(retstring, " ");
    strcpy(color, "#dbdbdb");
  } else if (localstatus == MPDStopped){
    return -1;
  }

  return 0;
}

int mpd_playpause_barmodule(BAR_MODULE_ARGUMENTS){
  int localstatus;

  pthread_mutex_lock(&mutex_mpc);
  localstatus = mpd_status;
  pthread_mutex_unlock(&mutex_mpc);

  if (localstatus == MPDPlaying){
    strcpy(retstring, " ");
    strcpy(color, "#dbdbdb");
  } else if (localstatus == MPDPaused){
    strcpy(retstring, " ");
    strcpy(color, "#dbdbdb");
  } else if (localstatus == MPDStopped){
    return -1;
  }

  return 0;
}

int mpd_stop_barmodule(BAR_MODULE_ARGUMENTS){
  int localstatus;

  pthread_mutex_lock(&mutex_mpc);
  localstatus = mpd_status;
  pthread_mutex_unlock(&mutex_mpc);

  if (localstatus == MPDPlaying || localstatus == MPDPaused){
    strcpy(retstring, " ");
    strcpy(color, "#dbdbdb");
  } else if (localstatus == MPDStopped){
    return -1;
  }

  return 0;
}

int mpd_next_barmodule(BAR_MODULE_ARGUMENTS){
  int localstatus;

  pthread_mutex_lock(&mutex_mpc);
  localstatus = mpd_status;
  pthread_mutex_unlock(&mutex_mpc);

  if (localstatus == MPDPlaying || localstatus == MPDPaused){
    strcpy(retstring, " ");
    strcpy(color, "#dbdbdb");
  } else if (localstatus == MPDStopped){
    return -1;
  }

  return 0;
}


int mpd_status_barmodule(BAR_MODULE_ARGUMENTS){
  int localstatus;
  char localsong[128];
  char localperc[8];
  char localdur[64];
  char progressbar[128];

  pthread_mutex_lock(&mutex_mpc);
  localstatus = mpd_status;
  strcpy(localsong, mpd_song);
  strcpy(localperc, mpd_percentage);
  strcpy(localdur, mpd_songduration);
  pthread_mutex_unlock(&mutex_mpc);

  int progress = atoi(localperc);

  percentage_to_progressbar(progressbar, progress, 20);

  if (localstatus == MPDPlaying || localstatus == MPDPaused){
    snprintf(retstring, bufsize, " %s   %s   %s", localsong, progressbar, localdur);
    strcpy(color, "#dbdbdb");
  }

  return 0;
}

int wm_mode_barmodule(BAR_MODULE_ARGUMENTS){
  if (wm_mode == WMModeDraw){
    strcpy(retstring, "");
  }
  return 0;
}

int openvpn_barmodule(BAR_MODULE_ARGUMENTS){
  Arg a;
  char ovpn_status[32];

  a.v = sysctl_status_ovpn;
  spawn_catchoutput(&a, ovpn_status, 31);

  if (strncmp(ovpn_status, "active", 6) == 0){
    snprintf(retstring, bufsize, " VPN");
    strcpy(color, COLOR_ENABLED);
  }

  return 0;
}

int updates_barmodule(BAR_MODULE_ARGUMENTS){
  int n_updates_pacman_local;
  int n_updates_aur_local;
  bool checking_updates_local;

  //Use mutex just to read global variables so we lock for the least time possible
  pthread_mutex_lock(&mutex_fetchupdates);
  n_updates_pacman_local = n_updates_pacman;
  n_updates_aur_local = n_updates_aur;
  checking_updates_local = checking_updates;
  pthread_mutex_unlock(&mutex_fetchupdates);

  if (checking_updates_local){
    snprintf(retstring, bufsize, "%d  %d", n_updates_pacman_local, n_updates_aur_local);
  }
  else if (!shall_fetch_updates || n_updates_pacman_local > 0 || n_updates_aur_local > 0){
    snprintf(retstring, bufsize, "%d  %d", n_updates_pacman_local, n_updates_aur_local);
  }

  if (checking_updates_local){
    strcpy(color, COLOR_ENABLED);
  } else if (shall_fetch_updates){
    strcpy(color, COLOR_WARNING);
  } else {
    strcpy(color, COLOR_DISABLED);
  }

  if (n_updates_pacman_local + n_updates_aur_local > old_updates){
    char buf[128];
    snprintf(buf, 127, "%d updates pending\n%d from AUR", n_updates_pacman_local, n_updates_aur_local);
    notify_send("Updates detected!", buf);
  }

  old_updates = n_updates_pacman_local + n_updates_aur_local;
  return 0;
}
int updates_clicked(int mask, int button){
  Arg a;
  switch(button){
    case 1:
      async_check_updates_handler(NULL);
      return 0;
    case 2:
      a.v = updatearchlinuxcmd;
      spawn(&a);
      return 0;
    case 3:
      toggle_update_checks(NULL);
      return 0;
    default:
      return -1;
  }
}

int mpd_status_clicked(int mask, int button){
  char *buffer;
  char songname[128];
  char buffer_title[256];
  switch(button){
    case 1:
      buffer = ncmpcpp_get_current_song_lyrics(songname, sizeof(songname)-1);
      if (buffer != NULL){
        snprintf(buffer_title, sizeof(buffer_title)-1, "Lyrics for %s", songname);
        notify_send_timeout(buffer_title, buffer, 0);
        free(buffer);
        return 0;
      }
      return -1;
    case 3:
      buffer = mpc_get_playlist();
      if (buffer != NULL){
        notify_send_timeout("Current playlist:", buffer, 0);
        free(buffer);
        return 0;
      }
      return -1;
    default:
      return -1;
  }
}
int mpd_prev_clicked(int mask, int button){
  Arg a;
  switch(button){
    case 1:
      a.v = mpc_prev;
      spawn_waitpid(&a);
      setmpcstatus(NULL);
      return 0;
    default:
      return -1;
  }
}
int mpd_playpause_clicked(int mask, int button){
  Arg a;
  switch(button){
    case 1:
      a.v = mpc_toggle;
      spawn_waitpid(&a);
      setmpcstatus(NULL);
      return 0;
    default:
      return -1;
  }
}
int mpd_stop_clicked(int mask, int button){
  Arg a;
  switch(button){
    case 1:
      a.v = mpc_stop;
      spawn_waitpid(&a);
      setmpcstatus(NULL);
      return 0;
    default:
      return -1;
  }
}
int mpd_next_clicked(int mask, int button){
  Arg a;
  switch(button){
    case 1:
      a.v = mpc_next;
      spawn_waitpid(&a);
      setmpcstatus(NULL);
      return 0;
    default:
      return -1;
  }
}

int volume_clicked(int mask, int button){
  Arg a;
  switch(button){
    case 1:
      a.v = mutevolume;
      spawn_waitpid(&a);
      return 0;
    case 4:
      a.v = upvolumecmd;
      spawn_waitpid(&a);
      return 0;
    case 5:
      a.v = downvolume;
      spawn_waitpid(&a);
      return 0;
    default:
      return -1;
      break;
  }
}
int volume_barmodule(BAR_MODULE_ARGUMENTS){
  char buffer_pamixeroutput[8];
  char *icon = "";

  const char *getvolume_cmd[] = {"pamixer", "--get-volume", NULL };
  Arg getvolume_arg = {.v = getvolume_cmd};

  const char *getmute_cmd[] = {"pamixer", "--get-mute", NULL };
  Arg getmute_arg = {.v = getmute_cmd};

  //Get volume pctg
  spawn_catchoutput(&getvolume_arg, buffer_pamixeroutput, 8);
  int percent = atoi(buffer_pamixeroutput);

  //Get mute state
  spawn_catchoutput(&getmute_arg, buffer_pamixeroutput, 8);
  if (buffer_pamixeroutput[0] == 't'){ //If volume is muted
    icon = "";
    strcpy(color, COLOR_DISABLED);
  } else {
    if (percent >= 70){
      icon = "";
    } else if (percent >= 40){
      icon = "";
    } else {
      icon = "";
    }
  }

  char progressbar[DEFAULT_PROGRESS_BAR_SIZE];
  progressbar[0] = '\0';
  percentage_to_progressbar(progressbar, percent, DEFAULT_PROGRESS_BAR_LEN);
  sprintf(retstring, "%s  %s %3d%%", icon, progressbar, percent);

  return 0;
}

int keyboard_mapping_clicked(int mask, int button){
  switch(button){
    case 1:
      switch_keyboard_mapping();
      return 0;
    default:
      return -1;
  }
}
int keyboard_mapping_barmodule(BAR_MODULE_ARGUMENTS){
  // int **a = (int **) args;
  // int kbd = *a[0];
  snprintf(retstring, bufsize, " %s", keyboard_mappings[keyboard_mapping]);
  return 0;
}

int brightness_clicked(int mask, int button){
  Arg a;
  switch(button){
    case 1:
      a.v = monitoroffcmd;
      spawn_waitpid(&a);
      return 0;
    case 4:
      a.v = upbrightnesscmd;
      spawn_waitpid(&a);
      return 0;
    case 5:
      a.v = downbrightnesscmd;
      spawn_waitpid(&a);
      return 0;
    default:
      return -1;
      break;
  }
}
int brightness_barmodule(BAR_MODULE_ARGUMENTS){
  int file_buffer_len = strlen(BRIGHTNESS_FOLDER) + 32;
  char brightnessfile_buffer[file_buffer_len];
  char max_brightnessfile_buffer[file_buffer_len];

  snprintf(brightnessfile_buffer, file_buffer_len, "%s/brightness", BRIGHTNESS_FOLDER);
  snprintf(max_brightnessfile_buffer, file_buffer_len, "%s/max_brightness", BRIGHTNESS_FOLDER);

  int brightness = read_file_int(brightnessfile_buffer);
  int max_brightness = read_file_int(max_brightnessfile_buffer);
  int percent = 100 * brightness / max_brightness;

  char progressbar[DEFAULT_PROGRESS_BAR_SIZE];
  progressbar[0] = '\0';
  percentage_to_progressbar(progressbar, percent, DEFAULT_PROGRESS_BAR_LEN);
  sprintf(retstring, "  %s %3d%%", progressbar, percent);

  return 0;
}

int battery_status_barmodule(BAR_MODULE_ARGUMENTS){
  int selected_battery = 0;
  int folderbufsize = strlen(BATTERY_STATUS_FOLDER) + 32;
  char battery_folder_buf[folderbufsize];
  char file_buf[folderbufsize + 32];

  char AC_online_file_buf[strlen(AC_ADAPTER_FOLDER) + 32];

  float charge_max;
  float charge_now;
  float percentf;
  int percent;

  int oldstatus;

  char *symbol;

  _Bool is_charging;

  snprintf(battery_folder_buf, folderbufsize, "%s%d", BATTERY_STATUS_FOLDER, selected_battery);

  snprintf(file_buf, folderbufsize + 32, "%s/%s", battery_folder_buf, "charge_now");
  charge_now = read_file_float(file_buf);

  snprintf(file_buf, folderbufsize + 32, "%s/%s", battery_folder_buf, "charge_full");
  charge_max = read_file_float(file_buf);

  snprintf(AC_online_file_buf, strlen(AC_ADAPTER_FOLDER) + 32, "%s%d/online", AC_ADAPTER_FOLDER, 0);
  is_charging = read_file_int(AC_online_file_buf)? true : false;

  //If battery is at 99.something, show it as 100.
  percentf = 100*charge_now/charge_max;
  if (percentf > 99){
    percent = 100;
  } else {
    percent = (int) percentf;
  }

  oldstatus = battery_status;

  //        
  if (percent >= 90){
    symbol = "";
    battery_status = BATTERY_HEALTHY;
  } else if (percent >= 70){
    symbol = " ";
    battery_status = BATTERY_HEALTHY;
  } else if (percent >= 40){
    symbol = " ";
    battery_status = BATTERY_HEALTHY;
  } else if (percent > 15){
    symbol = " ";
    battery_status = BATTERY_HEALTHY;
  } else if (percent > 5){
    battery_status = BATTERY_LOW;
    symbol = "";
  } else {
    battery_status = BATTERY_CRITICAL;
    symbol = "";
  }

  //Notify when significant battery drop
  if (oldstatus < battery_status && !is_charging){
    char percent_str[16];
    snprintf(percent_str, 15, "%d%%", percent);
    switch(battery_status){
      case BATTERY_LOW:
        notify_send_critical("Battery low!", percent_str);
        break;
      case BATTERY_CRITICAL:
        notify_send_timeout_critical("Battery critical!", percent_str, 0); //Timeout of 0 means persistent
        break;
    }
  }

  if (is_charging){
    strcpy(color, COLOR_ENABLED);
    battery_status = BATTERY_HEALTHY;         //This is just used so if you unplug charger, you get a warning of battery level
  } else if (battery_status >= BATTERY_LOW){
    strcpy(color, COLOR_DISABLED);
  }

  ////Show progressbar on battery
  //char battery_progressbar[DEFAULT_PROGRESS_BAR_LEN * 3];
  //battery_progressbar[0] = '\0';
  //percentage_to_progressbar(battery_progressbar, percent, DEFAULT_PROGRESS_BAR_LEN);
  //sprintf(retstring, "%s %s %d%%", symbol, battery_progressbar, percent);

  //Don't show progressbar on battery
  sprintf(retstring, "%s %d %%", symbol, percent);

  return 0;
}

int date_barmodule(BAR_MODULE_ARGUMENTS){
  time_t rawtime;
  struct tm *timeinfo;
  char *weekday = "";
  char *month = "";

  time (&rawtime);
  timeinfo = localtime(&rawtime);

  switch(timeinfo->tm_wday){
    case 1: weekday = "Mon"; break;
    case 2: weekday = "Tue"; break;
    case 3: weekday = "Wed"; break;
    case 4: weekday = "Thu"; break;
    case 5: weekday = "Fri"; break;
    case 6: weekday = "Sat"; break;
    case 7: weekday = "Sun"; break;
  }

  switch(timeinfo->tm_mon){
    case 0: month = "Jan"; break;
    case 1: month = "Feb"; break;
    case 2: month = "Mar"; break;
    case 3: month = "Apr"; break;
    case 4: month = "May"; break;
    case 5: month = "Jun"; break;
    case 6: month = "Jul"; break;
    case 7: month = "Aug"; break;
    case 8: month = "Sept"; break;
    case 9: month = "Oct"; break;
    case 10: month = "Nov"; break;
    case 11: month = "Dec"; break;
  }

  //Bar with seconds
  // sprintf(retstring, " %s %02d/%s/%d    %02d:%02d:%02d", weekday, timeinfo->tm_mday, month, timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  // Without seconds
  sprintf(retstring, " %s %02d %s %d    %02d:%02d", weekday, timeinfo->tm_mday, month, timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min);
  return 0;
}
