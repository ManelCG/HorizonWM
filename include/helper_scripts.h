#ifndef __HELPER_SCRIPTS_H_
#define __HELPER_SCRIPTS_H_

#include <horizonwm_type_definitions.h>

void notify_send(const char *title, const char *text);
void notify_send_critical(const char *title, const char *text);
void notify_send_timeout(const char *title, const char *text, int timeout);
void notify_send_timeout_critical(const char *title, const char *text, int timeout);

void scripts_take_screenshot(const Arg *a);
void scripts_set_keyboard_mapping(const Arg *a);

float read_file_float(const char *file);
int read_file_int(const char *file);

int percentage_to_progressbar(char *buffer, int pctg, int len);

void switch_keyboard_mapping();

void toggle_update_checks(const Arg *a);
void async_check_updates_handler(const Arg *a);
void *check_updates(void *args);

#endif
