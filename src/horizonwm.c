/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xlib-xcb.h>
#include <xcb/res.h>
#include <pthread.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>

#ifdef __OpenBSD__
#include <sys/sysctl.h>
#include <kvm.h>
#endif //OpenBSD

#include <horizonwm_type_definitions.h>
#include <spawn_programs.h>
#include <bar_modules.h>
#include <global_vars.h>

#include "drw.h"
#include "util.h"

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)

#define SIGNAL_UPDATEBAR 42069

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel }; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
       NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast, ClkBarModules, ClkDrawmode }; /* clicks */

//Window resize anchor points
enum { RszTL, RszT, RszTR, RszR, RszBR, RszB, RszBL, RszL};

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
	int bw, oldbw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen, isterminal, noswallow;
  pid_t pid;
	Client *next;
	Client *snext;
  Client *swallowing;
	Monitor *mon;
	Window win;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

struct Monitor {
	char ltsymbol[16];
	float mfact;
	int nmaster;
	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	unsigned int seltags;
	unsigned int sellt;
	unsigned int tagset[2];
	int showbar;
	int topbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
	const Layout *lt[2];
};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	unsigned int tags;
	int isfloating;
  int isterminal;
  int noswallow;
	int monitor;
} Rule;

/* function declarations */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void drawbars_caller_with_arg(const Arg *a);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *c);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void draw_on_screen_mouse(const Arg *arg);
static void restack(Monitor *m);
static void run(void);
static void scan(void);
static int sendevent(Client *c, Atom proto);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void modgaps(const Arg *arg);
static void togglegaps (const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void sigchld(int unused);
// static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void tile(Monitor *m);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void xinitvisual();
static void zoom(const Arg *arg);
static void F11_togglefullscreen_handler();
static void switch_wm_mode();

static pid_t getparentprocess(pid_t p);
static int isdescprocess(pid_t p, pid_t c);
static Client *swallowingclient(Window w);
static Client *termforwin(const Client *c);
static pid_t winpid(Window w);

/* variables */
static const char broken[] = "broken";
static char stext[256];
static int screen;
static int sw, sh;           /* X display screen geometry width, height */

int keyboard_mapping;                   //Extern, defined on <global_vars.h>
const char *keyboard_mappings[] = {     //Extern, defined on <global_vars.h>
  "es", "ru",
  NULL
};

int wm_mode;                            //Extern defined on <global_vars.h>
bool is_ethernet_connected;             //Extern defined on <global_vars.h>
bool is_wifi_connected;                 //Extern defined on <global_vars.h>
char wifi_ssid[128];                    //Extern defined on <global_vars.h>

int mpd_status = MPDStopped;            //Extern defined on <global_vars.h>
char mpd_song[128];                     //Extern defined on <global_vars.h>
char mpd_songduration[64];              //Extern defined on <global_vars.h>
char mpd_percentage[8];                 //Extern defined on <global_vars.h>

bool shall_fetch_updates = true;
bool checking_updates = false;

//MUTEX
pthread_mutex_t mutex_mpc;                //Extern, defined on <global_vars.h>
pthread_mutex_t mutex_drawbar;            //Extern, defined on <global_vars.h>
pthread_mutex_t mutex_fetchupdates;       //Extern, defined on <global_vars.h>
pthread_mutex_t mutex_connection_checker; //Extern, defined on <global_vars.h>

//BAR
static int bh;               /* bar height */
static int bar_separatorwidth;

static int lrpad;            /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast];
static int running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
Display *dpy;                       //Extern
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;

static xcb_connection_t *xcon;

static int useargb = 0;
static Visual *visual;
static int depth;
static Colormap cmap;

static int window_gap_inner;
static int window_gap_outter;

static pthread_t mpc_loop_pthread_t;
static pthread_t bar_loop_pthread_t;
static pthread_t updates_checker_pthread_t;
static pthread_t connection_checker_pthread_t;

/* configuration, allows nested code to access above variables */
#include <config.h>

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

