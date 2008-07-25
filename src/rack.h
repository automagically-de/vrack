#ifndef _RACK_H
#define _RACK_H

#include <gtk/gtk.h>

#include "main.h"

typedef struct _VRack VRack;

typedef struct _VRackItem VRackItem;

VRack *rack_new(VRackCtxt *ctxt, guint32 n_units);
void rack_free(VRack *rack);
GtkWidget *rack_get_widget(VRack *rack);
gint32 rack_find_space(VRack *rack, guint32 n_units);
gboolean rack_attach(VRack *rack, gpointer object, guint32 unit);
gboolean rack_detach(gpointer object);

VRackItem *rack_item_new(GtkWidget *widget, guint32 n_units);
void rack_item_free(VRackItem *item);

/* get rack item data from generic component (switch/machine/...) */
VRackItem *rack_item_get(gpointer object);

#endif
