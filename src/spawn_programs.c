#include <horizonwm_type_definitions.h>
#include <spawn_programs.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"

unsigned int spawn_pid (const Arg *arg){
  int pid;
  if ((pid = fork()) == 0) {
    // if (dpy)
    //   close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }

  return pid;
}
void spawn (const Arg *arg){
  if (fork() == 0) {
    // if (dpy)
    //   close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }
}
void spawn_programs_list(ProgramService *l){
  int i = 0;
  ProgramService p = l[0];
  const char **program_cmd = p.cmd;
  Arg a;
  int pid;
  while (program_cmd != NULL){
    a.v = program_cmd;
    pid = spawn_pid(&a);
    l[i].pid = pid;
    i++;
    p = l[i];
    program_cmd = p.cmd;

    //TODO: Store pid of program
  }
}
