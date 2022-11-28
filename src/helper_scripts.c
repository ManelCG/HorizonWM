#include <helper_scripts.h>
#include <horizonwm_type_definitions.h>
#include <spawn_programs.h>

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

void scripts_take_screenshot(Arg *a){
  DIR *d;
  struct dirent *dir;
  int nfiles = 1;
  time_t rawtime;
  struct tm *timeinfo;
  char *month = "";
  char bufname[256];
  char bufnot[512];

  d = opendir(SCREENSHOT_DIR);

  while ((dir = readdir(d)) != NULL){
    if (strcmp(".", dir->d_name) != 0 &&
        strcmp("..", dir->d_name) != 0){
      nfiles++;
    }
  }

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
  snprintf(bufname, 255, "%s/%04d__%02d-%s-%04d__%02d:%02d.png",
           SCREENSHOT_DIR,
           nfiles + 1,
           timeinfo->tm_mday,
           month,
           timeinfo->tm_year+1900,
           timeinfo->tm_hour,
           timeinfo->tm_min);
  snprintf(bufnot, 500, "Took screenshot with name\n%s", bufname);

  if (a->i == 0){
    const char *cmd[] = {"scrot", bufname, NULL};
    Arg arg;
    arg.v = cmd;
    spawn(&arg);
  } else if (a->i == 1){
    const char *cmd[] = {"scrot", "-s", bufname, NULL};
    Arg arg;
    arg.v = cmd;
    spawn(&arg);
  }

  const char *notify[] = {"notify-send", bufnot, NULL};
  Arg arg;
  arg.v = notify;
  spawn(&arg);
}
