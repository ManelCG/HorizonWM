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


//All these functions spawn a program, each doing different functionality
void spawn(const Arg *arg);                                           //Spawns a program
unsigned int spawn_pid(const Arg *arg);                               //Spawns a program, returns its pid
void spawn_waitpid(const Arg *arg);                                   //Spawns a program. Waits until program terminates
void spawn_catchoutput(const Arg *, char *, size_t);                  //Spawns a program. Saves program output in buffer with size
void spawn_greppattern(const Arg *, const char *flags, const char *p, char *b, size_t);  //Spawns a program. Passes output by grep
int spawn_countlines(const Arg *);                                    //Spawns a program. Returns number of lines in output
int spawn_readint(const Arg *);                                       //Spawns a program. Expects int as output of program. Returns it.
int spawn_readint_feedstdin(const Arg *, const char *buf);            //Spawns a program. Feeds it string to stdin. Expects int as output of program. Returns it.
int spawn_retval(const Arg *);                                        //Spawns a program. Returns the exit value of the program.

void spawn_programs_list(ProgramService *l);

/* commands */
extern const char *browsercmd[];
extern const char *browser_private_cmd[];

extern const char *roficmd[];
extern const char *rofibarcmd[];
extern const char *termcmd[] ;
extern const char *picomcmd[];
extern const char *claviscmd[];
extern const char *calculatorcmd[];

extern const char *dunstcmd[];
extern const char *configkeyboardcmd[];
extern const char *wallpapercmd[];

//Monitor brightness
extern const char *downbrightnesscmd[];
extern const char *upbrightnesscmd[];
extern const char *monitoroffcmd[];

//Volume
extern const char *upvolumecmd[];
extern const char *downvolume[];
extern const char *mutevolume[];

//Keyboard brightness
extern const char *KBdownbrightnesscmd[];
extern const char *KBupbrightnesscmd[];

//System commands
extern const char *poweroffcmd[];
extern const char *rebootcmd[];
extern const char *lockscreencmd[];
extern const char *updatearchlinuxcmd[];
extern const char *sysctl_status_ovpn[];
extern const char *sysctl_start_ovpn[];
extern const char *stsctl_stop_ovpn[];
extern const char *nmcli_getssids[];
extern const char *nmcli_getdevstatus[];

// List of programs to be run at startup
extern ProgramService startup_programs[];

#endif //_SPAWN_PROGRAMS_H_
