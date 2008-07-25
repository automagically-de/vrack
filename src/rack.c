#include <gtk/gtk.h>

#include "rack.h"

struct _VRack {
	guint32 n_units;
	VRackItem **units;
	GtkWidget *widget;
	GtkWidget *w_table;
	GtkWidget **w_empty;
};

#define VRACKITEM_MAGIC ('I'<<24|'t'<<16|'e'<<8|'m')

struct _VRackItem {
	guint32 magic;
	VRack *rack;
	gint32 start_unit;
	guint32 n_units;
	GtkWidget *widget;
};

struct _VRackComponent {
	VRackItem *rackitem;
};

static void rack_create_widget(VRack *rack);
static GtkWidget *rack_empty_unit_new(guint32 unit);
static void rack_empty_unit_free(GtkWidget *widget);

VRack *rack_new(VRackCtxt *ctxt, guint32 n_units)
{
	VRack *rack;

	rack = g_new0(VRack, 1);

	rack->n_units = n_units;
	rack->units = g_new0(VRackItem *, n_units);

	rack_create_widget(rack);

	return rack;
}

void rack_free(VRack *rack)
{
	gint i;

	for(i = 0; i < rack->n_units; i ++)
		if(rack->units[i] == NULL)
			rack_empty_unit_free(rack->w_empty[i]);
	g_free(rack->w_empty);
	g_free(rack->units);
	g_free(rack);
}

GtkWidget *rack_get_widget(VRack *rack)
{
	return rack->widget;
}

gint32 rack_find_space(VRack *rack, guint32 n_units)
{
	gint i, j;
	gboolean empty = TRUE;

	for(i = 0; i <= (rack->n_units - n_units); i ++) {
		if(rack->units[i] == NULL) {
			empty = TRUE;
			for(j = 1; j < n_units; j ++) {
				if(rack->units[i + j] != NULL) {
					empty = FALSE;
					break;
				}
			}
			if(empty)
				return i;
		}
	}
	return -1;
}

gboolean rack_attach(VRack *rack, gpointer object, guint32 unit)
{
	VRackItem *rackitem;
	gint i;

	rackitem = rack_item_get(object);
	if(rackitem == NULL)
		return FALSE;

	if((unit + rackitem->n_units) > rack->n_units) {
		g_warning("vrack_attach: unit %u (+ %u) > %u", unit,
			rackitem->n_units, rack->n_units);
		return FALSE;
	}
	for(i = 0; i < rackitem->n_units; i ++) {
		if(rack->units[unit + i] != NULL) {
			g_warning("vrack_attach: rack unit %d already occupied",
				unit + i);
			return FALSE;
		}
	}

	/* finally attach */
	rackitem->rack = rack;
	rackitem->start_unit = unit;
	for(i = 0; i < rackitem->n_units; i ++) {
		/* want to destroy the widget by myself */
		g_object_ref(G_OBJECT(rack->w_empty[unit + i]));
		gtk_container_remove(GTK_CONTAINER(rack->w_table),
			rack->w_empty[unit + i]);
		rack_empty_unit_free(rack->w_empty[unit + i]);
		rack->w_empty[unit + i] = NULL;
		rack->units[unit + i] = rackitem;
	}
	gtk_table_attach_defaults(GTK_TABLE(rack->w_table), rackitem->widget,
		0, 1, rackitem->start_unit, rackitem->start_unit + rackitem->n_units);

	return TRUE;
}

gboolean rack_detach(gpointer object)
{
	VRack *rack;
	VRackItem *rackitem;
	gint i;

	rackitem = rack_item_get(object);
	if(rackitem == NULL)
		return FALSE;

	rack = rackitem->rack;
	if(rack == NULL) {
		g_warning("vrack_detach: component not attached");
		return FALSE;
	}
	if((rackitem->start_unit + rackitem->n_units) > rack->n_units) {
		g_warning("vrack_detach: invalid state: %d + %u > %u",
			rackitem->start_unit, rackitem->n_units, rack->n_units);
		return FALSE;
	}

	/* finally detach */
	gtk_container_remove(GTK_CONTAINER(rack->w_table), rackitem->widget);
	for(i = 0; i < rackitem->n_units; i ++) {
		rack->units[rackitem->start_unit + i] = NULL;
		rack->w_empty[rackitem->start_unit + i] =
			rack_empty_unit_new(rackitem->start_unit + i);
		gtk_table_attach_defaults(GTK_TABLE(rack->w_table),
			rack->w_empty[rackitem->start_unit + i], 0, 1,
			rackitem->start_unit + i, rackitem->start_unit + i + 1);
	}
	rackitem->start_unit = -1;
	rackitem->rack = NULL;

	return TRUE;
}

VRackItem *rack_item_new(GtkWidget *widget, guint32 n_units)
{
	VRackItem *item;

	item = g_new0(VRackItem, 1);
	item->magic = VRACKITEM_MAGIC;
	item->widget = widget;
	item->n_units = n_units;
	item->start_unit = -1;

	return item;
}

void rack_item_free(VRackItem *item)
{
	if(item == NULL)
		return;
	g_free(item);
}

VRackItem *rack_item_get(gpointer object)
{
	struct _VRackComponent *comp = (struct _VRackComponent *)object;

	if((comp == NULL) || (comp->rackitem == NULL) ||
		(comp->rackitem->magic != VRACKITEM_MAGIC)) {
		g_warning("rack_item_get: object is not a rack component");
		return NULL;
	}
	return comp->rackitem;
}

/*****************************************************************************/

static void rack_create_widget(VRack *rack)
{
	gint i;

	rack->w_empty = g_new0(GtkWidget *, rack->n_units);
	rack->widget = gtk_frame_new("Rack");
	rack->w_table = gtk_table_new(rack->n_units, 1, TRUE);
	gtk_container_add(GTK_CONTAINER(rack->widget), rack->w_table);

	for(i = 0; i < rack->n_units; i ++) {
		rack->w_empty[i] = rack_empty_unit_new(i);
		gtk_table_attach_defaults(GTK_TABLE(rack->w_table), rack->w_empty[i],
			0, 1, i, i + 1);
	}
}

static GtkWidget *rack_empty_unit_new(guint32 unit)
{
	GtkWidget *widget;
	gchar *name;

	name = g_strdup_printf("empty unit #%d", unit);
	widget = gtk_frame_new(name);
	g_free(name);
	return widget;
}

static void rack_empty_unit_free(GtkWidget *widget)
{
	gtk_widget_destroy(widget);
}
