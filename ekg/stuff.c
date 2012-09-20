/* $Id: stuff.c 4970 2010-08-20 08:43:27Z wiechu $ */

/*
 *  (C) Copyright 2001-2003 Wojtek Kaniewski <wojtekka@irc.pl>
 *			    Robert J. Wo�ny <speedy@ziew.org>
 *			    Pawe� Maziarz <drg@o2.pl>
 *			    Dawid Jarosz <dawjar@poczta.onet.pl>
 *			    Piotr Domagalski <szalik@szalik.net>
 *			    Adam Mikuta <adammikuta@poczta.onet.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License Version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "ekg2-config.h"
#include "win32.h"

#define _XOPEN_SOURCE 600
#define __EXTENSIONS__
#include <sys/types.h>
#include <sys/stat.h>

#ifndef NO_POSIX_SYSTEM
#include <sys/socket.h>
#endif

#define __USE_BSD
#include <sys/time.h>

#ifndef NO_POSIX_SYSTEM
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#ifndef NO_POSIX_SYSTEM
#include <sched.h>
#include <pwd.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_ICONV
#	include <iconv.h>
#endif

#ifndef HAVE_STRLCPY
#  include "compat/strlcpy.h"
#endif
#ifndef HAVE_STRLCAT
#  include "compat/strlcat.h"
#endif

#include "debug.h"
#include "commands.h"
#include "dynstuff.h"
#include "protocol.h"
#include "stuff.h"
#include "themes.h"
#include "userlist.h"
#include "vars.h"
#include "windows.h"
#include "xmalloc.h"
#include "plugins.h"
#include "sessions.h"
#include "recode.h"

#include "dynstuff_inline.h"
#include "queries.h"

child_t *children = NULL;

static LIST_FREE_ITEM(child_free_item, child_t *) { xfree(data->name); }

DYNSTUFF_LIST_DECLARE(children, child_t, child_free_item,
	static __DYNSTUFF_LIST_ADD,		/* children_add() */
	__DYNSTUFF_LIST_REMOVE_ITER,		/* children_removei() */
	__DYNSTUFF_LIST_DESTROY)		/* children_destroy() */

alias_t *aliases = NULL;
list_t autofinds = NULL;

struct timer *timers = NULL;
static LIST_FREE_ITEM(timer_free_item, struct timer *) { data->function(1, data->data); xfree(data->name); }


DYNSTUFF_LIST_DECLARE2(timers, struct timer, timer_free_item,
	static __DYNSTUFF_LIST_ADD,		/* timers_add() */
	__DYNSTUFF_LIST_REMOVE_SAFE,		/* timers_remove() */
	__DYNSTUFF_LIST_REMOVE_ITER,		/* timers_removei() */
	__DYNSTUFF_LIST_DESTROY)		/* timers_destroy() */

struct conference *conferences = NULL;
newconference_t *newconferences = NULL;

struct buffer_info buffer_debug = { NULL, 0, DEBUG_MAX_LINES };		/**< debug buffer */
struct buffer_info buffer_speech = { NULL, 0, 50 };		/**< speech buffer */

int old_stderr;
char *config_subject_prefix;
char *config_subject_reply_prefix;

int in_autoexec = 0;
int config_auto_save = 0;
int config_auto_user_add = 0;
time_t last_save = 0;
int config_display_color = 1;
int config_beep = 1;
int config_beep_msg = 1;
int config_beep_chat = 1;
int config_beep_groupchat = 1;
int config_beep_notify = 1;
char *config_console_charset;
char *config_dcc_dir;
int config_display_blinking = 1;
int config_events_delay = 3;
int config_expert_mode = 0;
int config_history_savedups = 1;
char *config_sound_msg_file = NULL;
char *config_sound_chat_file = NULL;
char *config_sound_notify_file = NULL;
char *config_sound_sysmsg_file = NULL;
char *config_sound_mail_file = NULL;
char *config_sound_app = NULL;
int config_use_unicode;
int config_use_iso;
int config_changed = 0;
int config_display_ack = 12;
int config_completion_notify = 1;
char *config_completion_char = NULL;
time_t ekg_started = 0;
int config_display_notify = 1;
char *config_theme = NULL;
int config_default_status_window = 0;
char *home_dir = NULL;
char *config_quit_reason = NULL;
char *config_away_reason = NULL;
char *config_back_reason = NULL;
int config_query_commands = 0;
int config_slash_messages = 0;
int quit_message_send = 0;
int batch_mode = 0;
char *batch_line = NULL;
int config_make_window = 6;
char *config_tab_command = NULL;
int config_save_password = 1;
int config_save_quit = 1;
char *config_timestamp = NULL;
int config_timestamp_show = 1;
int config_display_sent = 1;
int config_send_white_lines = 0;
int config_sort_windows = 1;
int config_keep_reason = 1;
char *config_speech_app = NULL;
int config_time_deviation = 300;
int config_mesg = MESG_DEFAULT;
int config_display_welcome = 1;
char *config_display_color_map = NULL;
char *config_session_default = NULL;
int config_sessions_save = 0;
int config_window_session_allow = 0;
int config_windows_save = 0;
char *config_windows_layout = NULL;
char *config_profile = NULL;
int config_debug = 1;
int config_lastlog_noitems = 0;
int config_lastlog_case = 0;
int config_lastlog_display_all = 0;
int config_version = 0;
char *config_exit_exec = NULL;
int config_session_locks = 0;
char *config_nickname = NULL;

char *last_search_first_name = NULL;
char *last_search_last_name = NULL;
char *last_search_nickname = NULL;
char *last_search_uid = 0;

int ekg2_reason_changed = 0;

/*
 * windows_save()
 *
 * saves current open windows to the variable @a config_windows_layout if @a config_windows_save is on
 * @sa config_windows_layout
 * @sa config_windows_save
 */

void windows_save() {
	window_t *w;

	if (config_windows_save) {
		string_t s = string_init(NULL);
		int maxid = 0, i;
		
		for (w = windows; w; w = w->next) {
			if (!w->floating && w->id > maxid)
				maxid = w->id;
		}

		for (i = 1; i <= maxid; i++) {
			const char *target = "-";
			const char *session_name = NULL;
			
			for (w = windows; w; w = w->next) {
				if (w->id == i) {
					target = w->target;
					if (w->session)
						session_name = w->session->uid;
					break;
				}
			}
		
			if (session_name && target) {
				string_append(s, session_name);
				string_append_c(s, '/');
			}

			if (target) {
				string_append_c(s, '\"');
				string_append(s, target);
				string_append_c(s, '\"');
			}

			if (i < maxid)
				string_append_c(s, '|');
		}

		for (w = windows; w; w = w->next) {
			if (w->floating && (!w->target || xstrncmp(w->target, "__", 2))) {
				char *tmp = saprintf("|*%d,%d,%d,%d,%d,%s", w->left, w->top, w->width, w->height, w->frames, w->target);
				string_append(s, tmp);
				xfree(tmp);
			}
		}
		xfree(config_windows_layout);
		config_windows_layout = string_free(s, 0);
	}
}

static LIST_FREE_ITEM(list_alias_free, alias_t *) { xfree(data->name); list_destroy(data->commands, 1); }

DYNSTUFF_LIST_DECLARE(aliases, alias_t, list_alias_free,
	static __DYNSTUFF_LIST_ADD,		/* aliases_add() */
	static __DYNSTUFF_LIST_REMOVE_ITER,	/* aliases_removei() */
	__DYNSTUFF_LIST_DESTROY)		/* aliases_destroy() */

/*
 * alias_add()
 *
 * dopisuje alias do listy alias�w.
 *
 *  - string - linia w formacie 'alias cmd',
 *  - quiet - czy wypluwa� mesgi na stdout,
 *  - append - czy dodajemy kolejn� komend�?
 *
 * 0/-1
 */
int alias_add(const char *string, int quiet, int append)
{
	char *cmd, *aname, *tmp;
	command_t *c;
	alias_t *a;
	char **params = NULL;
	char *array;
	int i;

	if (!string || !(cmd = xstrchr(string, ' ')))
		return -1;

	*cmd++ = 0;

	for (a = aliases; a; a = a->next) {
		if (!xstrcasecmp(string, a->name)) {
			if (!append) {
				printq("aliases_exist", string);
				return -1;
			} else {
				list_add(&a->commands, xstrdup(cmd));
				
				/* przy wielu komendach trudno dope�nia�, bo wg. kt�rej? */
				for (c = commands; c; c = c->next) {
					if (!xstrcasecmp(c->name, a->name)) {
						xfree(c->params);
						c->params = array_make(("?"), (" "), 0, 1, 1);
						break;
					}
				}
			
				printq("aliases_append", string);

				return 0;
			}
		}
	}


	aname = xstrdup((*cmd == '/') ? cmd + 1 : cmd);
	if ((tmp = xstrchr(aname, ' ')))
		*tmp = 0;

	for (i=0; i<2; i++) {
		for (c = commands; c && !params; c = c->next) {
			const char *cname = c->name;
			if (i) {
				if ((tmp = xstrchr(cname, ':')))
					cname = tmp+1;
				else  
					continue;
			}

			if (!xstrcasecmp(string, cname) && !(c->flags & COMMAND_ISALIAS)) {
				printq("aliases_command", string);
				xfree(aname);
				return -1;
			}

			if (!xstrcasecmp(aname, cname)) {
				params = c->params;
				break;
			}
		}
	}
	xfree(aname);

	a = xmalloc(sizeof(struct alias));
	a->name = xstrdup(string);
	a->commands = NULL;
	list_add(&(a->commands), xstrdup(cmd));
	aliases_add(a);

	array = (params) ? array_join(params, (" ")) : xstrdup(("?"));
	command_add(NULL, a->name, array, cmd_alias_exec, COMMAND_ISALIAS, NULL);
	xfree(array);
	
	printq("aliases_add", a->name, (""));

	return 0;
}

