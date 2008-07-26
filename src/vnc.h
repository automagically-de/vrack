#ifndef _VNC_H
#define _VNC_H

#include <gtk/gtk.h>

#include "main.h"
#include "kvm.h"

gboolean vnc_init(VRackCtxt *ctxt);
GtkWidget *vnc_create_widget(VRackCtxt *ctxt);
VRackKvmSource *vnc_get_source(VRackCtxt *ctxt, guint32 token,
	const gchar *host, guint32 port);

#endif
