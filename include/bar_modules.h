#ifndef __BAR_MODULES_H_
#define __BAR_MODULES_H_

#include <stdlib.h>

int date_barmodule(int bufsize, char *retstring);

typedef int (*BarModuleFunction)(int bufsize, char *retstring);
typedef struct BarModule {
  BarModuleFunction function;
  unsigned int period;
} BarModule;

static const BarModule bar_modules[] = {
  {date_barmodule, 0},
  {NULL, 0}
};


#endif //_BAR_MODULES_H_