/*
 * alias_remove()
 *
 * usuwa alias z listy alias�w.
 *
 *  - name - alias lub NULL,
 *  - quiet.
 *
 * 0/-1
 */
int alias_remove(const char *name, int quiet)
{
	alias_t *a;
	int removed = 0;

	for (a = aliases; a; a = a->next) {
		if (!name || !xstrcasecmp(a->name, name)) {
			if (name)
				printq("aliases_del", name);
			command_remove(NULL, a->name);
			
			a = aliases_removei(a);
			removed = 1;
		}
	}

	if (!removed) {
		if (name)
			printq("aliases_noexist", name);
		else
			printq("aliases_list_empty");

		return -1;
	}

	if (removed && !name)
		printq("aliases_del_all");

	return 0;
}

static LIST_FREE_ITEM(list_buffer_free, struct buffer *) { xfree(data->line); xfree(data->target); }

static __DYNSTUFF_ADD(buffers, struct buffer, NULL)			/* buffers_add() */
static __DYNSTUFF_REMOVE_ITER(buffers, struct buffer, list_buffer_free)	/* buffers_removei() */
static __DYNSTUFF_DESTROY(buffers, struct buffer, list_buffer_free)	/* buffers_destroy() */
static __DYNSTUFF_COUNT(buffers, struct buffer)				/* buffers_count() */
static __DYNSTUFF_GET_NTH(buffers, struct buffer)			/* buffers_get_nth() */

static void buffer_add_common(struct buffer_info *type, const char *target, const char *line, time_t ts) {
	struct buffer *b;
	struct buffer **addpoint = (type->last ? &(type->last) : &(type->data));

	/* What the heck with addpoint thing?
	 * - if type->last ain't NULL, it points to last element of the list;
	 *   we can pass it directly to LIST_ADD2() to avoid iterating through all items,
	 *   it just sets its' 'next' field and everything is fine,
	 * - but if it's NULL, then data is NULL too. That means LIST_ADD2() would need
	 *   to modify the list pointer, so we need to pass it &(type->data) instead.
	 *   Else type->last would point to the list, but type->data would be still NULL,
	 * - if last is NULL, but data ain't, that means something broke. But that's
	 *   no problem, as we're still passing &(type->data), so adding works fine
	 *   and then type->last is fixed.
	 */

	if (type->max_lines) { /* XXX: move to idles? */
		int n;
bac_countupd:
		n = type->count - type->max_lines + 1;
		
		if (n > 0) { /* list slice removal */
			b = buffers_get_nth(type->data, n);		/* last element to remove */
			if (!b) { /* count has been broken */
				type->count = buffers_count(type->data);
				goto bac_countupd;
			}
			type->data	= b->next;
			b->next		= NULL;			/* unlink elements to be removed */
			type->count -= n;
	/* XXX,
	 *	b->next == NULL
	 *	so buffers_destroy(&b) will free only b,
	 *	shouldn't be saved type->data value?
	 */
			buffers_destroy(&b);			/* and remove them */
		}
	}

	b		= xmalloc(sizeof(struct buffer));
	b->ts		= ts;
	b->target	= xstrdup(target);
	b->line		= xstrdup(line);

	buffers_add(addpoint, b);

	type->last	= b;
	type->count++;
}

/**
 * buffer_add()
 *
 * Add new line to given buffer_t, if max_lines > 0 than it maintain list that we can have max: @a max_lines items on it.
 *
 * @param type		- pointer to buffer beginning ptr
 * @param target	- name of target.. or just name of smth we want to keep in b->target
 * @param line		- line which we want to save.
 *
 * @return	0 - when line was successfully added to buffer, else -1	(when @a type was NULL)
 */

int buffer_add(struct buffer_info *type, const char *target, const char *line) {
	if (!type)
		return -1;

	buffer_add_common(type, target, line, time(NULL));

	return 0;			/* so always return success here */
}

/**
 * buffer_add_str()
 *
 * Add new line to given buffer_t, if max_lines > 0 than it maintain list that we can have max: @a max_lines items on it.
 *
 * @param type		- pointer to buffer beginning ptr
 * @param target	- name of target, or just name of smth we want to keep in b->target
 * @param str		- string in format: [time_when_it_happen proper line... blah, blah] <i>time_when_it_happen</i> should be in digits.
 *
 * @return	0 - when line was successfully added to buffer, else -1 (when @a type was NULL, or @a line was in wrong format)
 */

int buffer_add_str(struct buffer_info *type, const char *target, const char *str) {
	char *sep;
	time_t ts = 0;

	if (!type || !str)
		return -1;

	for (sep = (char *) str; xisdigit(*sep); sep++) {
		/* XXX check if there's no time_t overflow? */
		ts *= 10;
		ts += (*sep - '0');
	}

	if (sep == str || *sep != ' ') {
		debug_error("buffer_add_str() parsing str: %s failed\n", str);
		return -1;
	}

	buffer_add_common(type, target, sep+1, ts);
	return 0;			/* so always return success here */
}

/**
 * buffer_tail()
 *
 * Return oldest b->line, free b->target and remove whole buffer_t from list
 * 
 * @param type	- pointer to buffer beginning ptr
 *
 * @return First b->line on the list, or NULL, if no items on list.
 */

char *buffer_tail(struct buffer_info *type) {
	struct buffer *b;
	char *str;

	if (!type || !type->data)
		return NULL;

	b = type->data;
	str = b->line;			/* save b->line */
	b->line = NULL;

	(void) buffers_removei(&(type->data), b);

	if (type->last == b) 
		type->last = NULL;

	type->count--;

	return str;			/* return saved b->line */
}

/**
 * buffer_free()
 *
 * Free memory after given buffer.<br>
 * After it set *type to NULL
 *
 * @param type - pointer to buffer beginning ptr
 * 
 */

void buffer_free(struct buffer_info *type) {
	if (!type || !type->data)
		return;

	buffers_destroy(&(type->data));

	type->last	= NULL;
	type->count	= 0;
}

void changed_make_window(const char *var)
{
	static int old_value = 6;

	if (config_make_window == 4) {
		config_make_window = old_value;
		print("variable_invalid", var);
	}

	old_value = config_make_window;
}

/*
 * changed_mesg()
 *
 * funkcja wywo�ywana przy zmianie warto�ci zmiennej ,,mesg''.
 */
void changed_mesg(const char *var)
{
	if (config_mesg == MESG_DEFAULT)
		mesg_set(mesg_startup);
	else
		mesg_set(config_mesg);
}
	
/*
 * changed_auto_save()
 *
 * wywo�ywane po zmianie warto�ci zmiennej ,,auto_save''.
 */
void changed_auto_save(const char *var)
{
	/* oszukujemy, ale takie zachowanie wydaje si� by�
	 * bardziej ,,naturalne'' */
	last_save = time(NULL);
}

/*
 * changed_display_blinking()
 *
 * wywo�ywane po zmianie warto�ci zmiennej ,,display_blinking''.
 */
void changed_display_blinking(const char *var)
{
	session_t *s;

	/* wy��czamy wszystkie blinkaj�ce uid'y */
	for (s = sessions; s; s = s->next) {
		userlist_t *ul;

		for (ul = s->userlist; ul; ul = ul->next) {
			userlist_t *u	= ul;
			u->blink	= 0;
		}
	}
}

/*
 * changed_theme()
 *
 * funkcja wywo�ywana przy zmianie warto�ci zmiennej ,,theme''.
 */
void changed_theme(const char *var)
{
	if (in_autoexec)
		return;
	if (!config_theme) {
		theme_free();
		theme_init();
	} else {
		if (!theme_read(config_theme, 1)) {
			print("theme_loaded", config_theme);
		} else {
			print("error_loading_theme", strerror(errno));
			variable_set(("theme"), NULL);
		}
	}
}

/**
 * compile_time()
 *
 * Return compilation date, and time..<br>
 * Used by <i>/version command</i> and <i>ekg2 --version</i>
 *
 * @return	__DATE__" "__TIME__<br> 
 *		For example: <b>"Jun 21 1987" " " "22:06:47"</b>
 */

const char *compile_time() {
	return __DATE__ " " __TIME__;
}

/* NEW CONFERENCE API HERE, WHEN OLD CONFERENCE API BECOME OBSOLETE CHANGE FUNCTION NAME, ETC.... */

static LIST_FREE_ITEM(newconference_free_item, newconference_t *) { xfree(data->name); xfree(data->session); userlists_destroy(&(data->participants)); }

DYNSTUFF_LIST_DECLARE(newconferences, newconference_t, newconference_free_item,
	static __DYNSTUFF_LIST_ADD,		/* newconferences_add() */
	static __DYNSTUFF_LIST_REMOVE_SAFE,	/* newconferences_remove() */
	__DYNSTUFF_LIST_DESTROY)		/* newconferences_destroy() */

userlist_t *newconference_member_find(newconference_t *conf, const char *nickname) {
	userlist_t *ul;

	if (!conf || !nickname) return NULL;

	for (ul = conf->participants; ul; ul = ul->next) {
		userlist_t *u = ul;

		if (!xstrcasecmp(u->nickname, nickname))
			return u;
	}
	return NULL;
}

userlist_t *newconference_member_add(newconference_t *conf, const char *uid, const char *nick) {
	userlist_t *u;
	if (!conf || !uid) return NULL;

	if (!(u = newconference_member_find(conf, uid)))
		u = userlist_add_u(&(conf->participants), uid, nick);
	return u;
}
	/* remove userlist_t from conference. wrapper. */
