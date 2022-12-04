#ifndef __TYPEDEFS_HORIZONWM__
#define __TYPEDEFS_HORIZONWM__

#define SCREENSHOT_DIR "Pictures/screenshots/"

#define DEFAULT_PROGRESS_BAR_LEN 10
#define DEFAULT_PROGRESS_BAR_SIZE DEFAULT_PROGRESS_BAR_LEN*4
#define PROGRESSBAR_EMPTY_CHAR "─"
#define PROGRESSBAR_FULL_CHAR  "─"
#define PROGRESSBAR_CURR_CHAR  ""

typedef union {
  int i;
  unsigned int ui;
  float f;
  const void *v;
} Arg;



#endif //TYPEDEFS
