#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cmdq.h"

struct _CmdQ {
	GIOChannel *io;
	GSList *queue;
	CmdQEntry *active;
	CmdQPromptFunc promptfunc;
	gpointer user_data;
	guint watch_hup;
	guint watch_in;
	gchar *buffer;
};

struct _CmdQEntry {
	gchar *command;
	CmdQCallback callback;
};

static gboolean cmdq_hup_cb(GIOChannel *source, GIOCondition condition,
	gpointer data)
{
	CmdQ *q = (CmdQ *)data;

	g_return_val_if_fail(q != NULL, FALSE);

	g_debug("CmdQ: HUP received");

	return FALSE; /* remove event source */
}

static gboolean cmdq_send(GIOChannel *io, const gchar *cmd)
{
	GError *error = NULL;
	gsize len = 0;

	g_io_channel_write_chars(io, cmd, strlen(cmd), &len, &error);
	if(error != NULL) {
		g_warning("cmdq: write error: %s", error->message);
		g_error_free(error);
		return FALSE;
	}
	error = NULL;
	g_io_channel_write_chars(io, "\n", 1, &len, &error);
	error = NULL;
	g_io_channel_flush(io, &error);

	return TRUE;
}

static gboolean cmdq_in_cb(GIOChannel *source, GIOCondition condition,
	gpointer data)
{
	CmdQ *q = (CmdQ *)data;
	GError *error = NULL;
	gchar buf[BUFSIZ + 1], *tmp;
	gsize len;

	g_return_val_if_fail(q != NULL, FALSE);

	memset(buf, '\0', BUFSIZ + 1);
	g_io_channel_read_chars(q->io, buf, BUFSIZ, &len, &error);
	if(len > 0) {
		if(q->buffer) {
			tmp = g_strconcat(q->buffer, buf, NULL);
			g_free(q->buffer);
			q->buffer = tmp;
		} else {
			q->buffer = g_strdup(buf);
		}

		if(q->promptfunc && q->promptfunc(q->buffer, q->user_data)) {
			if(q->buffer) {
				if(q->active && q->active->callback) {
					g_debug("cmdq: running callback for %s",
						q->active->command);
					q->active->callback(q->io, q->buffer, q->user_data);
				}
				g_free(q->buffer);
				q->buffer = NULL;
			}

			/* cleanup active command */
			if(q->active) {
				g_free(q->active->command);
				g_free(q->active);
				q->active = NULL;
			}

			/* get next command */
			if(q->queue) {
				q->active = (CmdQEntry *)q->queue->data;
				q->queue = g_slist_remove(q->queue, q->active);

				g_debug("queueing command '%s'", q->active->command);

				/* send command to socket */
				cmdq_send(q->io, q->active->command);
			}
		}
	}

	return TRUE;
}

CmdQ *cmdq_create(GIOChannel *io, CmdQPromptFunc promptfunc,
	gpointer user_data)
{
	CmdQ *q;

	q = g_new0(CmdQ, 1);
	q->io = io;
	q->promptfunc = promptfunc;
	q->user_data = user_data;

	q->watch_hup = g_io_add_watch(io, G_IO_HUP, cmdq_hup_cb, q);
	q->watch_in = g_io_add_watch(io, G_IO_IN, cmdq_in_cb, q);

	return q;
}

gboolean cmdq_add(CmdQ *q, const gchar *cmd, CmdQCallback callback)
{
	CmdQEntry *entry;

	entry = g_new0(CmdQEntry, 1);
	entry->command = g_strdup(cmd);
	entry->callback = callback;

	if(q->active == NULL) {
		q->active = entry;
		cmdq_send(q->io, cmd);
	} else {
		q->queue = g_slist_append(q->queue, entry);
	}

	return TRUE;
}

void cmdq_destroy(CmdQ *q)
{
	g_free(q);
}