/* function implementations */
void
applyrules(Client *c)
{
	const char *class, *instance;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { NULL, NULL };

	/* rule matching */
	c->isfloating = 0;
	c->tags = 0;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;

	for (i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if ((!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance)))
		{
      c->isterminal = r->isterminal;
      c->noswallow  = r->noswallow;
			c->isfloating = r->isfloating;
			c->tags |= r->tags;
			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

void switch_wm_mode(){
  wm_mode = wm_mode == WMModeDraw? WMModeNormal : WMModeDraw;
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
		if (!c->hintsvalid)
			updatesizehints(c);
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m)
{
	strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}

void
attach(Client *c)
{
	c->next = c->mon->clients;
	c->mon->clients = c;
}

void
attachstack(Client *c)
{
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void swallow(Client *p, Client *c){
  if (c->noswallow || c->isterminal){
    return;
  }
  if (c->noswallow && !swallowfloating && c->isfloating){
    return;
  }

  detach(c);
  detachstack(c);

  setclientstate(c, WithdrawnState);
  XUnmapWindow(dpy, p->win);

  p->swallowing = c;
  c->mon = p->mon;

  Window w = p->win;
  p->win = c->win;
  c->win = w;

  updatetitle(p);
  XMoveResizeWindow(dpy, p->win, p->x, p->y, p->w, p->h);
  arrange(p->mon);
  configure(p);
  updateclientlist();
}

void unswallow(Client *c){
  c->win = c->swallowing->win;

  free(c->swallowing);
  c->swallowing = NULL;

  setfullscreen(c, 0);
  updatetitle(c);
  arrange(c->mon);
  XMapWindow(dpy, c->win);
  XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
  setclientstate(c, NormalState);
  focus(NULL);
  arrange(c->mon);
}

void
buttonpress(XEvent *e)
{
	unsigned int i, j, tags_width, click, is_tag_selected, occ = 0;
  unsigned int modules_fullwidth = 0, nummodules;
  int modules_width_progress = 0;  //Used when detecting which module was pressed
  BarModule module;  //Bar module
	Arg arg = {0};
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

  //In draw mode, all click functions are ommited and all button presses are considered drawing
  if (wm_mode == WMModeDraw){
    click = ClkDrawmode;
    goto buttonpress_findbutton;
  }

	click = ClkRootWin;
	/* focus monitor if necessary */
	if ((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}

  //Get full width of modules on the right
  nummodules = 0;
  module = bar_modules[nummodules];
  do {
    modules_fullwidth += module.width;
    nummodules++;
    module = bar_modules[nummodules];
  } while (module.function != NULL);
  modules_fullwidth += bar_separatorwidth * (nummodules-1);

  //Are we clicking on the bar?
	if (ev->window == selmon->barwin) {
		i = tags_width = 0;

    //Which tags have windows?
    for (c = m->clients; c; c = c->next) {
      occ |= c->tags;
    }

    //Loop around all tags until the X value of the click is bigger than the length of the counted tags
		do {
      is_tag_selected = m->tagset[m->seltags] & 1 << i ? 1 : 0;   //Is tag selected?
      if (occ & 1 << i || is_tag_selected){                       //If tag is selected, or has windows
        tags_width += TEXTW(tags[i]);                             //Then we count it
      }
    } while (ev->x >= tags_width && ++i < LENGTH(tags));          //If X is bigger than the count, then break;

    //At this point, i is either the tag clicked, or 1 bigger

    //If i is one of the tags
		if (i < LENGTH(tags)) {
			click = ClkTagBar;  //We use the ClkTagBar mask
			arg.ui = 1 << i;    //Arg to be passed to the tag switching function  (mask of tag)
		} else if (ev->x < tags_width + TEXTW(selmon->ltsymbol)) {
			click = ClkLtSymbol;
    } else if (ev->x > selmon->ww - modules_fullwidth){ //A bar module has been clicked
      click = ClkBarModules;
      //Check which module was clicked

      for (j = 0; j < nummodules; j++){
        modules_width_progress += bar_modules[j].width;
        if (ev->x > m->ww - modules_width_progress){  //Module number j was clicked
          module = bar_modules[j];
          //Call function with specified click and mask
          if (module.functionOnClick){
            module.functionOnClick(CLEANMASK(ev->state), ev->button); //Call the function
            drawbars();
          }
          return;
        }

        modules_width_progress += bar_separatorwidth;
        if (ev->x > m->ww - modules_width_progress){  //Separator was clicked
          j = -1;
          break;
        }
      }

    } else {
			click = ClkWinTitle;
    }
	} else if ((c = wintoclient(ev->window))) {
		focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}

  buttonpress_findbutton:
	for (i = 0; i < LENGTH(buttons); i++)
		if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void)
{
	Arg a = {.ui = ~0};
	Layout foo = { "", NULL };
	Monitor *m;
	size_t i;

	view(&a);
	selmon->lt[selmon->sellt] = &foo;
	for (m = mons; m; m = m->next)
		while (m->stack)
			unmanage(m->stack, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	free(scheme);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	free(mon);
}

void
clientmessage(XEvent *e)
{
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen]) {}
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != selmon->sel && !c->isurgent)
			seturgent(c, 1);
	}
}

void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e)
{
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	/* TODO: updategeom handling sucks, needs to be simplified */
	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			drw_resize(drw, sw, bh);
			updatebars();
			for (m = mons; m; m = m->next) {
				for (c = m->clients; c; c = c->next)
					if (c->isfullscreen)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
				XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
			}
			focus(NULL);
			arrange(NULL);
		}
	}
}

void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

Monitor *
createmon(void)
{
	Monitor *m;

	m = ecalloc(1, sizeof(Monitor));
	m->tagset[0] = m->tagset[1] = 1;
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	m->topbar = topbar;
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
	return m;
}

void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window))) {
		unmanage(c, 1);
  } else if (( c = swallowingclient(ev->window))){
    unmanage(c->swallowing, 1);
  }
}

