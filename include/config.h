/* See LICENSE file for copyright and license details. */
#ifndef __HORIZONWM_CONFIG_H_
#define __HORIZONWM_CONFIG_H_

#include <spawn_programs.h>
#include <helper_scripts.h>
#include <menu_scripts.h>
#include <X11/XF86keysym.h>

//Borders and gaps
static const unsigned int borderpx  = 0;        /* border pixel of windows */
static const unsigned int snap      = 10;       /* snap pixel */
static const unsigned int def_gap_i = 10;       //Default inner gap
static const unsigned int def_gap_o = 20;       //Default outter gap
static const unsigned int border_alpha = 0xff;  //Border opacity (opaque)

//Bar
static const int showbar            = 1;       //0 means no bar
static const int bar_lobar          = 3;       //Thickness of bar color details (bottom)
static const int bar_hibar          = 0;       //Thickness of bar color details (bottom)
static const int topbar             = 1;       //0=bottom bar, 1=top bar
static const int bar_sleeptime      = 5;       //Seconds. 0 or negative means dont update
static const int bar_alpha          = 0xcc;    //Bar opacity 80%

//Fonts
// static const char *fonts[]          = { "monospace:size=10" };
static const char *fonts[]          = { "pango:SFNS Display Regular:size=10",  };
static const char dmenufont[]       = "monospace:size=10";

//Colors
static const char col_gray1[]       = "#222222";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#eeeeee";
static const char col_cyan[]        = "#005577";
static const char global_bg[]       = "#1a1a1a";
static const char global_fg[]       = "#dbdbdb";

//Colorschemes
static const char *colors[][3]      = {
	/*               fg         bg         border   */
	[SchemeNorm] = { global_fg, global_bg, global_bg },
	[SchemeSel]  = { global_bg, global_fg, global_fg  },
};

static const unsigned int alphas[][3] = {
  //
  [SchemeNorm] = { 0xff,      bar_alpha, border_alpha },
  [SchemeSel]  = { 0xff,      0xff, border_alpha },
};

/* tagging */
static const char *tags[] = { "1. ", "2. ", "3. ", "4. ", "5. ", "6. ", "7. ", "8. ", "9. ", "10. " };

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class                instance    title       tags mask     isfloating   monitor */
	{ "Gimp",               NULL,       NULL,       0,            1,           -1 },
	{ "Firefox",            NULL,       NULL,       1 << 8,       0,           -1 },
  { "Clavis",             NULL,       NULL,       0,            1,           -1 },
  { "gnome-calculator",   NULL,       NULL,       0,            1,           -1 },
};

/* layout(s) */
static const float mfact     = 0.5; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "",      tile },    /* first entry is default */
	{ "",      NULL },    /* no layout function means floating behavior */
	{ "",      monocle },
};

/* key definitions */
#define ALTKEY Mod1Mask
#define WINKEY Mod4Mask
#define MODKEY WINKEY
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ALTKEY,                KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