int newconference_member_remove(newconference_t *conf, userlist_t *u) {
	if (!conf || !u) return -1;
	return userlist_remove_u(&(conf->participants), u);
}

newconference_t *newconference_find(session_t *s, const char *name) {
	newconference_t *c;
	
	for (c = newconferences; c; c = c->next) {
		if ((!s || !xstrcmp(s->uid, c->session)) && !xstrcmp(name, c->name)) return c;
	}
	return NULL;
}

newconference_t *newconference_create(session_t *s, const char *name, int create_wnd) {
	newconference_t *c;
	window_t *w;

	if (!s || !name) return NULL;

	if ((c = newconference_find(s, name))) return c;

	if (!(w = window_find_s(s, name)) && create_wnd) {
		w = window_new(name, s, 0);
	}

	c		= xmalloc(sizeof(newconference_t));
	c->session	= xstrdup(s->uid);
	c->name		= xstrdup(name);
	
	newconferences_add(c);
	return c;
}

void newconference_destroy(newconference_t *conf, int kill_wnd) {
	window_t *w = NULL; 
	if (!conf) return;
	if (kill_wnd) w = window_find_s(session_find(conf->session), conf->name);

	newconferences_remove(conf);
	window_kill(w);
}

/* OLD CONFERENCE API HERE, REQUEST REWRITING/USING NEW-ONE */

static LIST_FREE_ITEM(conference_free_item, struct conference *) { xfree(data->name); list_destroy(data->recipients, 1); }

DYNSTUFF_LIST_DECLARE(conferences, struct conference, conference_free_item,
	static __DYNSTUFF_LIST_ADD,		/* conferences_add() */
	static __DYNSTUFF_LIST_REMOVE_ITER,	/* conferences_removei() */
	__DYNSTUFF_LIST_DESTROY)		/* conferences_destroy() */

/*
 * conference_add()
 *
 * dopisuje konferencje do listy konferencji.
 *
 *  - name - nazwa konferencji,
 *  - nicklist - lista nick�w, grup, czegokolwiek,
 *  - quiet - czy wypluwa� mesgi na stdout.
 *
 * zaalokowan� struct conference lub NULL w przypadku b��du.
 */
struct conference *conference_add(session_t *session, const char *name, const char *nicklist, int quiet)
{
	struct conference c, *cf;
	char **nicks;
	int i, count;
	char **p;

	if (!name || !nicklist)
		return NULL;

	if (nicklist[0] == ',' || nicklist[xstrlen(nicklist) - 1] == ',') {
		printq("invalid_params", ("chat"));
		return NULL;
	}

	nicks = array_make(nicklist, " ,", 0, 1, 0);

	/* grupy zamieniamy na niki */
	for (i = 0; nicks[i]; i++) {
		if (nicks[i][0] == '@') {
			session_t *s;
			char *gname = xstrdup(nicks[i] + 1);
			int first = 0;
			int nig = 0; /* nicks in group */
		
			for (s = sessions; s; s = s->next) {
				userlist_t *ul;
				for (ul = s->userlist; ul; ul = ul->next) {
					userlist_t *u = ul;
					struct ekg_group *gl;

					if (!u->nickname)
						continue;

					for (gl = u->groups; gl; gl = gl->next) {
						struct ekg_group *g = gl;

						if (!xstrcasecmp(gname, g->name)) {
							if (first++)
								array_add(&nicks, xstrdup(u->nickname));
							else {
								xfree(nicks[i]);
								nicks[i] = xstrdup(u->nickname);
							}

							nig++;

							break;
						}
					}
				}
			}

			xfree(gname);

			if (!nig) {
				printq("group_empty", gname);
				printq("conferences_not_added", name);
				array_free(nicks);
				return NULL;
			}
		}
	}

	count = array_count(nicks);

	for (cf = conferences; cf; cf = cf->next) {
		if (!xstrcasecmp(name, cf->name)) {
			printq("conferences_exist", name);

			array_free(nicks);

			return NULL;
		}
	}

	memset(&c, 0, sizeof(c));

	for (p = nicks, i = 0; *p; p++) {
		const char *uid;

		if (!xstrcmp(*p, ""))
			continue;
			/* XXX, check if bad uid */
		uid = get_uid(session, *p);

		if (uid)
			list_add(&(c.recipients), xstrdup(uid));
		i++;
	}


	array_free(nicks);

	if (i != count) {
		printq("conferences_not_added", name);
		list_destroy(c.recipients, 1);
		return NULL;
	}

	printq("conferences_add", name);

	c.name = xstrdup(name);

	tabnick_add(name);

	cf = xmemdup(&c, sizeof(c));
	conferences_add(cf);
	return cf;
}

/*
 * conference_remove()
 *
 * usuwa konferencj� z listy konferencji.
 *
 *  - name - konferencja lub NULL dla wszystkich,
 *  - quiet.
 *
 * 0/-1
 */
int conference_remove(const char *name, int quiet)
{
	struct conference *c;
	int removed = 0;

	for (c = conferences; c; c = c->next) {
		if (!name || !xstrcasecmp(c->name, name)) {
			if (name)
				printq("conferences_del", name);
			tabnick_remove(c->name);

			c = conferences_removei(c);
			removed = 1;
		}
	}

	if (!removed) {
		if (name)
			printq("conferences_noexist", name);
		else
			printq("conferences_list_empty");
		
		return -1;
	}

	if (removed && !name)
		printq("conferences_del_all");

	return 0;
}

/*
 * conference_create()
 *
 * tworzy now� konferencj� z wygenerowan� nazw�.
 *
 *  - nicks - lista nik�w tak, jak dla polecenia conference.
 */
struct conference *conference_create(session_t *session, const char *nicks)
{
	struct conference *c;
	static int count = 1;
	char *name = saprintf("#conf%d", count);

	if ((c = conference_add(session, name, nicks, 0)))
		count++;

	xfree(name);

	return c;
}

/*
 * conference_find()
 *
 * znajduje i zwraca wska�nik do konferencji lub NULL.
 *
 *  - name - nazwa konferencji.
 */
struct conference *conference_find(const char *name) 
{
	struct conference *c;

	for (c = conferences; c; c = c->next) {
		if (!xstrcmp(c->name, name))
			return c;
	}
	
	return NULL;
}

/*
 * conference_participant()
 *
 * sprawdza, czy dany numer jest uczestnikiem konferencji.
 *
 *  - c - konferencja,
 *  - uin - numer.
 *
 * 1 je�li jest, 0 je�li nie.
 */
int conference_participant(struct conference *c, const char *uid)
{
	list_t l;
	
	for (l = c->recipients; l; l = l->next) {
		char *u = l->data;

		if (!xstrcasecmp(u, uid))
			return 1;
	}

	return 0;

}

/*
 * conference_find_by_uids()
 *
 * znajduje konferencj�, do kt�rej nale�� podane uiny. je�eli nie znaleziono,
 * zwracany jest NULL. je�li numer�w jest wi�cej, zostan� dodane do
 * konferencji, bo najwyra�niej kto� do niej do��czy�.
 * 
 *  - from - kto jest nadawc� wiadomo�ci,
 *  - recipients - tablica numer�w nale��cych do konferencji,
 *  - count - ilo�� numer�w,
 *  - quiet.
 */
struct conference *conference_find_by_uids(session_t *s, const char *from, const char **recipients, int count, int quiet) 
{
	int i;
	struct conference *c;

	for (c = conferences; c; c = c->next) {
		int matched = 0;

		for (i = 0; i < count; i++)
			if (conference_participant(c, recipients[i]))
				matched++;

		if (conference_participant(c, from))
			matched++;

		debug_function("// conference_find_by_uids(): from=%s, rcpt count=%d, matched=%d, list_count(c->recipients)=%d\n", from, count, matched, LIST_COUNT2(c->recipients));

		if (matched == LIST_COUNT2(c->recipients) && matched <= (!xstrcasecmp(from, s->uid) ? count : count + 1)) {
			string_t new = string_init(NULL);
			int comma = 0;

			if (xstrcasecmp(from, s->uid) && !conference_participant(c, from)) {
				list_add(&c->recipients, xmemdup(&from, sizeof(from)));

				comma++;
				string_append(new, format_user(s, from));
			} 

			for (i = 0; i < count; i++) {
				if (xstrcasecmp(recipients[i], s->uid) && !conference_participant(c, recipients[i])) {
					list_add(&c->recipients, xmemdup(&recipients[i], sizeof(recipients[0])));
			
					if (comma++)
						string_append(new, ", ");
					string_append(new, format_user(s, recipients[i]));
				}
			}

			if (xstrcmp(new->str, "") && !c->ignore)
				printq("conferences_joined", new->str, c->name);
			string_free(new, 1);

			debug("// conference_find_by_uins(): matching %s\n", c->name);

			return c;
		}
	}

	return NULL;
}

/*
 * conference_set_ignore()
 *
 * ustawia stan konferencji na ignorowany lub nie.
 *
 *  - name - nazwa konferencji,
 *  - flag - 1 ignorowa�, 0 nie ignorowa�,
 *  - quiet.
 *
 * 0/-1
 */
int conference_set_ignore(const char *name, int flag, int quiet)
{
	struct conference *c = conference_find(name);

	if (!c) {
		printq("conferences_noexist", name);
		return -1;
	}

	c->ignore = flag;
	printq((flag ? "conferences_ignore" : "conferences_unignore"), name);

	return 0;
}

/*
 * conference_rename()
 *
 * zmienia nazw� instniej�cej konferencji.
 * 
 *  - oldname - stara nazwa,
 *  - newname - nowa nazwa,
 *  - quiet.
 *
 * 0/-1
 */