void
detach(Client *c)
{
	Client **tc;

	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
detachstack(Client *c)
{
	Client **tc, *t;

	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	if (dir > 0) {
		if (!(m = selmon->next))
			m = mons;
	} else if (selmon == mons)
		for (m = mons; m->next; m = m->next);
	else
		for (m = mons; m->next != selmon; m = m->next);
	return m;
}

void
drawbar(Monitor *m)
{
	int x, w, modules_textwidth = 0;
  int is_tag_selected;
  int nmons = 0;
	unsigned int i, occ = 0, urg = 0;
	Client *c;

  //Quit if we should not draw the bar
	if (!m->showbar)
		return;

  //Drawing the bar is protected by mutex
  pthread_mutex_lock(&mutex_drawbar);

  //Draw empty bar canvas
  drw_rect(drw, 0, 0, m->ww, bh, 1, 1);

  // ----------- Draw modules -------------
  int module_width;             //Width in pixels of current module (set inside loop)
  int side_padding = 7;         //Pixel padding left and right to each module. Gets "doubled" because each module has its own.
  char buffer[256] = "";        //Text returned by module to be written in bar
  char module_barcolor[8];      //Color returned by module to be set as extra color accent if bar_hibar or bar_lobar are bigger than 0.
  const char *module_scheme[3]; //Array of 3 color strings. In reality, will be set to {module_barcolor, module_barcolor, module_barcolor} to just draw line with said color.
  const unsigned int module_alphas[3] = {0xff, 0xff, 0xff};
  Clr *scheme_color;            //Color scheme structure, set with drw_scm_create and passed to drw_setscheme.

  //i loops through all modules. module is the module being currently drawn
  i = 0;
  BarModule module = bar_modules[i];
  do {
    //Empty the buffers to check if functions return something through them
    buffer[0] = '\0';           //Text to be written in the module
    module_barcolor[0] = '\0';  //Color of the bottom / top bar of the module

    //Set scheme
    drw_setscheme(drw, scheme[SchemeNorm]);
    module.function(256, buffer, NULL, module_barcolor);

    module_width = TEXTW(buffer) - lrpad + side_padding;
    drw_text(drw, m->ww - (module_width + modules_textwidth), bar_hibar, module_width, bh - (bar_lobar + bar_hibar), 0, buffer, 0); //Draw module text

    if (module_barcolor[0] != '\0'){
      module_scheme[0] = module_barcolor; // Color scheme internally uses 3 colors, but
      module_scheme[1] = module_barcolor; // we only want to draw one, so we set all 3
      module_scheme[2] = module_barcolor; // to be the same color.

		  scheme_color = drw_scm_create(drw, (const char **) module_scheme, module_alphas, 3);
      drw_setscheme(drw, scheme_color);
      drw_rect(drw, m->ww - (module_width + modules_textwidth), 0,              module_width - side_padding, bar_hibar, 1, 1);
      drw_rect(drw, m->ww - (module_width + modules_textwidth), bh - bar_lobar, module_width - side_padding, bar_lobar, 1, 1);

      free(scheme_color);
      drw_setscheme(drw, scheme[SchemeNorm]);
    }

    //Offset next drawing position to the left
    if (strcmp(buffer, "") != 0){
      module.width = module_width + side_padding; //3px padding on the left
      bar_modules[i].width = module_width + side_padding;
      modules_textwidth += module.width;

      //Draw vertical separators between modules
      if (bar_modules[i+1].function != NULL){
        drw_rect(drw, m->ww - (modules_textwidth + bar_separatorwidth), 0, bar_separatorwidth, bh, 1, 0);
        modules_textwidth += bar_separatorwidth;
      }
    }

    i++;
    module = bar_modules[i];
  } while (module.function != NULL);

	///* draw status first so it can be overdrawn by tags later */
	//if (m == selmon) { /* status is only drawn on selected monitor */
	//	drw_setscheme(drw, scheme[SchemeNorm]);
    ////DWM text at the right
	//	modules_textwidth = TEXTW(buffer) - lrpad + 2; /* 2px right padding */
	//	drw_text(drw, m->ww - modules_textwidth, 0, modules_textwidth, bh, 0, buffer, 0);
	//}


  //Square drawing functions
  //Original DWM squares when there is window in tag
  // if (occ & 1 << i)
  // 	drw_rect(drw, x + boxs, boxs, boxw, boxw,
  // 		m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
  // 		urg & 1 << i);

  // if (m->sel->isfloating) //Dwm draw square if window is floating
  // 	drw_rect(drw, x + boxs, boxs, boxw, boxw, m->sel->isfixed, 0);


  // -------------- Draw tags (workspaces) ----------------
  //Which tags have windows?
	for (c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	x = 0;

  //Draw the tags in bar
	for (i = 0; i < LENGTH(tags); i++) {
    w = TEXTW(tags[i]);
    is_tag_selected = m->tagset[m->seltags] & 1 << i ? 1 : 0;

    //Set scheme (Sel if tag is selected, or norm otherwise)
    drw_setscheme(drw, scheme[is_tag_selected ? SchemeSel : SchemeNorm]);

    if (occ & 1 << i || is_tag_selected){
      drw_text(drw, x, bar_hibar, w, bh-(bar_lobar+bar_hibar), lrpad / 2, tags[i], 0);

      //Draw lower and upper bar if they exist
      drw_rect(drw, x, 0, w, bar_hibar, 1, 0);
      drw_rect(drw, x, bh - bar_lobar, w, bar_lobar, 1, 0);

      x += w;
    }
	}

  // ------------ Draw selected layout icon -------------
	w = TEXTW(m->ltsymbol);
	drw_setscheme(drw, scheme[SchemeNorm]);
  drw_rect(drw, x, 0, w, bh, 1, 1);
	x = drw_text(drw, x, bar_hibar, w, bh-(bar_lobar + bar_hibar), lrpad / 2, m->ltsymbol, 0);

  // ------------- Get number of monitors ---------------
	for (Monitor *m = mons; m; m = m->next){
    nmons++;
  }

  // ------------- Write text of currently selected window ---------------
	if ((w = m->ww - modules_textwidth - x) > bh) {
    drw_rect(drw, x, 0, w, bh, 1, 1); //Empty bar all the way to right
		if (m->sel) {
			drw_setscheme(drw, scheme[nmons > 1 && m == selmon ? SchemeSel : SchemeNorm]);  //Fills color if more than 1 monitor and selected
			drw_text(drw, x, bar_hibar, w, bh-(bar_lobar + bar_hibar), lrpad / 2, m->sel->name, 0);
		}
	}

  // -------------- Draw bar on screen ---------------
	drw_map(drw, m->barwin, 0, 0, m->ww, bh);

  pthread_mutex_unlock(&mutex_drawbar);
}
void
drawbars(void)
{
	Monitor *m;

	for (m = mons; m; m = m->next)
		drawbar(m);
}
//This function is just an extra step so to not include the drawbars() function directly in the keys array
void drawbars_caller_with_arg(const Arg *a){
  drawbars();
}

void *updates_checker(void *args){
  //Wait a little for internet connection to be stablished
  sleep(5);
  while (1){
    if (shall_fetch_updates){
      check_updates(NULL);
    }

    sleep(1);   //First sleep call getting ignored??? Bug with compiler maybe?
    sleep(900); //Sleep 15 minutes
  }
  return NULL;
}

void *connection_checker(void *args){
  Arg arg_eth = {.v = nmcli_getdevstatus};
  Arg arg_wifi = {.v = nmcli_getssids};
  bool is_e_con = false;
  bool is_w_con = false;
  char *pattern_wifi = "^yes";
  char *pattern_eth = "ethernet";
  char buffer[128];
  char *tok;

  for (;;){
    //Get ethernet status
    spawn_greppattern(&arg_eth, NULL, pattern_eth, buffer, 127);
    tok = strchr(buffer, '\n');
    if (tok != NULL){
      *tok = '\0';
    }
    if (strcmp(buffer, "") == 0 || strncmp(buffer, "connected:", 10) != 0){
      is_e_con = false;
    } else {
      is_e_con = true;
    }

    //Get wifi status
    spawn_greppattern(&arg_wifi, NULL, pattern_wifi, buffer, 127);
    if (strcmp(buffer, "") == 0 || strncmp(buffer, "yes:", 4) != 0){
      is_w_con = false;
    } else {
      tok = strchr(buffer, '\n');
      if (tok != NULL){
        is_w_con = true;
        *tok = '\0';
      }
    }

    pthread_mutex_lock(&mutex_connection_checker);
    is_ethernet_connected = is_e_con;
    is_wifi_connected = is_w_con;
    snprintf(wifi_ssid, 127, buffer+4);
    pthread_mutex_unlock(&mutex_connection_checker);

    sleep(1);
    sleep(5);
  }
}

void setmpcstatus(){
  char buffer[64];
  char durbuffer[64];
  char songbuffer[128];
  char percbuffer[8];
  char *tok;
  Arg arg;

  const char *mpc_statuscmd[] = {"mpc", "status", NULL};
  const char *mpc_currentcmd[] = {"mpc", "current", NULL};

  int status_local = MPDStopped;


  arg.v = mpc_statuscmd;

  buffer[0] = '\0';
  spawn_greppattern(&arg, NULL, "^\\[playing\\].*#[0-9]*/[0-9]*.*([0-9]*%)", buffer, sizeof(buffer)-1);
  if (strlen(buffer) > 0){
    status_local = MPDPlaying;
  } else {
    buffer[0] = '\0';
    spawn_greppattern(&arg, NULL, "^\\[paused\\].*#[0-9]*/[0-9]*.*([0-9]*%)", buffer, sizeof(buffer)-1);
    if (strlen(buffer) > 0){
      status_local = MPDPaused;
    }
  }

  if (status_local == MPDPlaying || MPDPaused){
    durbuffer[0] = '\0';
    spawn_greppattern(&arg, "-Po", "(?<= )(?<!#)([0-9]+:)*[0-9]+/([0-9]+:)*[0-9]+ ", durbuffer, sizeof(durbuffer)-1);
    if ((tok = strchr(durbuffer, '\n'))){
      *tok = '\0';
    }
    if ((tok = strchr(durbuffer, ' '))){
      *tok = '\0';
    }

    percbuffer[0] = '\0';
    spawn_greppattern(&arg, "-Po", "(?<=\\()[0-9]+(?=%\\))", percbuffer, sizeof(durbuffer)-1);
    if ((tok = strchr(percbuffer, '\n'))){
      *tok = '\0';
    }

    arg.v = mpc_currentcmd;
    spawn_catchoutput(&arg, songbuffer, sizeof(songbuffer)-1);
    if ((tok = strchr(songbuffer, '\n'))){
      *tok = '\0';
    }

    pthread_mutex_lock(&mutex_mpc);
    mpd_status = status_local;
    strcpy(mpd_song, songbuffer);
    strcpy(mpd_songduration, durbuffer);
    strcpy(mpd_percentage, percbuffer);
    pthread_mutex_unlock(&mutex_mpc);
  } else {
    pthread_mutex_lock(&mutex_mpc);
    mpd_status = status_local;
    mpd_song[0] = '\0';
    mpd_songduration[0] = '\0';
    mpd_percentage[0] = '\0';
    pthread_mutex_unlock(&mutex_mpc);
  }
}

void *mpc_loop(void *args){
  for (;;){
    setmpcstatus();

    sleep(1);
    sleep(1);
  }
  return NULL;
}

void *bar_loop(void *args){
  int sleeptime = *((int *) args);
  if (sleeptime <= 0){
    return NULL;
  }
  for (;;){
    sleep(1);
    sleep(sleeptime);
    drawbars();
  }
  return NULL;
}

void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
	focus(c);
}

void
expose(XEvent *e)
{
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0 && (m = wintomon(ev->window)))
		drawbar(m);
}