static const Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_d,      spawn,          {.v = roficmd } },
	{ MODKEY,                       XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY,                       XK_b,      spawn,          {.v = browsercmd } },
	{ MODKEY|ShiftMask,             XK_b,      spawn,          {.v = browser_private_cmd } },
	{ MODKEY,                       XK_x,      spawn,          {.v = claviscmd } },

  //  ##### Function keys #####
  //Volume
  { 0,                            XF86XK_AudioRaiseVolume,    spawn_waitpid,                 {.v = upvolumecmd}          },
  { 0,                            XF86XK_AudioLowerVolume,    spawn_waitpid,                 {.v = downvolume}           },
  { 0,                            XF86XK_AudioMute,           spawn_waitpid,                 {.v = mutevolume}           },
  //Volume (update bar)
  { 0,                            XF86XK_AudioRaiseVolume,    drawbars_caller_with_arg,      {0}                         },
  { 0,                            XF86XK_AudioLowerVolume,    drawbars_caller_with_arg,      {0}                         },
  { 0,                            XF86XK_AudioMute,           drawbars_caller_with_arg,      {0}                         },
  //Monitor
  { 0,                            XF86XK_MonBrightnessDown,   spawn_waitpid,                 {.v = downbrightnesscmd}    },
  { 0,                            XF86XK_MonBrightnessUp,     spawn_waitpid,                 {.v = upbrightnesscmd}      },
  { 0,                            XF86XK_ScreenSaver,         spawn_waitpid,                 {.v = monitoroffcmd}        },
  //Monitor (update bar)
  { 0,                            XF86XK_MonBrightnessDown,   drawbars_caller_with_arg,      {0}                         },
  { 0,                            XF86XK_MonBrightnessUp,     drawbars_caller_with_arg,      {0}                         },
  { 0,                            XF86XK_ScreenSaver,         drawbars_caller_with_arg,      {0}                         },
  //KBD light
  { 0,                            XF86XK_KbdBrightnessDown,   spawn,                         {.v = KBdownbrightnesscmd}  },
  { 0,                            XF86XK_KbdBrightnessUp,     spawn,                         {.v = KBupbrightnesscmd}    },
  //PowerMenu
  { 0,                            XF86XK_PowerOff,            menuscripts_powermenu,         {0}                         },

  { 0,                            XF86XK_Calculator,          spawn,                         {.v = calculatorcmd}        },

  //Updates checker
  { MODKEY,                       XK_u,                       toggle_update_checks,          {0}                         },
  { MODKEY,                       XK_u,                       drawbars_caller_with_arg,      {0}                         },

	{ MODKEY,                       XK_n,                       togglebar,                     {0}                         },
	{ MODKEY|ShiftMask,             XK_e,                       quit,                          {0}                         },

  //Move between windows
	{ MODKEY,                       XK_j,                       focusstack,                    {.i = +1 }                  },
	{ MODKEY,                       XK_Right,                   focusstack,                    {.i = +1 }                  },
	{ MODKEY,                       XK_Left,                    focusstack,                    {.i = -1 }                  },
	{ MODKEY,                       XK_k,                       focusstack,                    {.i = -1 }                  },

  //Restack master
	{ MODKEY,                       XK_minus,                   incnmaster,                    {.i = +1 }                  },
	{ MODKEY,                       XK_plus,                    incnmaster,                    {.i = -1 }                  },

  //Resize windows
	{ MODKEY|ShiftMask,             XK_Right,                   setmfact,                      {.f = +0.05}                },
	{ MODKEY|ShiftMask,             XK_Left,                    setmfact,                      {.f = -0.05}                },

  { 0,                            XK_F11,                     F11_togglefullscreen_handler,  {0}                         },

	{ MODKEY|ShiftMask,             XK_Return,                  zoom,                          {0}                         },
	{ MODKEY,                       XK_Tab,                     view,                          {0}                         },
	{ MODKEY,                       XK_q,                       killclient,                    {0}                         },

	{ MODKEY,                       XK_t,                       setlayout,                     {.v = &layouts[0]}          },
	// { MODKEY,                       XK_f,                       setlayout,                     {.v = &layouts[1]}          },
	{ MODKEY,                       XK_m,                       setlayout,                     {.v = &layouts[2]}          },
	{ MODKEY,                       XK_space,                   setlayout,                     {0}                         },

  //Gaps
	{ MODKEY|ShiftMask,             XK_plus,                    modgaps,                       {.i = +5}                   },
	{ MODKEY|ShiftMask,             XK_minus,                   modgaps,                       {.i = -5}                   },
  { MODKEY,                       XK_f,                       togglegaps,                    {.i = 0}                    },
  { MODKEY|ShiftMask,             XK_f,                       togglegaps,                    {.i = 1}                    },

  //Floating
	{ MODKEY,                       XK_g,                       togglefloating,                {0}                         },

  //View all tags
	{ MODKEY,                       XK_apostrophe,              view,                          {.ui = ~0 }                 },

  //Sticky
	{ MODKEY,                       XK_s,                       tag,                           {.ui = ~0 }                 },

	{ MODKEY,                       XK_comma,                   focusmon,                      {.i = -1 }                  },
	{ MODKEY,                       XK_period,                  focusmon,                      {.i = +1 }                  },
	{ MODKEY|ShiftMask,             XK_comma,                   tagmon,                        {.i = -1 }                  },
	{ MODKEY|ShiftMask,             XK_period,                  tagmon,                        {.i = +1 }                  },

  //Keyboard mappings
  { ControlMask,                  XK_Menu,                    switch_keyboard_mapping,       {0}                         },
  { ControlMask,                  XK_Menu,                    drawbars_caller_with_arg,      {0}                         },

	{ 0,                            XK_Print,                   scripts_take_screenshot,       {.i = 0}                    },
	{ ShiftMask,                    XK_Print,                   scripts_take_screenshot,       {.i = 1}                    },

	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	TAGKEYS(                        XK_0,                      9)
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

#endif //_HORIZONWM_CONFIG_H_
