#include <horizonwm_type_definitions.h>
#include <spawn_programs.h>
#include <util.h>
#include <global_vars.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

const char *browsercmd[]            = { "brave", NULL };
const char *browser_private_cmd[]   = { "brave", "--incognito",  NULL };

const char *roficmd[]               = { "rofi", "-show", "drun", NULL};
const char *rofibarcmd[]            = { "rofi", "-show", "drun", "-config", ROFIBARCNFG, NULL};
const char *termcmd[]               = { "alacritty", NULL };
const char *picomcmd[]              = {"picom", NULL};
const char *claviscmd[]             = {"clavis", NULL};
const char *calculatorcmd[]         = {"gnome-calculator", NULL};

const char *dunstcmd[]              = {"dunst", NULL};
const char *configkeyboardcmd[]     = {"xset", "r", "rate", "300", "50", NULL };
const char *wallpapercmd[]          = {WALLPAPERCMD, "bg", NULL};

//Monitor brightness
const char *downbrightnesscmd[]     = {"brightnessctl", "s", "5%-", NULL};
const char *upbrightnesscmd[]       = {"brightnessctl", "s", "5%+", NULL};
const char *monitoroffcmd[]         = {"xset", "dpms", "force", "off", NULL};

//Volume
const char *upvolumecmd[]           = {"pamixer", "-i", "5", NULL};
const char *downvolume[]            = {"pamixer", "-d", "5", NULL};
const char *mutevolume[]            = {"pamixer", "-t", NULL};

//System commands
const char *poweroffcmd[]           = {"poweroff", NULL};
const char *rebootcmd[]             = {"reboot", NULL};
const char *lockscreencmd[]         = {"i3lock", "-f", "-e", NULL};
const char *updatearchlinuxcmd[]    = {"alacritty", "-e", "yay", "-Syu", NULL};
const char *sysctl_status_ovpn[]    = {"systemctl", "is-active", "openvpn-client@*", NULL};
const char *sysctl_start_ovpn[]     = {"sudo", "systemctl", "start", "openvpn-client@client", NULL};
const char *sysctl_stop_ovpn[]      = {"sudo", "systemctl", "stop", "openvpn-client@*", NULL};
const char *nmcli_getssids[]        = {"nmcli", "-t", "-f", "active,ssid", "dev", "wifi", NULL};
const char *nmcli_getdevstatus[]    = {"nmcli", "-t", "-f", "state,type", "dev", NULL};

//Mpc commands
const char *mpc_toggle[]            = {"mpc", "-q", "toggle",       NULL};
const char *mpc_prev[]              = {"mpc", "-q", "cdprev",       NULL};
const char *mpc_stop[]              = {"mpc", "-q", "stop",         NULL};
const char *mpc_next[]              = {"mpc", "-q", "next",         NULL};
const char *mpc_volumeup[]          = {"mpc", "-q", "volume", "+5", NULL};
const char *mpc_volumedown[]        = {"mpc", "-q", "volume", "-5", NULL};

//Keyboard brightness
const char *KBdownbrightnesscmd[]   = {"brightnessctl", "-q", "-d='asus::kbd_backlight'", "s", "1-", NULL};
const char *KBupbrightnesscmd[]     = {"brightnessctl", "-q", "-d='asus::kbd_backlight'", "s", "1+", NULL};

// static const char **startup_programs[] = {
ProgramService startup_programs[] = {
  {picomcmd,          0, 0},
  {wallpapercmd,      0, 0},
  {dunstcmd,          0, 0},
  {configkeyboardcmd, 0, 0},
  {0, 0, 0}
};

unsigned int spawn_pid (const Arg *arg){
  int pid;
  if ((pid = fork()) == 0) {
    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }

  return pid;
}

void spawn (const Arg *arg){
  if (fork() == 0) {
    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }
}

void spawn_waitpid (const Arg *arg){
  int pid;
  if ((pid = fork() == 0)) {
    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }

  waitpid(pid, NULL, 0);
}
int spawn_retval(const Arg *arg){
  int pid, retval = 0;
  if ((pid = fork() == 0)){
    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }

  waitpid(pid, &retval, 0);
  return retval;
}

void spawn_devnull(const Arg *arg){
  if (fork() == 0) {
    close(1);
    open("/dev/null", O_WRONLY);

    close(2);
    dup(0);

    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }
}