void
focus(Client *c)
{
	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

void
focusmon(const Arg *arg)
{
	Monitor *m;

	if (!mons->next)
		return;
	if ((m = dirtomon(arg->i)) == selmon)
		return;
	unfocus(selmon->sel, 0);
	selmon = m;
	focus(NULL);
}

void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen))
		return;
	if (arg->i > 0) {
		for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		if (!c)
			for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	if (c) {
		focus(c);
		restack(selmon);
	}
}

Atom
getatomprop(Client *c, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		XFree(p);
	}
	return atom;
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING) {
		strncpy(text, (char *)name.value, size - 1);
	} else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
		strncpy(text, *list, size - 1);
		XFreeStringList(list);
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

void
grabbuttons(Client *c, int focused)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].click == ClkClientWin)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, buttons[i].button,
						buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

void
grabkeys(void)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		KeyCode code;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		for (i = 0; i < LENGTH(keys); i++)
			if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
						True, GrabModeAsync, GrabModeAsync);
	}
}

void
incnmaster(const Arg *arg)
{
	selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
	arrange(selmon);
}

#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

void
keypress(XEvent *e)
{
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	for (i = 0; i < LENGTH(keys); i++)
		if (keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func) {
			keys[i].func(&(keys[i].arg));
    }

}

void
killclient(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (!sendevent(selmon->sel, wmatom[WMDelete])) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL, *term = NULL;
	Window trans = None;
	XWindowChanges wc;

	c = ecalloc(1, sizeof(Client));
	c->win = w;
  c->pid = winpid(w);
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;

	updatetitle(c);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
	} else {
		c->mon = selmon;
		applyrules(c);
    term = termforwin(c);
	}

	if (c->x + WIDTH(c) > c->mon->wx + c->mon->ww)
		c->x = c->mon->wx + c->mon->ww - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->wy + c->mon->wh)
		c->y = c->mon->wy + c->mon->wh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->wx);
	c->y = MAX(c->y, c->mon->wy);
	c->bw = borderpx;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	if (!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if (c->isfloating)
		XRaiseWindow(dpy, c->win);
	attach(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (c->mon == selmon)
		unfocus(selmon->sel, 0);
	c->mon->sel = c;
	arrange(c->mon);
	XMapWindow(dpy, c->win);
  if (term){
    swallow(term, c);
  }
	focus(NULL);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
monocle(Monitor *m)
{
	unsigned int n = 0;
	Client *c;

	for (c = m->clients; c; c = c->next)
		if (ISVISIBLE(c))
			n++;
	if (n > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
		resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
}

void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if (ev->window != root)
		return;
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);

      //Snap to X (screen border)
			if (abs(selmon->wx - nx) < snap) {
				nx = selmon->wx;
			} else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap){
				nx = selmon->wx + selmon->ww - WIDTH(c);
      }
      //Snap to X (Inside gaps)
			else if (abs(selmon->wx + window_gap_outter - nx) < snap) {
				nx = selmon->wx + window_gap_outter;
      } else if (abs((selmon->wx + selmon->ww) - (window_gap_outter + nx + WIDTH(c))) < snap) {
				nx = selmon->wx + selmon->ww - (WIDTH(c) + window_gap_outter);
      }

      //Snap to Y (Inside gaps)
			if (abs(selmon->wy + window_gap_outter - ny) < snap) {
				ny = selmon->wy + window_gap_outter;
      } else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c) + window_gap_outter)) < snap) {
				ny = selmon->wy + selmon->wh - (HEIGHT(c) + window_gap_outter);
      }
      //Snap to Y (screen border)
			else if (abs(selmon->wy - ny) < snap) {
				ny = selmon->wy;
      } else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap) {
				ny = selmon->wy + selmon->wh - HEIGHT(c);
      }


			if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

