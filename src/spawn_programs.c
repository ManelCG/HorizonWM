#include <horizonwm_type_definitions.h>
#include <spawn_programs.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"

int spawn (const Arg *arg){
  if (fork() == 0) {
    // if (dpy)
    //   close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }
}
void spawn_programs_list(const ProgramService *l){
  int i = 0;
  ProgramService p = l[0];
  const char **programs = p.cmd;
  Arg a;
  while (programs != NULL){
    a.v = programs;
    spawn(&a);
    i++;
    p = l[i];
    programs = p.cmd;

    //TODO: Store pid of program
  }
}