void spawn_catchoutput (const Arg *arg, char *buffer, size_t size){
  int p[2];
  size_t bread;

  if (pipe(p) < 0){
    die("horizonwm: pipe failed on spawn_catchoutput(%s)", ((char **)arg->v)[0]);
  }

  if (fork() == 0) {
    close(1); dup(p[1]); close(p[1]); close(p[0]);  //Redirect stdout to pipe
    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }
  close(p[1]);
  bread = read(p[0], buffer, size);
  close(p[0]);

  buffer[bread] = '\0';

  return;
}

void spawn_greppattern(const Arg *arg, const char *flags, const char *pattern, char *buffer, size_t bufsize){
  int p_arg_grep[2];
  int p_grep_main[2];

  if (pipe(p_arg_grep) < 0){
    die("horizonwm: pipe failed on spawn_greppattern(%s, %s)", ((char **)arg->v)[0], pattern);
  }

  if (fork() == 0){
    close(1); dup(p_arg_grep[1]); close(p_arg_grep[1]); close(p_arg_grep[0]);
    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }
  close(p_arg_grep[1]);

  if (pipe(p_grep_main) < 0){
    die("horizonwm: pipe failed on spawn_greppattern(%s, %s)", ((char **)arg->v)[0], pattern);
  }

  if (fork() == 0){
    close(0); dup(p_arg_grep[0]); close(p_arg_grep[1]); close(p_arg_grep[0]);
    close(1); dup(p_grep_main[1]); close(p_grep_main[1]); close(p_grep_main[0]);
    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    if (flags != NULL){
      execlp("grep", "grep", flags, pattern, NULL);
    } else {
      execlp("grep", "grep", pattern, NULL);
    }
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }
  close(p_grep_main[1]); close(p_arg_grep[0]);

  read(p_grep_main[0], buffer, bufsize);
  close(p_grep_main[0]);
}

int spawn_readint(const Arg *arg){
  int p[2];
  char buffer[32];
  if (pipe(p) < 0){
    die("horizonwm: pipe failed on spawn_catchoutput(%s)", ((char **)arg->v)[0]);
  }

  if (fork() == 0) {
    close(1); dup(p[1]); close(p[1]); close(p[0]);  //Redirect stdout to pipe
    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }
  close(p[1]);
  read(p[0], buffer, 31);
  close(p[0]);

  return atoi(buffer);
}

int spawn_readint_feedstdin(const Arg *arg, const char *s){
  int p_in[2];
  int p_out[2];
  char buffer[32];
  buffer[0] = '\0';

  if (pipe(p_in) < 0){
    die("horizonwm: pipe failed on spawn_catchoutput(%s)", ((char **)arg->v)[0]);
  }
  if (pipe(p_out) < 0){
    die("horizonwm: pipe failed on spawn_catchoutput(%s)", ((char **)arg->v)[0]);
  }

  if (fork() == 0){
    close(0); dup(p_in[0]); close(p_in[1]); close(p_in[0]);
    close(1); dup(p_out[1]);close(p_out[0]);close(p_out[1]);

    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }
  close(p_in[0]); close(p_out[1]);

  write(p_in[1], s, strlen(s));
  close(p_in[1]);

  read(p_out[0], buffer, 31);
  close(p_out[0]);

  if (strlen(buffer) == 0){
    return -1;
  }

  return atoi(buffer);
}

int spawn_countlines (const Arg *arg){
  int p[2];
  int p2[2];
  char buffer[32];
  if (pipe(p) < 0){
    die("horizonwm: pipe failed on spawn_catchoutput(%s)", ((char **)arg->v)[0]);
  }
  if (fork() == 0) {
    close(1); dup(p[1]); close(p[1]); close(p[0]);  //Redirect stdout to pipe
    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("horizonwm: execvp '%s' failed:", ((char **)arg->v)[0]);
  }

  if (pipe(p2) < 0){
    die("horizonwm: pipe failed on spawn_catchoutput(%s)", ((char **)arg->v)[0]);
  }

  if (fork() == 0){
    close(0); dup(p[0]); close(p[1]); close(p[0]);
    close(1); dup(p2[1]); close(p2[1]); close(p2[0]);
    execlp("wc", "wc", "-l", NULL);
    die("horizonwm: execlp 'wc -l' failed:");
  }
  close(p[1]); close(p[1]);
  close(p2[1]);
  read(p2[0], buffer, 31);
  close(p2[0]);

  return atoi(buffer);
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
  }
}