Client *
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

void
pop(Client *c)
{
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
	else if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(c->isfloating = (wintoclient(trans)) != NULL))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			c->hintsvalid = 0;
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

void
quit(const Arg *arg)
{
	running = 0;
}

Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

void draw_on_screen_mouse(const Arg *arg){
  Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
      if ((ev.xmotion.time - lasttime) <= (1000 / 60))
        continue;
      lasttime = ev.xmotion.time;

      break;
    }
	} while (ev.type != ButtonRelease);

	XUngrabPointer(dpy, CurrentTime);
}

void
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh, nx, ny, ocw, och;
  int minw, minh;
  int mousex, mousey;
  int top, bot, left, right;
  int medx, medy;

  int default_min = 85;

	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;
	XSizeHints hints;
  long supplied_return;

  int anchor;

  //Window resize anchor points
  // enum { RszTL, RszT, RszTR, RszR, RszBR, RszB, RszBL, RszL};

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);

  //Get original window dimensions
	ocx = c->x;
	ocy = c->y;
  ocw = c->w;
  och = c->h;

  //Size hints of window. Contains minimum height and width
  XGetWMNormalHints(dpy, c->win, &hints, &supplied_return);

  //Keep widescreen aspect ratio sometimes
  if (hints.min_width < default_min && hints.min_height < default_min){
    minw = default_min;
    minh = default_min;
  } else if (hints.min_height < default_min){
    minw = hints.min_width;
    minh = minw * 9 / 16;
  } else if (hints.min_width < default_min){
    minh = hints.min_height;
    minw = minh * 16 / 9;
  } else {
    minh = hints.min_height;
    minw = hints.min_width;
  }

  //Pixel boundaries of window
  left = c->x;
  right = c->x + c->w + 2*c->bw;
  top = c->y;
  bot = c->y + c->h + 2*c->bw;

  //Median of boundaries (half-points)
  medx = (left + right)/2;
  medy = (top + bot)/2;

  //Get the mouse event
  XPeekEvent(dpy, &ev);
  mousex = ev.xbutton.x_root;
  mousey = ev.xbutton.y_root;

  //##### Find anchor point #######
  if (abs(mousex - right) < abs(mousex - medx)){          //On right?
    if (abs(mousey - bot) < abs(mousey - medy)){          //Bottom?
      XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
      anchor = RszBR;
    } else if (abs(mousey - top) < abs(mousey - medy)){   //Top?
      XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, -c->bw);
      anchor = RszTR;
    } else {  //CenterY
      XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h/2 -1);
      anchor = RszR;
    }
  } else if (abs(mousex - left) < abs(mousex - medx)){    //On left?
    if (abs(mousey - bot) < abs(mousey - medy)){          //Bottom?
      XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, -c->bw, c->h + c->bw - 1);
      anchor = RszBL;
    } else if (abs(mousey - top) < abs(mousey - medy)){   //Top?
      XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, -c->bw, -c->bw);
      anchor = RszTL;
    } else {  //CenterY
      XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, -c->bw, c->h/2);
      anchor = RszL;
    }
  } else {    //CenterX
    if (abs(mousey - bot) < abs(mousey - medy)){          //Bottom?
      XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w/2  -1, c->h + c->bw - 1);
      anchor = RszB;
    } else if (abs(mousey - top) < abs(mousey - medy)){   //Top?
      XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w/2  -1, -c->bw);
      anchor = RszT;
    } else {  //Window Center. Skip resize
      return;
    }
  }

	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

      mousex = ev.xmotion.x;
      mousey = ev.xmotion.y;

      //Snap to X (screen border)
			if (abs(selmon->mx - mousex) < snap) {
				mousex = selmon->mx;
			} else if (abs((selmon->mx + selmon->mw) - mousex) < snap){
				mousex = selmon->mx + selmon->mw;
      }
      //Snap to X (Inside gaps)
			else if (abs(selmon->mx + window_gap_outter - mousex) < snap) {
				mousex = selmon->mx + window_gap_outter;
      } else if (abs((selmon->mx + selmon->mw) - (window_gap_outter + mousex)) < snap) {
				mousex = selmon->mx + selmon->mw - (window_gap_outter);
      }

      //Snap to Y (Inside gaps)
			if (abs(selmon->my + window_gap_outter + selmon->showbar*topbar*bh - mousey) < snap) {
				mousey = selmon->my + window_gap_outter + selmon->showbar*topbar*bh;
      } else if (abs((selmon->my + selmon->mh) - (mousey + window_gap_outter + selmon->showbar*(!topbar)*bh)) < snap) {
				mousey = selmon->my + selmon->mh - (window_gap_outter + selmon->showbar*(!topbar)*bh);
      }
      //Snap to Y (screen border)
			else if (abs(selmon->my - mousey + selmon->showbar*topbar*bh) < snap) {
				mousey = selmon->my + selmon->showbar*topbar*bh;
      } else if (abs((selmon->my + selmon->mh) - (mousey + selmon->showbar*(!topbar)*bh)) < snap) {
				mousey = selmon->my + selmon->mh - selmon->showbar*(!topbar)*bh;
      }

      switch(anchor){
        case RszT:
          nw = c->w;
          nh = ocy - mousey + och;
          nx = c->x;
          ny = mousey;

          if (nh < minh){
            ny = mousey - (minh - nh);
            nh = minh;
          }

          if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
          && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
          {
            if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
            && (abs(nw - nw) > snap || abs(nh - c->h) > snap))
              togglefloating(NULL);
          }
          break;

        case RszB:
          nw = c->w;
          nh = mousey - ocy - 2 * c->bw + 1;
          nx = c->x;
          ny = c->y;

          if (nh < minh){
            nh = minh;
          }

          if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
          && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
          {
            if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
            && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
              togglefloating(NULL);
          }
          break;

        case RszR:
          nw = mousex - ocx - 2 * c->bw + 1;
          nh = c->h;
          nx = c->x;
          ny = c->y;

          if (nw < minw){
            nw = minw;
          }

          if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
          && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
          {
            if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
            && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
              togglefloating(NULL);
          }
          break;

        case RszL:
          nw = ocx - mousex + ocw;
          nh = c->h;
          nx = mousex;
          ny = c->y;

          if (nw < minw){
            nx = mousex - (minw - nw);
            nw = minw;
          }

          if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
          && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
          {
            if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
            && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
              togglefloating(NULL);
          }
          break;

        case RszBR:
          nw = mousex - ocx - 2 * c->bw + 1;
          nh = mousey - ocy - 2 * c->bw + 1;
          nx = c->x;
          ny = c->y;

          if (nw < minw){
            nw = minw;
          }
          if (nh < minh){
            nh = minh;
          }

          if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
          && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
          {
            if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
            && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
              togglefloating(NULL);
          }
          break;

        case RszTR:
          nw = mousex - ocx - 2 * c->bw + 1;
          nh = ocy - mousey + och;
          nx = c->x;
          ny = mousey;

          if (nw < minw){
            nw = minw;
          }
          if (nh < minh){
            ny = mousey - (minh - nh);
            nh = minh;
          }

          if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
          && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
          {
            if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
            && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
              togglefloating(NULL);
          }
          break;

        case RszBL:
          nw = ocx - mousex + ocw;
          nh = mousey - ocy - 2 * c->bw + 1;
          nx = mousex;
          ny = c->y;

          if (nh < minh){
            nh = minh;
          }
          if (nw < minw){
            nx = mousex - (minw - nw);
            nw = minw;
          }

          if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
          && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
          {
            if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
            && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
              togglefloating(NULL);
          }
          break;

        case RszTL:
          nw = ocx - mousex + ocw;
          nh = ocy - mousey + och;
          nx = mousex;
          ny = mousey;

          if (nh < minh){
            ny = mousey - (minh - nh);
            nh = minh;
          }
          if (nw < minw){
            nx = mousex - (minw - nw);
            nw = minw;
          }

          if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
          && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
          {
            if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
            && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
              togglefloating(NULL);
          }
          break;

      }
      if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
        resize(c, nx, ny, nw, nh, 1);
			break;
		}
	} while (ev.type != ButtonRelease);

	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if (!m->sel)
		return;
	if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
