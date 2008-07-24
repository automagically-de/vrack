#ifndef _MAIN_H
#define _MAIN_H

#include <gtk/gtk.h>
#include <vncdisplay.h>

typedef struct {
	int *argcp;
	char ***argvp;

	GtkWidget *window;
	GtkWidget *da;
	GdkPixbuf *backbuffer;
	VncDisplay *vnc;
	guint8 button_mask;
	GSList *switches;
	gchar *tmpdir;
} VRackCtxt;

#endif