int conference_rename(const char *oldname, const char *newname, int quiet)
{
	struct conference *c;
	
	if (conference_find(newname)) {
		printq("conferences_exist", newname);
		return -1;
	}

	if (!(c = conference_find(oldname))) {
		printq("conference_noexist", oldname);
		return -1;
	}

	xfree(c->name);		c->name = xstrdup(newname);

	tabnick_remove(oldname);
	tabnick_add(newname);
	
	printq("conferences_rename", oldname, newname);

	query_emit_id(NULL, CONFERENCE_RENAMED, &oldname, &newname);	/* XXX READ-ONLY QUERY */

	return 0;
}

/*
 * help_path()
 *
 * zwraca plik z pomoc� we w�a�ciwym j�zyku lub null je�li nie ma takiego pliku
 *
 */
FILE *help_path(char *name, char *plugin) {
	char lang[3];
	char *tmp;
	FILE *fp;

	char *base = plugin ? 
		saprintf(DATADIR "/plugins/%s/%s", plugin, name) :
		saprintf(DATADIR "/%s", name);

	do {
		/* if we don't get lang from $LANGUAGE (man 3 gettext) */
		if ((tmp = getenv("LANGUAGE"))) break;
		/* fallback on locale enviroments.. (man 5 locale) */
		if ((tmp = getenv("LC_ALL"))) break;
		if ((tmp = getenv("LANG"))) break;
		/* fallback to en language */
		tmp = "en";
	} while (0);

	xstrncpy(&lang[0], tmp, 2);
	lang[2] = 0;
	
help_again:
	tmp = saprintf("%s-%s.txt", base, lang);

	if ((fp = fopen(tmp, "r"))) {
		xfree(base);
		xfree(tmp);
		return fp;
	}

	/* Temporary fallback - untill we don't have full en translation */
	xfree(tmp);
	if (xstrcasecmp(lang, "pl")) {
		lang[0] = 'p';
		lang[1] = 'l';
		goto help_again;
	}

	/* last chance, just base without lang. */
	tmp = saprintf("%s.txt", base);
	fp = fopen(tmp, "r");

	xfree(tmp);
	xfree(base);
	return fp;
}


/*
 * ekg_hash()
 *
 * liczy prosty hash z nazwy, wykorzystywany przy przeszukiwaniu list
 * zmiennych, format�w itp.
 *
 *  - name - nazwa.
 */
int ekg_hash(const char *name)
{
	int hash = 0;

	for (; *name; name++) {
		hash ^= *name;
		hash <<= 1;
	}

	return hash;
}

/*
 * mesg_set()
 *
 * w��cza/wy��cza/sprawdza mo�liwo�� pisania do naszego terminala.
 *
 *  - what - MESG_ON, MESG_OFF lub MESG_CHECK
 * 
 * -1 je�li b�ad, lub aktualny stan: MESG_ON/MESG_OFF
*/
int mesg_set(int what)
{
#ifndef NO_POSIX_SYSTEM
	const char *tty;
	struct stat s;

	if (!(tty = ttyname(old_stderr)) || stat(tty, &s)) {
		debug_error("mesg_set() error: %s\n", strerror(errno));
		return -1;
	}

	switch (what) {
		case MESG_OFF:
			chmod(tty, s.st_mode & ~S_IWGRP);
			break;
		case MESG_ON:
			chmod(tty, s.st_mode | S_IWGRP);
			break;
		case MESG_CHECK:
			return ((s.st_mode & S_IWGRP) ? MESG_ON : MESG_OFF);
	}
	
	return 0;
#else
	return -1;
#endif
}

/**
 * strip_spaces()
 *
 * strips spaces from the begining and the end of string @a line
 *
 * @param line - given string
 *
 * @note If you pass here smth which was strdup'ed() malloc'ed() or smth which was allocated.<br>
 *		You <b>must</b> xfree() string passed, not result of this function.
 *
 * @return buffer without spaces.
 */

char *strip_spaces(char *line) {
	size_t linelen;
	char *buf;

	if (!(linelen = xstrlen(line))) return line;
	
	for (buf = line; xisspace(*buf); buf++);

	while (linelen > 0 && xisspace(line[linelen - 1])) {
		line[linelen - 1] = 0;
		linelen--;
	}
	
	return buf;
}

/*
 * play_sound()
 *
 * odtwarza dzwi�k o podanej nazwie.
 *
 * 0/-1
 */
int play_sound(const char *sound_path)
{
	char *params[2];
	int res;

	if (!config_sound_app || !sound_path) {
		errno = EINVAL;
		return -1;
	}

	params[0] = saprintf(("^%s %s"), config_sound_app, sound_path);
	params[1] = NULL;

	res = cmd_exec(("exec"), (const char **) params, NULL, NULL, 1);

	xfree(params[0]);

	return res;
}

/*
 * child_add()
 *
 * dopisuje do listy uruchomionych dzieci proces�w.
 *
 *  - plugin
 *  - pid
 *  - name
 *  - handler
 *  - data
 *
 * 0/-1
 */
child_t *child_add(plugin_t *plugin, pid_t pid, const char *name, child_handler_t handler, void *priv_data)
{
	child_t *c = xmalloc(sizeof(child_t));

	c->plugin	= plugin;
	c->pid		= pid;
	c->name		= xstrdup(name);
	c->handler	= handler;
	c->priv_data	= priv_data;
	
	children_add(c);
	return c;
}

/**
 * mkdir_recursive()
 *
 * Create directory @a pathname and all needed parent directories.<br>
 *
 * @todo Maybe at begining of function let's check with stat() if that dir/file already exists?
 *
 * @param pathname	- path to directory or file (see @a isdir comment)
 * @param isdir		- if @a isdir is set, than we should also create dir specified by full @a pathname path,
 *			  else we shouldn't do it, because it's filename and we want to create directory only to last '/' char
 *
 * @return Like mkdir() do we return -1 on fail with errno set.
 */
int mkdir_recursive(const char *pathname, int isdir) {
	char fullname[PATH_MAX+1];
	struct stat st;
	int i = 0;
	char *tmp, *check = NULL;

	if (!pathname) {
		errno = EFAULT;
		return -1;
	}

	if (isdir)
		check = xstrdup(pathname);
	 else if ((tmp = xstrrchr(pathname, '/')))
		check = xstrndup(pathname, (tmp-pathname)+1);

	if (check) {
		if (stat(check, &st) == 0) {			/* if smth exists with such filename */
			xfree(check);
			if (!S_ISDIR(st.st_mode)) {		/* and it's not dir, abort. */
				errno = ENOTDIR;
				return -1;
			}
			return 0;
		}
		xfree(check);
	}

	do {
		if (i == PATH_MAX) {
			errno = ENAMETOOLONG;
			return -1;
		}

		fullname[i] = pathname[i];

		if (pathname[i] == '/' || (isdir && pathname[i] == '\0')) {	/* if it's / or it's last char.. */
			if (!isdir && !xstrchr(&pathname[i], '/'))		/* if it's not dir (e.g filename) we don't want to create the dir.. */
				return 0;

			fullname[i+1] = '\0';

			if (stat(fullname, &st) == 0) {		/* if smth exists with such filename */
				if (!S_ISDIR(st.st_mode)) {	/* and it's not dir, abort. */
					errno = ENOTDIR;
					return -1;
				}
			} else {				/* if not, try mkdir() and if fail exit. */
				if
#ifndef NO_POSIX_SYSTEM
				(mkdir(fullname, 0700) == -1)
#else
				(mkdir(fullname) == -1)
#endif
					return -1;
			}
		}
	} while (pathname[i++]);	/* while not NUL */
	return 0;
}

/**
 * prepare_pathf()
 *
 * Return path to configdir/profiledir (~/.ekg2 or ~/.ekg2/$PROFILE) and append @a filename (formated using vsnprintf()) 
 * If length of this string is larger than PATH_MAX (4096 on Linux) than unlike prepare_path() it'll return NULL
 */

const char *prepare_pathf(const char *filename, ...) {
	static char path[PATH_MAX];
	size_t len;
	int fpassed = (filename && *filename);

	len = strlcpy(path, config_dir ? config_dir : "", sizeof(path));

	if (len + fpassed >= sizeof(path)) {
		debug_error("prepare_pathf() LEVEL0 %d + %d >= %d\n", len, fpassed, sizeof(path));
		return NULL;
	}

	if (fpassed) {
		va_list ap;
		size_t len2;

		path[len++] = '/';

		va_start(ap, filename);
		len2 = vsnprintf(&path[len], sizeof(path)-len, filename, ap);
		va_end(ap);

		if (len2 == -1 || (len + len2) >= sizeof(path)) {	/* (len + len2 == sizeof(path)) ? */
			debug_error("prepare_pathf() LEVEL1 %d | %d + %d >= %d\n", len2, len, len2, sizeof(path));
			return NULL;
		}
	}

	return path;
}

/*
 * prepare_path()
 *
 * zwraca pe�n� �cie�k� do podanego pliku katalogu ~/.ekg2/
 *
 *  - filename - nazwa pliku,
 *  - do_mkdir - czy tworzy� katalog ~/.ekg2 ?
 */
