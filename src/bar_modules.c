#include <bar_modules.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

int date_barmodule(int bufsize, char *retstring){
  time_t rawtime;
  struct tm *timeinfo;
  char *weekday = "";
  char *month = "";

  time (&rawtime);
  timeinfo = localtime(&rawtime);

  switch(timeinfo->tm_wday){
    case 1: weekday = "Mon"; break;
    case 2: weekday = "Tue"; break;
    case 3: weekday = "Wed"; break;
    case 4: weekday = "Thu"; break;
    case 5: weekday = "Fri"; break;
    case 6: weekday = "Sat"; break;
    case 7: weekday = "Sun"; break;
  }

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

  //Bar with seconds
  // sprintf(retstring, " %s %02d/%s/%d    %02d:%02d:%02d", weekday, timeinfo->tm_mday, month, timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  // Without seconds
  sprintf(retstring, " %s %02d/%s/%d    %02d:%02d", weekday, timeinfo->tm_mday, month, timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min);
  return 0;
}
