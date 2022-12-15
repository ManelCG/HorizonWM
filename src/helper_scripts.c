#include <helper_scripts.h>
#include <horizonwm_type_definitions.h>
#include <spawn_programs.h>
#include <global_vars.h>

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

float read_file_float(const char *file){
  char buf[128];
  int fd = open(file, O_RDONLY);
  read(fd, buf, 128);
  close(fd);
  return atof(buf);
}
int read_file_int(const char *file){
  char buf[128];
  int fd = open(file, O_RDONLY);
  read(fd, buf, 128);
  close(fd);
  return atoi(buf);
}

void switch_keyboard_mapping(){
  const char *map = keyboard_mappings[0];
  int num = 0;
  Arg arg;

  while (map != NULL){
    num++;
    map = keyboard_mappings[num];
  }

  keyboard_mapping = (keyboard_mapping + 1) % num;

  arg.v = keyboard_mappings[keyboard_mapping];
  scripts_set_keyboard_mapping(&arg);
}
void scripts_set_keyboard_mapping(const Arg *a){
  const char *mapping = a->v;

  const char *cmd[] = {"setxkbmap", mapping, NULL};
  Arg arg;
  arg.v = cmd;

  spawn(&arg);
}

void notify_send(const char *title, const char *text){
  const char *cmd[] = {"notify-send", title, text, NULL};
  Arg a;
  a.v = cmd;
  spawn(&a);
}
void notify_send_critical(const char *title, const char *text){
  const char *cmd[] = {"notify-send", "-u", "Critical", title, text, NULL};
  Arg a;
  a.v = cmd;
  spawn(&a);
}
void notify_send_timeout(const char *title, const char *text, int timeout){
  char timeout_str[16];
  snprintf(timeout_str, 16, "%d", timeout);
  const char *cmd[] = {"notify-send", "-t", timeout_str, title, text, NULL};
  Arg a;
  a.v = cmd;
  spawn(&a);
}
void notify_send_timeout_critical(const char *title, const char *text, int timeout){
  char timeout_str[16];
  snprintf(timeout_str, 16, "%d", timeout);
  const char *cmd[] = {"notify-send", "-t", timeout_str, "-u", "Critical", title, text, NULL};
  Arg a;
  a.v = cmd;
  spawn(&a);
}

int percentage_to_progressbar(char *buffer, int percentage, int len){
  if (percentage > 100 || percentage < 0 || len < 0){
    return -1;
  }

  //Empty buffer
  buffer[0] = '\0';

  int current_position = (len * percentage) / 100;
  for (int i = 0; i < current_position; i++){
    strcat(buffer, PROGRESSBAR_FULL_CHAR);
    // buffer[i] = PROGRESSBAR_FULL_CHAR;
  }
  // buffer[current_position] = PROGRESSBAR_CURR_CHAR;
  strcat(buffer, PROGRESSBAR_CURR_CHAR);
  for (int i = current_position; i < len; i++){
    strcat(buffer, PROGRESSBAR_EMPTY_CHAR);
    // buffer[i] = PROGRESSBAR_EMPTY_CHAR;
  }

  return 0;
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


  pthread_mutex_lock(&mutex_drawbar);
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
  pthread_mutex_unlock(&mutex_drawbar);

  if (scrot_status == 0){
    notify_send("Screenshot taken!", bufname);
  }

}

//UPDATES CHECKER
void  *check_updates(void *args){
  pthread_mutex_lock(&mutex_fetchupdates);
  checking_updates = true;
  pthread_mutex_unlock(&mutex_fetchupdates);

  const char *checkupdates[] = {"checkupdates", NULL};
  Arg checkupdates_arg = {.v = checkupdates};

  const char *checkupdates_aur[] = {"yay", "-Qum", NULL};
  Arg checkupdates_aur_arg = {.v = checkupdates_aur};

  int updates_pacman_local;
  int updates_aur_local;

  //Get pacman updates
  updates_pacman_local = spawn_countlines(&checkupdates_arg);
  updates_aur_local    = spawn_countlines(&checkupdates_aur_arg);

  //Modify the global updates variable in mutex
  pthread_mutex_lock(&mutex_fetchupdates);
  n_updates_pacman = updates_pacman_local;
  n_updates_aur    = updates_aur_local;
  checking_updates = false;
  pthread_mutex_unlock(&mutex_fetchupdates);

  return NULL;
}
void async_check_updates_handler(const Arg *a){
  pthread_t tid;
  pthread_create(&tid, NULL, check_updates, NULL);
}
void toggle_update_checks(const Arg *a){
  if (shall_fetch_updates){
    shall_fetch_updates = false;
  } else {
    shall_fetch_updates = true;
    async_check_updates_handler(NULL);
  }
}
