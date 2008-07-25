#ifndef _MAIN_H
#define _MAIN_H

#include <gtk/gtk.h>
#include <vncdisplay.h>

typedef struct {
	int *argcp;
	char ***argvp;

	GtkWidget *window;
	VncDisplay *vnc;
	GSList *switches;
	GSList *machines;
	gchar *tmpdir;
} VRackCtxt;

#endif