const char *prepare_path(const char *filename, int do_mkdir)
{
	static char path[PATH_MAX];

	if (do_mkdir) {
		if (config_profile) {
			char *cd = xstrdup(config_dir), *tmp;

			if ((tmp = xstrrchr(cd, '/')))
				*tmp = 0;
#ifndef NO_POSIX_SYSTEM
			if (mkdir(cd, 0700) && errno != EEXIST) {
#else
			if (mkdir(cd) && errno != EEXIST) {
#endif
				xfree(cd);
				return NULL;
			}

			xfree(cd);
		}
#ifndef NO_POSIX_SYSTEM
		if (mkdir(config_dir, 0700) && errno != EEXIST)
#else
		if (mkdir(config_dir) && errno != EEXIST)
#endif
			return NULL;
	}
	
	if (!filename || !*filename)
		snprintf(path, sizeof(path), "%s", config_dir);
	else
		snprintf(path, sizeof(path), "%s/%s", config_dir, filename);
	
	return path;
}

/**
 * prepare_path_user()
 *
 * Converts path given by user to absolute path.
 *
 * @bug		Behaves correctly only with POSIX slashes, need to be modified for NO_POSIX_SYSTEM.
 *
 * @param	path	- input path.
 *
 * @return	Pointer to output path or NULL, if longer than PATH_MAX.
 */

const char *prepare_path_user(const char *path) {
	static char out[PATH_MAX];
	const char *in = path;
	const char *homedir = NULL;

	if (!in || (xstrlen(in)+1 > sizeof(out))) /* sorry, but I don't want to additionally play with '..' here */
		return NULL;

#ifndef NO_POSIX_SYSTEM
	if (*in == '/') /* absolute path */
#endif
		xstrcpy(out, in);
#ifndef NO_POSIX_SYSTEM
	else {
		if (*in == '~') { /* magical home directory handler */
			++in;
			if (*in == '/') { /* own homedir */
				if (!home_dir)
					return NULL;
				homedir = home_dir;
			} else {
				struct passwd *p;
				const char *slash = xstrchr(in, '/');
				
				if (slash) {
					char *user = xstrndup(in, slash-in);
					if ((p = getpwnam(user))) {
						homedir = p->pw_dir;
						in = slash+1;
					} else
						homedir = "";
					xfree(user);
				}
				--in;
			}
		}

		if (!homedir || *homedir != '/') {
			if (!(getcwd(out, sizeof(out)-xstrlen(homedir)-xstrlen(in)-2)))
				return NULL;
			if (*out != '/') {
				debug_error("prepare_path_user(): what the holy shmoly? getcwd() didn't return absolute path! (windows?)\n");
				return NULL;
			}
			xstrcat(out, "/");
		} else
			*out = 0;
		if (homedir && strlcat(out, homedir, sizeof(out)-xstrlen(out)-1) >= sizeof(out)-xstrlen(out)-1)
			return NULL; /* we don't add slash here, 'cause in already has it */
		if (strlcat(out, in, sizeof(out)-xstrlen(out)) >= sizeof(out)-xstrlen(out))
			return NULL;
	}

	{
		char *p;

		while ((p = xstrstr(out, "//"))) /* remove double slashes */
			memmove(p, p+1, xstrlen(p+1)+1);
		while ((p = xstrstr(out, "/./"))) /* single dots suck too */
			memmove(p, p+2, xstrlen(p+2)+1);
		while ((p = xstrstr(out, "/../"))) { /* and finally, '..' */
			char *prev;

			*p = 0;
			if (!(prev = xstrrchr(out, '/')))
				prev = p;
			memmove(prev, p+3, xstrlen(p+3)+1);
		}

				/* clean out end of path */
		p = out+xstrlen(out)-1;
		if (*p == '.') {
			if (*(p-1) == '/') /* '.' */
				*(p--) = 0;
			else if (*(p-1) == '.' && *(p-2) == '/') { /* '..' */
				char *q;

				p -= 2;
				*p = 0;
				if ((q = xstrrchr(out, '/')))
					*(q+1) = 0;
				else {
					*p = '/';
					*(p+1) = 0;
				}
			}
		}
		if (*p == '/' && out != p)
			*p = 0;
	}
#endif

	return out;
}

/**
 * random_line()
 *
 * Open file specified by @a path and select by random one line from file specified by @a path
 *
 * @param	path - path to file.
 *
 * @sa read_file() - if you want read next line from file.
 *
 * @return	NULL - if file was not found or file has no line inside. <br>
 *		else random line founded at file,
 */

static char *random_line(const char *path) {
	int max = 0, item, tmp = 0;
	char *line;
	FILE *f;

	if (!path)
		return NULL;

	if ((f = fopen(path, "r")) == NULL)
		return NULL;
	
	while ((line = read_file(f, 0)))
		max++;

	if (max) {
		rewind(f);
		item = rand() / (RAND_MAX / max + 1);

		while ((line = read_file(f, (tmp == item)))) {	/* read_file(f, 0) or read_file(f, 1) if this is that line */
			if (tmp == item) {
				fclose(f);
				return line;
			}
			tmp++;
		}
	}
		
	fclose(f);
	return NULL;
}

/**
 * read_file()
 *
 * Read next line from file @a f, if needed alloc memory for it.<br>
 * Remove \\r and \\n chars from end of line if needed.
 *
 * @param f	- opened FILE *
 * @param alloc 
 *		- If  0 than it return internal read_file() either xrealloc()'ed or static char with sizeof()==1024,
 *			which you <b>MUST NOT</b> xfree()<br>
 *		- If  1 than it return strdup()'ed string this <b>MUST</b> xfree()<br>
 *		- If -1 than it free <i>internal</i> pointer which were used by xrealloc()
 *
 * @return Line without \\r and \\n which must or mustn't be xfree()'d. It depends on @a alloc param
 */

char *read_file(FILE *f, int alloc) {
	static char buf[1024];
	static char *reres = NULL;

	char *res = NULL;

	size_t reslen = 0;

	int isnewline = 0;

	if (alloc == -1) {
		xfree(reres);
		reres = NULL;
		return NULL;
	}

	if (!f)
		return NULL;

	while (fgets(buf, sizeof(buf), f)) {
		size_t new_size = reslen + xstrlen(buf);

		if (xstrchr(buf, '\n')) {
			isnewline = 1;
			if (!reslen) {
				res = buf;
				reslen = new_size;
				break;
			}
		}

		res = reres = xrealloc(reres, new_size+1);
		xstrcpy(res + reslen, buf);

		reslen = new_size;
		if (isnewline)
			break;
	}

	if (reslen > 0 && res[reslen - 1] == '\n') {
		res[reslen - 1] = 0;
		reslen--;
	}

	if (reslen > 0 && res[reslen - 1] == '\r') {
		res[reslen - 1] = 0;
/*		reslen--;	*/
	}

	return (alloc) ? xstrdup(res) : res;
}

char *read_file_iso(FILE *f, int alloc) {
	static char *tmp = NULL;
	char *buf = read_file(f, 0);
	char *res;

	xfree(tmp);
	tmp = NULL;
	if (alloc == -1)
		return NULL;

	ekg_recode_iso2_inc();
	res = ekg_iso2_to_locale_dup(buf);
	if (!alloc)
		tmp = res;

	ekg_recode_iso2_dec();
	return res;
}

/**
 * timestamp()
 *
 * It returns <b>static</b> buffer with formated current time.
 *
 * @param format - format to pass to strftime() [man 3 strftime]
 *
 * @return	if format is NULL or format == '\\0' than it return ""<br>
 *		else it returns strftime()'d value, or "TOOLONG" if @a buf (sizeof(@a buf) == 100) was too small..
 */

const char *timestamp(const char *format) {
	static char buf[100];
	time_t t;
	struct tm *tm;

	if (!format || format[0] == '\0')
		return "";

	t = time(NULL);
	tm = localtime(&t);
	if (!strftime(buf, sizeof(buf), format, tm))
		return "TOOLONG";
	return buf;
}

const char *timestamp_time(const char *format, time_t t) {
	struct tm *tm;
	static char buf[100];

	if (!format || format[0] == '\0')
		return itoa(t);

	tm = localtime(&t);

	if (!strftime(buf, sizeof(buf), format, tm))
		return "TOOLONG";
	return buf;
}

struct timer *timer_add_ms(plugin_t *plugin, const char *name, unsigned int period, int persist, int (*function)(int, void *), void *data) {
	struct timer *t;
	struct timeval tv;

	/* wylosuj now� nazw�, je�li nie mamy */
	if (!name) {
		int i;

		for (i = 1; !name; i++) {
			int gotit = 0;

			for (t = timers; t; t = t->next) {
				if (!xstrcmp(t->name, itoa(i))) {
					gotit = 1;
					break;
				}
			}

			if (!gotit)
				name = itoa(i);
		}
	}

	t = xmalloc(sizeof(struct timer));
	gettimeofday(&tv, NULL);
	tv.tv_sec += (period / 1000);
	tv.tv_usec += ((period % 1000) * 1000);
	if (tv.tv_usec >= 1000000) {
		tv.tv_usec -= 1000000;
		tv.tv_sec++;
	}
	memcpy(&(t->ends), &tv, sizeof(tv));
	t->name = xstrdup(name);
	t->period = period;
	t->persist = persist;
	t->function = function;
	t->data = data;
	t->plugin = plugin;

	timers_add(t);
	return t;
}

/*
 * timer_add()
 *
 * dodaje timera.
 *
 *  - plugin - plugin obs�uguj�cy timer,
 *  - name - nazwa timera w celach identyfikacji. je�li jest r�wna NULL,
 *	     zostanie przyznany pierwszy numerek z brzegu.
 *  - period - za jaki czas w sekundach ma by� uruchomiony,
 *  - persist - czy sta�y timer,
 *  - function - funkcja do wywo�ania po up�yni�ciu czasu,
 *  - data - dane przekazywane do funkcji.
 *
 * zwraca zaalokowan� struct timer lub NULL w przypadku b��du.
 */
struct timer *timer_add(plugin_t *plugin, const char *name, unsigned int period, int persist, int (*function)(int, void *), void *data)
{
	return timer_add_ms(plugin, name, period * 1000, persist, function, data);
}

struct timer *timer_add_session(session_t *session, const char *name, unsigned int period, int persist, int (*function)(int, session_t *)) {
	struct timer *t;

	if (!session || !session->plugin) {
		debug_error("timer_add_session() s: 0x%x s->plugin: 0x%x\n", session, session ? session->plugin : NULL);
		return NULL;
	}

	t = timer_add(session->plugin, name, period, persist, (void *) function, session);
	t->is_session = 1;
	return t;
}

/*
 * timer_remove()
 *
 * usuwa timer.
 *
 *  - plugin - plugin obs�uguj�cy timer,
 *  - name - nazwa timera,
 *
 * 0/-1
 */
int timer_remove(plugin_t *plugin, const char *name)
{
	struct timer *t;
	int removed = 0;

	for (t = timers; t; t = t->next) {
		if (t->plugin == plugin && !xstrcasecmp(name, t->name)) {
			t = timers_removei(t);
			removed++;
		}
	}

	return ((removed) ? 0 : -1);
}

struct timer *timer_find_session(session_t *session, const char *name) {
	struct timer *t;

	if (!session)
		return NULL;
	
	for (t = timers; t; t = t->next) {
		if (t->is_session && t->data == session && !xstrcmp(name, t->name))
			return t;
	}

	return NULL;
}

int timer_remove_session(session_t *session, const char *name)
{
	struct timer *t;
	plugin_t *p;
	int removed = 0;

	if (!session || (!(p = session->plugin)))
		return -1;

	for (t = timers; t; t = t->next) {
		if (t->is_session && t->data == session && !xstrcmp(name, t->name)) {
			t = timers_removei(t);
			removed++;
		}
	}

	return ((removed) ? 0 : -1);
}

/*
 * timer_handle_command()
 *
 * obs�uga timera wywo�uj�cego komend�.
 */
TIMER(timer_handle_command)
{
	if (type) {
		xfree(data);
		return 0;
	}
	
	command_exec(NULL, NULL, (char *) data, 0);
	return 0;
}

/*
 * timer_remove_user()
 *
 * usuwa wszystkie timery u�ytkownika.
 *
 * 0/-1
 */
int timer_remove_user(int at)
{
	struct timer *t;
	int removed = 0;

	for (t = timers; t; t = t->next) {
		if (t->at == at && t->function == timer_handle_command) { 
			t = timers_removei(t);
			removed = 1;
		}
	}

	return ((removed) ? 0 : -1);
}

/* 
 * xstrmid()
 *
 * wycina fragment tekstu alokuj�c dla niego pami��.
 *
 *  - str - tekst �r�d�owy,
 *  - start - pierwszy znak,
 *  - length - d�ugo�� wycinanego tekstu, je�li -1 do ko�ca.
 */
char *xstrmid(const char *str, int start, int length)
{
	char *res, *q;
	const char *p;

	if (!str)
		return xstrdup("");

	if (start > xstrlen(str))
		start = xstrlen(str);

	if (length == -1)
		length = xstrlen(str) - start;

	if (length < 1)
		return xstrdup("");

	if (length > xstrlen(str) - start)
		length = xstrlen(str) - start;
	
	res = xmalloc(length + 1);
	
	for (p = str + start, q = res; length; p++, q++, length--)
		*q = *p;

	*q = 0;

	return res;
}

struct color_map color_map_default[26] = {
	{ 'k', 0, 0, 0 },
	{ 'r', 168, 0, 0, },
	{ 'g', 0, 168, 0, },
	{ 'y', 168, 168, 0, },
	{ 'b', 0, 0, 168, },
	{ 'm', 168, 0, 168, },
	{ 'c', 0, 168, 168, },
	{ 'w', 168, 168, 168, },
	{ 'K', 96, 96, 96 },
	{ 'R', 255, 0, 0, },
	{ 'G', 0, 255, 0, },
	{ 'Y', 255, 255, 0, },
	{ 'B', 0, 0, 255, },
	{ 'M', 255, 0, 255, },
	{ 'C', 0, 255, 255, },
	{ 'W', 255, 255, 255, },

	/* dodatkowe mapowanie r��nych kolor�w istniej�cych w GG */
	{ 'C', 128, 255, 255, },
	{ 'G', 128, 255, 128, },
	{ 'M', 255, 128, 255, },
	{ 'B', 128, 128, 255, },
	{ 'R', 255, 128, 128, },
	{ 'Y', 255, 255, 128, }, 
	{ 'm', 168, 128, 168, },
	{ 'c', 128, 168, 168, },
	{ 'g', 64, 168, 64, },
	{ 'm', 128, 64, 128, }
};

/*
 * color_map()
 *
 * funkcja zwracaj�ca kod koloru z domy�lnej 16-kolorowej palety terminali
 * ansi odpadaj�cemu podanym warto�ciom RGB.
 */
char color_map(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long mindist = 255 * 255 * 255;
	struct color_map *map = color_map_default;
	char ch = 0;
	int i;

/*	debug("color=%.2x%.2x%.2x\n", r, g, b); */

#define __sq(x) ((x)*(x))
	for (i = 0; i < 26; i++) {
		unsigned long dist = __sq(r - map[i].r) + __sq(g - map[i].g) + __sq(b - map[i].b);

/*		debug("%d(%c)=%.2x%.2x%.2x, dist=%ld\n", i, map[i].color, map[i].r, map[i].g, map[i].b, dist); */

		if (dist < mindist) {
			ch = map[i].color;
			mindist = dist;
		}
	}
#undef __sq

/*	debug("mindist=%ld, color=%c\n", mindist, ch); */

	return ch;	
}

/*
 * sprawdza czy podany znak jest znakiem alphanumerycznym (uwzlglednia polskie znaki)
 */
int isalpha_pl(unsigned char c)
{
/*  gg_debug(GG_DEBUG_MISC, "c: %d\n", c); */
    if(isalpha(c)) /* normalne znaki */
	return 1;
    else if(c == 177 || c == 230 || c == 234 || c == 179 || c == 241 || c == 243 || c == 182 || c == 191 || c == 188) /* polskie literki */
	return 1;
    else if(c == 161 || c == 198 || c == 202 || c == 209 || c == 163 || c == 211 || c == 166 || c == 175 || c == 172) /* wielka litery polskie */
	return 1;
    else
	return 0;
}

/*
 * strcasestr()
 *
 * robi to samo co xstrstr() tyle �e bez zwracania uwagi na wielko��
 * znak�w.
 */
char *strcasestr(const char *haystack, const char *needle)
{
	int i, hlen = xstrlen(haystack), nlen = xstrlen(needle);

	for (i = 0; i <= hlen - nlen; i++) {
		if (!xstrncasecmp(haystack + i, needle, nlen))
			return (char*) (haystack + i);
	}

	return NULL;
}

/*
 * msg_all()
 *
 * msg to all users in session's userlist
 * it uses function to do it
 */
int msg_all(session_t *s, const char *function, const char *what)
{
	userlist_t *ul;

	if (!s->userlist)
		return -1;

	if (!function)
		return -2;

	for (ul = s->userlist; ul; ul = ul->next) {
		userlist_t *u = ul;

		if (!u || !u->uid)
			continue;
		/* XXX, when adding to userlist if we check if uid is good, this code will be ok. */

		command_exec_format(NULL, s, 0, "%s \"%s\" %s", function, get_nickname(s, u->uid), what);
	}

	return 0;
}
/*
 * say_it()
 *
 * zajmuje si� wypowiadaniem tekstu, uwa�aj�c na ju� dzia�aj�cy
 * syntezator w tle.
 *
 * 0/-1/-2. -2 w przypadku, gdy dodano do bufora.
 */
int say_it(const char *str)
{
#ifndef NO_POSIX_SYSTEM
	pid_t pid;

	if (!config_speech_app || !str || !xstrcmp(str, ("")))
		return -1;

	if (speech_pid) {
		buffer_add(&buffer_speech, NULL, str);
		return -2;
	}

	if ((pid = fork()) < 0)
		return -1;

	speech_pid = pid;

	if (!pid) {
		char *tmp = saprintf("%s 2>/dev/null 1>&2", config_speech_app);
		FILE *f = popen(tmp, "w");
		int status = -1;

		xfree(tmp);

		if (f) {
			fprintf(f, "%s.", str);
			status = pclose(f);	/* dzieciak czeka na dzieciaka */
		}

		exit(status);
	}

	child_add(NULL, pid, NULL, NULL, NULL);
	return 0;
#else
	return -1;
#endif
}

#ifndef DISABLE_DEBUG
void debug_ext(debug_level_t level, const char *format, ...) {
	va_list ap;
	if (!config_debug) return;

	va_start(ap, format);
	ekg_debug_handler(level, format, ap);
	va_end(ap);
}

/*
 * debug()
 *
 * debugowanie dla ekg.
 */
void debug(const char *format, ...)
{
	va_list ap;

	if (!config_debug)
		return;

	va_start(ap, format);
	ekg_debug_handler(0, format, ap);
	va_end(ap);
}
#endif

static char base64_charset[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * base64_encode()
 *
 * zapisuje ci�g znak�w w base64.
 *
 *  - buf - ci�g znak�w.
 *
 * zaalokowany bufor.
 */
char *base64_encode(const char *buf, size_t len)
{
	char *out, *res;
	int i = 0, j = 0, k = 0;

	if (!buf) return NULL;
/*	if (!len) return NULL; */
	
	res = out = xmalloc((len / 3 + 1) * 4 + 2);

	while (j < len) {
		switch (i % 4) {
			case 0:
				k = (buf[j] & 252) >> 2;
				break;
			case 1:
				if (j+1 < len)
					k = ((buf[j] & 3) << 4) | ((buf[j + 1] & 240) >> 4);
				else
					k = (buf[j] & 3) << 4;

				j++;
				break;
			case 2:
				if (j+1 < len)
					k = ((buf[j] & 15) << 2) | ((buf[j + 1] & 192) >> 6);
				else
					k = (buf[j] & 15) << 2;

				j++;
				break;
			case 3:
				k = buf[j++] & 63;
				break;
		}
		*out++ = base64_charset[k];
		i++;
	}

	if (i % 4)
		for (j = 0; j < 4 - (i % 4); j++, out++)
			*out = '=';
	
	*out = 0;
	
	return res;
}

/*
 * base64_decode()
 *
 * dekoduje ci�g znak�w z base64.
 *
 *  - buf - ci�g znak�w.
 *
 * zaalokowany bufor.
 */
char *base64_decode(const char *buf)
{
	char *res, *save, *foo, val;
	const char *end;
	int index = 0;
	size_t buflen;

	if (!buf || !(buflen = xstrlen(buf)))
		return NULL;

	save = res = xcalloc(1, (buflen / 4 + 1) * 3 + 2);

	end = buf + buflen - 1;

	while (*buf && buf < end) {
		if (*buf == '\r' || *buf == '\n') {
			buf++;
			continue;
		}
		if (!(foo = xstrchr(base64_charset, *buf)))
			foo = base64_charset;
		val = (int)(foo - base64_charset);
		buf++;
		switch (index) {
			case 0:
				*res |= val << 2;
				break;
			case 1:
				*res++ |= val >> 4;
				*res |= val << 4;
				break;
			case 2:
				*res++ |= val >> 2;
				*res |= val << 6;
				break;
			case 3:
				*res++ |= val;
				break;
		}
		index++;
		index %= 4;
	}
	*res = 0;
	
	return save;
}

/*
 * split_line()
 * 
 * podaje kolejn� lini� z bufora tekstowego. niszczy go bezpowrotnie, dziel�c
 * na kolejne stringi. zdarza si�, nie ma potrzeby pisania funkcji dubluj�cej
 * bufor �eby tylko mie� nieruszone dane wej�ciowe, skoro i tak nie b�d� nam
 * po�niej potrzebne. obcina `\r\n'.
 * 
 *  - ptr - wska�nik do zmiennej, kt�ra przechowuje aktualn� pozycj�
 *    w przemiatanym buforze
 * 
 * wska�nik do kolejnej linii tekstu lub NULL, je�li to ju� koniec bufora.
 */
char *split_line(char **ptr)
{
	char *foo, *res;

	if (!ptr || !*ptr || !xstrcmp(*ptr, ""))
		return NULL;

	res = *ptr;

	if (!(foo = xstrchr(*ptr, '\n')))
		*ptr += xstrlen(*ptr);
	else {
		size_t reslen;
		*ptr = foo + 1;
		*foo = 0;

		reslen = xstrlen(res);
		if (reslen > 1 && res[reslen - 1] == '\r')
			res[reslen - 1] = 0;
	}

	return res;
}

/*
 * ekg_status_label()
 *
 * tworzy etykiet� formatki opisuj�cej stan.
 */
const char *ekg_status_label(const int status, const char *descr, const char *prefix)
{
	static char buf[100]; /* maybe dynamic buffer would be better? */
	const char *status_string = ekg_status_string(status, 0);
	
	snprintf(buf, sizeof(buf), "%s%s%s", (prefix) ? prefix : "", status_string, (descr) ? "_descr" : "");

	return buf;
}

/*
 * ekg_draw_descr()
 *
 * losuje opis dla danego stanu lub pobiera ze zmiennej, lub cokolwiek
 * innego.
 */
char *ekg_draw_descr(const int status)
{
	const char *value;
	char file[100];
	char var[100];
	variable_t *v;	

	if (EKG_STATUS_IS_NA(status)) { /* or maybe == NA ? */
		xstrcpy(var, ("quit_reason"));
		xstrcpy(file, "quit.reasons");
	} else if (status == EKG_STATUS_AVAIL) {
		xstrcpy(var, ("back_reason"));
		xstrcpy(file, "back.reasons");
	} else {
		/* Wouldn't it be better to use command-names? */
		snprintf(var, sizeof(var), "%s_reason", ekg_status_string(status, 0));
		snprintf(file, sizeof(file), "%s.reasons", ekg_status_string(status, 0));
	}

	if (!(v = variable_find(var)) || v->type != VAR_STR)
		return NULL;

	value = *(char**)(v->ptr);

	if (!value)
		return NULL;

	if (!xstrcmp(value, "*"))
		return random_line(prepare_path(file, 0));

	return xstrdup(value);
}

/* 
 * ekg_update_status()
 *
 * updates our status, if we are on session contact list 
 * 
 */
void ekg_update_status(session_t *session)
{
	userlist_t *u;

	if ((u = userlist_find(session, session->uid))) {
		xfree(u->descr);
		u->descr = xstrdup(session->descr);

		if (!session_connected_get(session))
			u->status = EKG_STATUS_NA;
		else
			u->status = session->status;

		u->blink = 0;

		{
			const char *__session	= session_uid_get(session);
			const char *__uid		= u->uid;

			query_emit_id(NULL, USERLIST_CHANGED, &__session, &__uid);
		}
	}
}

/* status string tables */

struct ekg_status_info {
	status_t		status;		/* enumed status */
	const char*		label;		/* name used in formats */
	const char*		command;	/* command used to set status, if ==format, NULL */
};

/* please, keep it sorted with status_t */
const struct ekg_status_info ekg_statuses[] = {
		{ EKG_STATUS_VISITOR,  "visitor"	},
		{ EKG_STATUS_NONE,     "ne"	        },
		{ EKG_STATUS_MEMBER,   "member"		},
		{ EKG_STATUS_ADMIN,    "admin"		},
		{ EKG_STATUS_OWNER,    "owner"		},
		{ EKG_STATUS_ERROR,    "error"		},
		{ EKG_STATUS_BLOCKED,  "blocking"	},
		{ EKG_STATUS_UNKNOWN,  "unknown"	},
		{ EKG_STATUS_NA,       "notavail"	},
		{ EKG_STATUS_INVISIBLE,"invisible"	},
		{ EKG_STATUS_DND,      "dnd"		},
		{ EKG_STATUS_GONE,     "gone"		},
		{ EKG_STATUS_XA,       "xa"		},
		{ EKG_STATUS_AWAY,     "away"		},
		{ EKG_STATUS_AVAIL,    "avail", "back", },
		{ EKG_STATUS_FFC,      "chat",  "ffc"	},

				/* here go the special statuses */
		{ EKG_STATUS_AUTOAWAY,  "autoaway"	},
		{ EKG_STATUS_AUTOXA,    "autoxa"	},
		{ EKG_STATUS_AUTOBACK,  "autoback"	},
		{ EKG_STATUS_NULL			}
};

static inline const struct ekg_status_info *status_find(const int status) {
	const struct ekg_status_info *s;

		/* as long as ekg_statuses[] are sorted, this should be fast */
	if (status < EKG_STATUS_LAST) {
		for (s = &(ekg_statuses[status-1]); s->status != EKG_STATUS_NULL; s++) {
			if (s->status == status)
				return s;
		}
	}

	debug_function("status_find(), going into fallback loop (statuses ain't sorted?)\n");

		/* fallback if statuses aren't sorted, we've got unknown or special status,
		 * in second case we'll iterate part of list twice, but that's rather
		 * rare case, so I think optimization isn't needed here. */
	for (s = ekg_statuses; s->status != EKG_STATUS_NULL; s++) {
		if (s->status == status)
			return s;
	}

	return NULL;
}

/*
 * ekg_status_string()
 *
 * converts enum status to string
 * cmd = 0 for normal labels, 1 for command names, 2 for labels+special (esp. for debug, use wisely)
 */

const char *ekg_status_string(const int status, const int cmd)
{
	const char *r = NULL;

	if ((status > 0) && (status < (cmd == 2 ? 0x100 : 0x80))) {
		const struct ekg_status_info *s = status_find(status);

		if (s) {
			if (cmd == 1)
				r = s->command;		/* if command differs from status */
			if (!r)
				r = s->label;		/* else fetch status */
		}
	}

	if (!r) {
		const struct ekg_status_info *s = status_find(cmd == 1 ? EKG_STATUS_AVAIL : EKG_STATUS_UNKNOWN);

		/* we only allow 00..7F, or 00..FF with cmd==2
		 * else we return either UNKNOWN or AVAIL if cmd==1 */
		debug_error("ekg_status_string(): called with unexpected status: 0x%02x\n", status);
		if (!s) {
			debug_error("ekg_status_string(): critical error, status_find with predef value failed!\n");
			return NULL;
		}
		return (cmd == 1 ? s->command : s->label);
	}
	
	return r;
}

/*
 * ekg_status_int()
 *
 * converts string to enum status
 */

int ekg_status_int(const char *text)
{
	const struct ekg_status_info *s;

	for (s = ekg_statuses; s->status != EKG_STATUS_NULL; s++) {
		if (!xstrcasecmp(text, s->label) || !xstrcasecmp(text, s->command))
			return s->status;
	}

	debug_error("ekg_status_int(): Got unexpected status: %s\n", text);
	return EKG_STATUS_NULL;
}

/*
 * ekg_sent_message_format()
 *
 * funkcja pomocnicza dla protoko��w obs�uguj�cych kolorki. z podanego
 * tekstu wycina kolorki i zwraca informacje o formatowaniu tekstu bez
 * kolork�w.
 */
uint32_t *ekg_sent_message_format(const char *text)
{
	uint32_t *format, attr;
	char *newtext, *q;
	const char *p, *end;
	int len;

	/* je�li nie stwierdzono znak�w kontrolnych, spadamy */
/*
	if (!xstrpbrk(text, "\x02\x03\x12\x14\x1f"))
		return NULL;
 */

	/* oblicz d�ugo�� tekstu bez znaczk�w formatuj�cych */
	for (p = text, len = 0; *p; p++) {
		if (!xstrchr(("\x02\x03\x12\x14\x1f"), *p))
			len++;
	}

	if (len == xstrlen(text))
		return NULL;
	
	newtext = xmalloc(len + 1);
	format = xmalloc(len * 4);

	end = text + xstrlen(text);

	for (p = text, q = newtext, attr = 0; p < end; ) {
		int j;
			
		if (*p == 18 || *p == 3) {	/* Ctrl-R, Ctrl-C */
			p++;

			if (xisdigit(*p)) {
				int num = atoi(p);
				
				if (num < 0 || num > 15)
					num = 0;

				p++;

				if (xisdigit(*p))
					p++;

				attr &= ~EKG_FORMAT_RGB_MASK;
				attr |= EKG_FORMAT_COLOR;
				attr |= color_map_default[num].r;
				attr |= color_map_default[num].g << 8;
				attr |= color_map_default[num].b << 16;
			} else
				attr &= ~EKG_FORMAT_COLOR;

			continue;
		}

		if (*p == 2) {		/* Ctrl-B */
			attr ^= EKG_FORMAT_BOLD;
			p++;
			continue;
		}

		if (*p == 20) {		/* Ctrl-T */
			attr ^= EKG_FORMAT_ITALIC;
			p++;
			continue;
		}

		if (*p == 31) {		/* Ctrl-_ */
			attr ^= EKG_FORMAT_UNDERLINE;
			p++;
			continue;
		}

		/* zwyk�y znak */
		*q = *p;
		for (j = (int) (q - newtext); j < len; j++)
			format[j] = attr;
		q++;
		p++;
	}

	return format;
}

size_t strlen_pl(const char *s) {
	int ok = 1, len=xstrlen(s), count = 0;
	const char *end = s+len;

	while ((s < end) && (ok > 0)) {
		if ((ok = mbrtowc(NULL, s, len, NULL)) > 0) {
			count++;
			s += ok;
		}
	}
	return count;
}

static int tolower_pl(const unsigned char c);

char *xstrncat_pl(char *dest, const char *src, size_t n) {
#if USE_UNICODE
	int i, v, len=xstrlen(src);
	char *p = (char *)src;
	
	for (i = 0; i < n && *p; i++) {
		if ((v = mbrtowc(NULL, p, len, NULL)) > 0) {
			p += v;
		}
	}
	n = p - src;
#endif
	return xstrncat(dest, src, n);
}

/*
 * strncasecmp_pl()
 *
 * por�wnuje dwa ci�gi o okre�lonej przez n d�ugo�ci
 * dzia�a analogicznie do xstrncasecmp()
 * obs�uguje polskie znaki
 */
int strncasecmp_pl(const char *cs, const char *ct , size_t count)
{
	register signed char __res = 0;
#if USE_UNICODE
	wchar_t *wc1 = xcalloc(count+1, sizeof(wchar_t));
	wchar_t *wc2 = xcalloc(count+1, sizeof(wchar_t));
	wchar_t *p;

	mbsrtowcs(wc1, &cs, count, NULL);
	mbsrtowcs(wc2, &ct, count, NULL);

	for(p=wc1; *p; p++) *p = towlower(*p);
	for(p=wc2; *p; p++) *p = towlower(*p);

	__res = wcscoll(wc1, wc2);

	xfree(wc1);
	xfree(wc2);
#else
	while (count) {
		if ((__res = tolower_pl(*cs) - tolower_pl(*ct++)) != 0 || !*cs++)
			break;
		count--;
	}
#endif
	return __res;
}

int strcasecmp_pl(const char *cs, const char *ct)
{
	register signed char __res = 0;

	while ((__res = tolower_pl(*cs) - tolower_pl(*ct++)) == 0 && !*cs++) {
		if (!*cs++)
			return(0);
	}

	return __res;
}

/*
 * tolower_pl()
 *
 * zamienia podany znak na ma�y je�li to mo�liwe
 * obs�uguje polskie znaki
 */
static int tolower_pl(const unsigned char c) {
	switch(c) {
		case 161: /* � */
			return 177;
		case 198: /* � */
			return 230;
		case 202: /* � */
			return 234;
		case 163: /* � */
			return 179;
		case 209: /* � */
			return 241;
		case 211: /* � */
			return 243;
		case 166: /* � */
			return 182;
		case 175: /* � */
			return 191;
		case 172: /* � */
			return 188;
		default: /* reszta */
			return tolower(c);
	}
}

/*
 * saprintf()
 *
 * dzia�a jak sprintf() tylko, �e wyrzuca wska�nik
 * do powsta�ego ci�gu
 */
char *saprintf(const char *format, ...)
{
	va_list ap;
	char *res;

	va_start(ap, format);
	res = vsaprintf(format, ap);
	va_end(ap);

	return res;
}

/*
 * xstrtr()
 *
 * zamienia wszystko znaki a na b w podanym ci�gu
 * nie robie jego kopi!
 */
void xstrtr(char *text, char from, char to)
{
	
	if (!text || !from)
		return;

	while (*text++) {
		if (*text == from) {
			*text = to;
		}
	}
}

/**
 * ekg_yield_cpu()
 *
 * releases cpu
 * meant to be called while busy-looping
 */

inline void ekg_yield_cpu()
{
#ifdef _POSIX_PRIORITY_SCHEDULING
	sched_yield();
#endif
}

/**
 * ekg_write()
 *
 * write data to given fd, if it cannot be done [because system buffer is too small. it'll create watch, and write as soon as possible]
 * XXX, for now it'll always create watch.
 * (You can be notified about state of buffer when you call ekg_write(fd, NULL, -1))
 *
 * @note
 *	This _should_ be used as replacement for write() 
 */

int ekg_write(int fd, const char *buf, int len) {
	watch_t *wl = NULL;
	list_t l;

	if (fd == -1)
		return -1;

	/* first check if we have watch for this fd */
	for (l = watches; l; l = l->next) {
		watch_t *w = l->data;

		if (w && w->fd == fd && w->type == WATCH_WRITE && w->buf) {
			wl = w;
			break;
		}
	}

	if (wl) {
		if (!buf && len == -1) /* smells stupid, but do it */
			return wl->buf->len;
	} else {
		/* if we have no watch, let's create it. */	/* XXX, first try write() ? */
		wl = watch_add(NULL, fd, WATCH_WRITE_LINE, NULL, NULL);
	}

	return watch_write_data(wl, buf, len);
}

int ekg_writef(int fd, const char *format, ...) {
	char		*text;
	int		textlen;
	va_list		ap;
	int		res;

	if (fd == -1 || !format)
		return -1;

	va_start(ap, format);
	text = vsaprintf(format, ap);
	va_end(ap);
	
	textlen = xstrlen(text); 

	debug_io("ekg_writef: %s\n", text ? textlen ? text: "[0LENGTH]":"[FAILED]");

	if (!text) 
		return -1;

	res = ekg_write(fd, text, textlen);

	xfree(text);
	return res;
}

/**
 * ekg_close()
 *
 * close fd and all watches associated with that fd
 *
 * @note
 *	This _should_ be used as replacement for close() (especially in protocol plugins)
 */

int ekg_close(int fd) {
	list_t l;

	if (fd == -1)
		return -1;

	for (l = watches; l; l = l->next) {
		watch_t *w = l->data;

		if (w && w->fd == fd) {
			debug("ekg_close(%d) w->plugin: %s w->session: %s w->type: %d w->buf: %d\n", 
				fd, w->plugin ? w->plugin->name : "-",
				w->is_session ? ((session_t *) w->data)->uid : "-",
				w->type, !!w->buf);

			watch_free(w);
		}
	}
	return close(fd);
}

/**
 * password_input()
 *
 * Try to get password through UI_PASSWORD_INPUT, printing error messages if needed.
 *
 * @return	Pointer to new password (which needs to be freed) or NULL, if not
 *		succeeded (wrong input / no support).
 */

char *password_input(const char *prompt, const char *rprompt, const bool norepeat) {
	char *pass = NULL;

	if (query_emit_id(NULL, UI_PASSWORD_INPUT, &pass, &prompt, norepeat ? NULL : &rprompt) == -2) {
		print("password_nosupport");
		return NULL;
	}

	return pass;
}

int is_utf8_string(const char *txt) {
	const char *p;
	int mask, n;

	if (!txt) return 0;

	for (p = txt; *p; p++) {
		//	0xxxxxxx	continue
		//	10xxxxxx 	n=0; return 0
		//	110xxxxx	n=1
		//	1110xxxx	n=2
		//	11110xxx	n=3
		//	111110xx	n=4
		//	1111110x	n=5
		//	1111111x	n>5; return 0

		if (!(*p & 0x80)) continue;

		for (n = 0, mask = 0x40; (*p & mask); n++, mask >>= 1);

		if (!n || (n>5)) return 0;

		for (; n; n--)
			if ((*++p & 0xc0) != 0x80) return 0;
	}

	return 1;
}

/*
 * Local Variables:
 * mode: c
 * c-file-style: "k&r"
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 * vim: noet
 */
