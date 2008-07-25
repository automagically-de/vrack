#ifndef _GUI_H
#define _GUI_H

#include <gtk/gtk.h>

#include "main.h"

gboolean gui_init(VRackCtxt *ctxt);
gboolean gui_create(VRackCtxt *ctxt, GtkWidget *rackwidget);
gboolean gui_run(VRackCtxt *ctxt);

#endif