run(void)
{
	XEvent ev;
	/* main event loop */
	XSync(dpy, False);
	while (running && !XNextEvent(dpy, &ev)) {
		if (handler[ev.type]) {
			handler[ev.type](&ev); /* call handler */
    }
  }
}

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

void
sendmon(Client *c, Monitor *m)
{
	if (c->mon == m)
		return;
	unfocus(c, 1);
	detach(c);
	detachstack(c);
	c->mon = m;
	c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

int
sendevent(Client *c, Atom proto)
{
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;

	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	}
	return exists;
}

void
setfocus(Client *c)
{
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}
	sendevent(c, wmatom[WMTakeFocus]);
}

void
setfullscreen(Client *c, int fullscreen)
{
	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		c->oldstate = c->isfloating;
		c->oldbw = c->bw;
		c->bw = 0;
		c->isfloating = 1;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	} else if (!fullscreen && c->isfullscreen){
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		c->isfloating = c->oldstate;
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(c->mon);
	}
}

void F11_togglefullscreen_handler(){
	Client *c = selmon->sel;
  int fullscreen = c->isfullscreen? 0: 1;
  setfullscreen(c, fullscreen);
}

void
setlayout(const Arg *arg)
{
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
		selmon->sellt ^= 1;
	if (arg && arg->v)
		selmon->lt[selmon->sellt] = (Layout *)arg->v;
	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);
	if (selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
}

/* arg > 1.0 will set mfact absolutely */
void
setmfact(const Arg *arg)
{
	float f;

	if (!arg || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
	if (f < 0.05 || f > 0.95)
		return;
	selmon->mfact = f;
	arrange(selmon);
}

void
modgaps(const Arg *arg){
	if (!arg || !selmon->lt[selmon->sellt]->arrange)
		return;

  if (window_gap_inner + arg->i <= 0){
    window_gap_inner = 0;
  } else {
    window_gap_inner += arg->i;
  }
  if (window_gap_outter + 2*arg->i <= 0){
    window_gap_outter = 0;
  } else {
    window_gap_outter += 2*arg->i;
  }

  arrange(selmon);
}
void togglegaps (const Arg *arg){
  if (!arg || !selmon->lt[selmon->sellt]->arrange)
    return;

  if (arg->i == 1){
    window_gap_inner = def_gap_i;
    window_gap_outter = def_gap_o;
  } else if (arg->i == 0){
    window_gap_inner = 0;
    window_gap_outter = 0;
  }

  arrange(selmon);
}

void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;

	/* clean up any zombies immediately */
	sigchld(0);

  //Keyboard mapping
  keyboard_mapping = 0;
  { Arg arg;
    arg.v = keyboard_mappings[keyboard_mapping];
    scripts_set_keyboard_mapping(&arg);
  }

  //Init mutex
  pthread_mutex_init(&mutex_mpc, NULL);
  pthread_mutex_init(&mutex_drawbar, NULL);
  pthread_mutex_init(&mutex_fetchupdates, NULL);
  pthread_mutex_init(&mutex_connection_checker, NULL);

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
  xinitvisual();
	drw = drw_create(dpy, screen, root, sw, sh, visual, depth, cmap);
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h;
  bar_separatorwidth = 0;
  wm_mode = 0;
	bh = drw->fonts->h + 6 + bar_lobar + bar_hibar;
	updategeom();
	/* init atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	/* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], alphas[i], 3);
  window_gap_inner = def_gap_i;
  window_gap_outter = def_gap_o;
	/* init bars */
	updatebars();
	updatestatus();
  pthread_create(&mpc_loop_pthread_t, NULL, mpc_loop, NULL);
  pthread_create(&bar_loop_pthread_t, NULL, bar_loop, (void *) &bar_sleeptime);
  pthread_create(&updates_checker_pthread_t, NULL, updates_checker, NULL);
  pthread_create(&connection_checker_pthread_t, NULL, connection_checker, NULL);
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) WMNAME, 9);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys();
	focus(NULL);
}

