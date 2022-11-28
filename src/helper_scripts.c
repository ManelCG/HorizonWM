#include <helper_scripts.h>
#include <horizonwm_type_definitions.h>
#include <spawn_programs.h>

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

void scripts_set_keyboard_mapping(const Arg *a){
  const char *mapping = a->v;

  const char *cmd[] = {"setxkbmap", mapping, NULL};
  Arg arg;
  arg.v = cmd;

  spawn(&arg);
}

void scripts_take_screenshot(const Arg *a){
  DIR *d;
  struct dirent *dir;

  int nfiles = 1;

  time_t rawtime;
  struct tm *timeinfo;

  int scrot_pid;
  int scrot_status = -1;

  char *month = "";
  char bufname[128];
  char bufpath[256];
  char bufnot[512] = "Error taking screenshot";

  d = opendir(SCREENSHOT_DIR);

  while ((dir = readdir(d)) != NULL){
    if (strcmp(".", dir->d_name) != 0 &&
        strcmp("..", dir->d_name) != 0){
      nfiles++;
    }
  }

  closedir(d);

  time (&rawtime);
  timeinfo = localtime(&rawtime);

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


  // Without seconds
  snprintf(bufname, 127, "%04d__%02d-%s-%04d__%02d:%02d.png",
           nfiles + 1,
           timeinfo->tm_mday,
           month,
           timeinfo->tm_year+1900,
           timeinfo->tm_hour,
           timeinfo->tm_min);
  snprintf(bufpath, 255, "%s/%s", SCREENSHOT_DIR, bufname);
  snprintf(bufnot, 500, "Took screenshot with name\n%s", bufname);

  if (a->i == 0){
    const char *cmd[] = {"scrot", bufpath, NULL};
    Arg arg;
    arg.v = cmd;
    scrot_pid = spawn_pid(&arg);

    waitpid(scrot_pid, &scrot_status, 0);
  } else if (a->i == 1){
    const char *cmd[] = {"scrot", "-s", bufpath, NULL};
    Arg arg;
    arg.v = cmd;
    scrot_pid = spawn_pid(&arg);

    waitpid(scrot_pid, &scrot_status, 0);
  } else {
    return;
  }

  if (scrot_status == 0){
    const char *notify[] = {"notify-send", bufnot, NULL};
    Arg arg;
    arg.v = notify;
    spawn(&arg);
  }
}
