/*
 *      Copyright (C) 2014 Jean-Luc Barriere
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "mythdebug.h"

#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <string.h>
#include <ctype.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

typedef struct {
	const char *name;
	int        cur_level;
	int        (*selector)(int plevel, int slevel);
	void       (*msg_callback)(int level, char *msg);
} debug_ctx_t;

#define DEBUG_CTX_INIT(n,l,s) { n, l, s, NULL }

static debug_ctx_t debug_ctx = DEBUG_CTX_INIT("CPPMyth", MYTH_DBG_NONE, NULL);

/**
 * Set the debug level to be used for the subsystem
 * \param ctx the subsystem debug context to use
 * \param level the debug level for the subsystem
 * \return an integer subsystem id used for future interaction
 */
static inline void __dbg_setlevel(debug_ctx_t *ctx, int level)
{
	if (ctx != NULL) {
		ctx->cur_level = level;
	}
}

/**
 * Generate a debug message at a given debug level
 * \param ctx the subsystem debug context to use
 * \param level the debug level of the debug message
 * \param fmt a printf style format string for the message
 * \param ... arguments to the format
 */
static inline void __dbg(debug_ctx_t *ctx, int level, const char *fmt, va_list ap)
{
	char msg[4096];
	int len;
	if (ctx == NULL) {
		return;
	}
	if ((ctx->selector && ctx->selector(level, ctx->cur_level)) ||
	    (!ctx->selector && (level <= ctx->cur_level))) {
		len = snprintf(msg, sizeof(msg), "(%s)", ctx->name);
		vsnprintf(msg + len, sizeof(msg)-len, fmt, ap);
		if (ctx->msg_callback) {
			ctx->msg_callback(level, msg);
		} else {
			fwrite(msg, strlen(msg), 1, stderr);
		}
	}
}

void Myth::DBGLevel(int l)
{
  __dbg_setlevel(&debug_ctx, l);
}

void Myth::DBGAll()
{
  __dbg_setlevel(&debug_ctx, MYTH_DBG_ALL);
}

void Myth::DBGNone()
{
  __dbg_setlevel(&debug_ctx, MYTH_DBG_NONE);
}

void Myth::DBG(int level, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  __dbg(&debug_ctx, level, fmt, ap);
  va_end(ap);
}

void Myth::SetDBGMsgCallback(void (*msgcb)(int level, char *))
{
  debug_ctx.msg_callback = msgcb;
}