void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
sigchld(int unused)
{
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		die("can't install SIGCHLD handler:");
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
tag(const Arg *arg)
{
	if (selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
		focus(NULL);
		arrange(selmon);
	}
}

void
tagmon(const Arg *arg)
{
	if (!selmon->sel || !mons->next)
		return;
	sendmon(selmon->sel, dirtomon(arg->i));
}

void
tile(Monitor *m)
{
	unsigned int i, i_master, i_slave, n, master_width, slave_width, master_y, slave_y;
  unsigned int nmaster, nslave;
  unsigned int master_height, slave_height;
  unsigned int lowbound;
  unsigned int master_width_extra_offset = 0;
  unsigned int slave_width_extra_offset  = 0;
  unsigned int slave_lpadd_extra_offset  = 0;
	Client *c;

  //Get number of windows
	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);

	if (n == 0)
		return;

	if (n > m->nmaster){ //Some windows are not master
    master_width = m->nmaster > 0 ? m->ww * m->mfact : 0; /* Gaps */
    slave_width  = m->ww - master_width;
  } else {             //All windows are master
		master_width = m->ww;
    slave_width = 0;
  }

  nmaster = MIN(n, m->nmaster);
  nslave = n - nmaster;

  //Pre-calculate height of all windows
  master_height = 1;
  slave_height = 1;
  if (nmaster != 0){
    master_height = ((m->mh - bh - 2*window_gap_outter - (nmaster -1)*window_gap_inner)/nmaster);
    slave_lpadd_extra_offset = window_gap_inner/2;
    slave_width_extra_offset = -window_gap_inner/2;
  } else {  //nmaster = 0
    slave_lpadd_extra_offset = window_gap_outter;
    slave_width_extra_offset = -window_gap_outter;
  }
  if (nslave != 0){
    slave_height  =  ((m->mh - bh - 2*window_gap_outter - (nslave  -1)*window_gap_inner)/nslave);
    master_width -= window_gap_inner;
    master_width_extra_offset = window_gap_inner/2;
  } else {  //nslave = 0
    master_width_extra_offset = -window_gap_outter;
  }

  //Where should last pixel row be drawn (fixes rounding-errors)
  lowbound = m->mh - window_gap_outter - ((!topbar && selmon->showbar) * bh) - 2*borderpx;

  //Default vertical padding of windows (before gaps calculation)
  master_y = 0; slave_y = 0;

  //How many windows have been drawn
  i_master = 0; i_slave = 0;

  //Pointer to the windows
  c = nexttiled(m->clients);

  //Allocate memory outside of loop for the 4 corners of each window
  unsigned int left_padding, top_padding, width, height;
  for (i = 0; c != NULL; c = nexttiled(c->next), i++){
    if (i < m->nmaster){  //Window is master
      left_padding = m->wx + window_gap_outter;
      top_padding = m->wy + master_y + window_gap_outter;
      width = master_width - (2*c->bw) - window_gap_outter + master_width_extra_offset;

      //Last master window should reach until the bottom (This will make it bigger/smaller by 1 or 2 pixels)
      if (i_master +1 == nmaster){
        height = lowbound - top_padding;
      } else {
        height = master_height - (2*c->bw);
      }

      resize(c, left_padding, top_padding, width, height, 0);

      master_y += master_height + window_gap_inner;

      i_master++;
    } else {  //Window is not master
      left_padding = m->wx + master_width + window_gap_inner + slave_lpadd_extra_offset;
      top_padding = m->wy + slave_y + window_gap_outter;
      width = slave_width - (2*c->bw) - window_gap_outter + slave_width_extra_offset;

      //Last slave window should reach until the bottom (This will make it bigger/smaller by 1 or 2 pixels)
      if (i_slave+1 == nslave){
        height = lowbound -top_padding;
      } else {
        height = slave_height - (2*c->bw);
      }

      resize(c, left_padding, top_padding, width, height, 0);
      slave_y += slave_height + window_gap_inner;

      i_slave++;
    }
  }
}

void
togglebar(const Arg *arg)
{
	selmon->showbar = !selmon->showbar;
	updatebarpos(selmon);
	XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
	arrange(selmon);
}

void
togglefloating(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (selmon->sel->isfullscreen) /* no support for fullscreen windows */
		return;
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if (selmon->sel->isfloating)
		resize(selmon->sel, selmon->sel->x, selmon->sel->y,
			selmon->sel->w, selmon->sel->h, 0);
	arrange(selmon);
}

void
toggletag(const Arg *arg)
{
	unsigned int newtags;

	if (!selmon->sel)
		return;
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	if (newtags) {
		selmon->sel->tags = newtags;
		focus(NULL);
		arrange(selmon);
	}
}

void
toggleview(const Arg *arg)
{
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

	if (newtagset) {
		selmon->tagset[selmon->seltags] = newtagset;
		focus(NULL);
		arrange(selmon);
	}
}

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

void
unmanage(Client *c, int destroyed)
{
	Monitor *m = c->mon;
	XWindowChanges wc;

  if (c->swallowing){
    unswallow(c);
    return;
  }

  Client *s = swallowingclient(c->win);
  if (s){
    free(s->swallowing);
    s->swallowing = NULL;
    arrange(m);
    focus(NULL);
    return;
  }

	detach(c);
	detachstack(c);
	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy); /* avoid race conditions */
		XSetErrorHandler(xerrordummy);
		XSelectInput(dpy, c->win, NoEventMask);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);

  if (!s){
    arrange(m);
    focus(NULL);
    updateclientlist();
  }
}

void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
	}
}

void
updatebars(void)
{
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
    .background_pixel = 0,
    .border_pixel = 0,
    .colormap = cmap,
		.event_mask = ButtonPressMask|ExposureMask
	};
	XClassHint ch = {"horizonwm_bar", "horizonwm_bar"};
	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, depth,
				InputOutput, visual,
				CWOverrideRedirect|CWBackPixmap|CWBorderPixel|CWColormap|CWEventMask, &wa);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
		XMapRaised(dpy, m->barwin);
		XSetClassHint(dpy, m->barwin, &ch);
	}
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (m->showbar) {
		m->wh -= bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		m->wy = m->topbar ? m->wy + bh : m->wy;
	} else
		m->by = -bh;
}

void
updateclientlist()
{
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
				XA_WINDOW, 32, PropModeAppend,
				(unsigned char *) &(c->win), 1);
}

