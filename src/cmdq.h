#ifndef _CMDQ_H
#define _CMDQ_H

#include <glib.h>

typedef struct _CmdQ CmdQ;
typedef struct _CmdQEntry CmdQEntry;

typedef gboolean (* CmdQPromptFunc)(const gchar *text, gpointer user_data);
typedef void (* CmdQCallback)(GIOChannel *io, const gchar *text,
	gpointer user_data);

CmdQ *cmdq_create(GIOChannel *io, CmdQPromptFunc promptfunc,
	gpointer user_data);
gboolean cmdq_add(CmdQ *q, const gchar *cmd, CmdQCallback callback);
void cmdq_destroy(CmdQ *q);

#endif /* _CMDQ_H */