int
updategeom(void)
{
	int dirty = 0;

#ifdef XINERAMA
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;

		/* new monitors if nn > n */
		for (i = n; i < nn; i++) {
			for (m = mons; m && m->next; m = m->next);
			if (m)
				m->next = createmon();
			else
				mons = createmon();
		}
		for (i = 0, m = mons; i < nn && m; m = m->next, i++)
			if (i >= n
			|| unique[i].x_org != m->mx || unique[i].y_org != m->my
			|| unique[i].width != m->mw || unique[i].height != m->mh)
			{
				dirty = 1;
				m->num = i;
				m->mx = m->wx = unique[i].x_org;
				m->my = m->wy = unique[i].y_org;
				m->mw = m->ww = unique[i].width;
				m->mh = m->wh = unique[i].height;
				updatebarpos(m);
			}
		/* removed monitors if n > nn */
		for (i = nn; i < n; i++) {
			for (m = mons; m && m->next; m = m->next);
			while ((c = m->clients)) {
				dirty = 1;
				m->clients = c->next;
				detachstack(c);
				c->mon = mons;
				attach(c);
				attachstack(c);
			}
			if (m == selmon)
				selmon = mons;
			cleanupmon(m);
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
	c->hintsvalid = 1;
}

void
updatestatus(void)
{
	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
		strcpy(stext, "horizonwm-"VERSION);
	drawbar(selmon);
}

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

void
view(const Arg *arg)
{
	if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
		return;
	selmon->seltags ^= 1; /* toggle sel tagset */
	if (arg->ui & TAGMASK)
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
	focus(NULL);
	arrange(selmon);
}

pid_t winpid(Window w){
  pid_t result = 0;

  #ifdef __linux__
  xcb_res_client_id_spec_t spec = {0};
  spec.client = w;
  spec.mask = XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID;

  xcb_generic_error_t *e = NULL;
  xcb_res_query_client_ids_cookie_t c = xcb_res_query_client_ids(xcon, 1, &spec);
  xcb_res_query_client_ids_reply_t *r = xcb_res_query_client_ids_reply(xcon, c, &e);

  if (!r){
    return (pid_t) 0;
  }

  xcb_res_client_id_value_iterator_t i = xcb_res_query_client_ids_ids_iterator(r);
  for (; i.rem; xcb_res_client_id_value_next(&i)){
    spec = i.data->spec;
    if (spec.mask & XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID){
      uint32_t *t = xcb_res_client_id_value_value(i.data);
      result = *t;
      break;
    }
  }

  free(r);

  if (result == (pid_t) -1){
    result = 0;
  }
  #endif //linux__

  #ifdef __OpenBSD__
  Atom type;
  int format;
  unsigned long len, bytes;
  unsigned char *prop;
  pid_t ret;

  if (XGetWindowProperty(dpy, w, XInternAtom(dpy, "_NET_WM_PID", 0), 0, 1, False, AnyPropertyType, &type, &format, &len, &bytes, &prop) != Success || !prop){
    return 0;
  }

  ret = *(pid_t*) prop;
  XFree(prop);
  result = ret;
  #endif //OpenBSD__

  return result;
}

pid_t getparentprocess(pid_t p){
  unsigned int v = 0;

  #ifdef __linux__
  FILE *f;
  char buf[256];
  snprintf(buf, sizeof(buf) -1, "/proc/%u/stat", (unsigned) p);

  if (!(f = fopen(buf, "r"))){
    return 0;
  }

  fscanf(f, "%*u %*s %*c %u", &v);
  fclose(f);
  #endif //linux__

  #ifdef __OpenBSD__
  int n;
  kvm_t *kd;
  struct kinfo_proc *kp;

  kd = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, NULL);
  if (!kd){
    return 0;
  }

  kp = kvm_getprocs(kd, KERN_PROC_PID, p, sizeof(*kp), &n);
  v = kp->p_ppid;
  #endif //OpenBSD

  return (pid_t) v;
}

int isdescprocess(pid_t p, pid_t c){
  while (p != c && c != 0){
    c = getparentprocess(c);
  }

  return (int) c;
}

Client *termforwin(const Client *w){
  Client *c;
  Monitor *m;

  if (!w->pid || w->isterminal){
    return NULL;
  }

  for (m = mons; m; m = m->next){
    for (c = m->clients; c; c = c->next){
      if (c->isterminal && !c->swallowing && c->pid && isdescprocess(c->pid, w->pid)){
        return c;
      }
    }
  }

  return NULL;
}

Client *swallowingclient(Window w){
  Client *c;
  Monitor *m;

  for (m = mons; m; m = m->next){
    for (c = m->clients; c; c = c->next){
      if (c->swallowing && c->swallowing->win == w){
        return c;
      }
    }
  }

  return NULL;
}

Client *
wintoclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->win == w)
				return c;
	return NULL;
}

Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		if (w == m->barwin)
			return m;
	if ((c = wintoclient(w)))
		return c->mon;
	return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "horizonwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	die("horizonwm: another window manager is already running");
	return -1;
}

void xinitvisual(){
  XVisualInfo *infos;
  XRenderPictFormat *fmt;
  int nitems;
  int i;

  XVisualInfo tpl = {
    .screen = screen,
    .depth = 32,
    .class = TrueColor
  };

  long masks = VisualScreenMask | VisualDepthMask | VisualClassMask;

  infos = XGetVisualInfo(dpy, masks, &tpl, &nitems);
  visual = NULL;
  for (i = 0; i < nitems; i++){
    fmt = XRenderFindVisualFormat(dpy, infos[i].visual);
    if (fmt->type == PictTypeDirect && fmt->direct.alphaMask){
      visual = infos[i].visual;
      depth = infos[i].depth;
      cmap = XCreateColormap(dpy, root, visual, AllocNone);
      useargb = 1;
      break;
    }
  }

  XFree(infos);

  if (!visual){
    visual = DefaultVisual(dpy, screen);
    depth = DefaultDepth(dpy, screen);
    cmap = DefaultColormap(dpy, screen);
  }
}

void
zoom(const Arg *arg)
{
	Client *c = selmon->sel;

	if (!selmon->lt[selmon->sellt]->arrange || !c || c->isfloating)
		return;
	if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next)))
		return;
	pop(c);
}

int
main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("horizonwm-"VERSION);
	else if (argc != 1)
		die("usage: horizonwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("horizonwm: cannot open display");
  if (!(xcon = XGetXCBConnection(dpy))){
    die("horizonwm: cannot get xcb connection\n");
  }
	checkotherwm();
	setup();
#ifdef __OpenBSD__
	if (pledge("stdio rpath proc exec ps", NULL) == -1)
		die("pledge");
#endif /* __OpegBSD__ */
	scan();

  //Spawn first programs. List located in spawn_programs.c and declared in .h
  spawn_programs_list(startup_programs);

	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
