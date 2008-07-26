#include <gtk/gtk.h>

#include "main.h"
#include "kvm.h"
#include "rack.h"

struct _VRackKvmSwitch {
	VRackItem *rackitem;

	gint32 selected_port;
	guint32 n_ports;
	VRackKvmSource *source;
	VRackKvmSource **input_sources;
	GtkWidget *widget;
	GtkWidget **w_ports;
};

static void kvm_create_widget(VRackKvmSwitch *kvm);

static GdkPixbuf *kvm_get_pixbuf_cb(gpointer opaque);
static gboolean kvm_get_size_cb(gpointer opaque, guint32 *w, guint32 *h);

VRackKvmSwitch *kvm_switch_new(VRackCtxt *ctxt, guint32 n_ports)
{
	VRackKvmSwitch *kvm;

	kvm = g_new0(VRackKvmSwitch, 1);
	kvm->selected_port = -1;
	kvm->n_ports = n_ports;
	kvm->source = g_new0(VRackKvmSource, 1);
	kvm->source->opaque = kvm;
	kvm->source->get_pixbuf = kvm_get_pixbuf_cb;
	kvm->source->get_size = kvm_get_size_cb;
	kvm->input_sources = g_new0(VRackKvmSource *, n_ports);

	kvm_create_widget(kvm);

	kvm->rackitem = rack_item_new(kvm->widget, 1);

	return kvm;
}

gboolean kvm_switch_attach(VRackKvmSwitch *kvm, VRackKvmSource *src,
	gint32 port)
{
	gint i;

	g_return_val_if_fail(kvm != NULL, FALSE);
	g_return_val_if_fail(src != NULL, FALSE);

	if(port >= 0) {
		if(port >= kvm->n_ports)
			return FALSE;
		if(kvm->input_sources[port] != NULL)
			return FALSE;
		kvm->input_sources[port] = src;
		if(kvm->selected_port == -1)
			kvm->selected_port = port;
		return TRUE;
	}
	for(i = 0; i < kvm->n_ports; i ++)
		if(kvm->input_sources[i] == NULL) {
			kvm->input_sources[i] = src;
			if(kvm->selected_port == -1)
				kvm->selected_port = i;
			return TRUE;
		}

	return FALSE;
}

VRackKvmSource *kvm_switch_get_kvm_source(VRackKvmSwitch *kvm)
{
	return kvm->source;
}

/*****************************************************************************/

static void kvm_toggle_cb(GtkToggleButton *togglebutton, gpointer data)
{
	VRackKvmSwitch *kvm = data;
	gint port;

	if(gtk_toggle_button_get_active(togglebutton) == FALSE)
		return;

	port = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(togglebutton), "port"));
	kvm->selected_port = port;
}

static void kvm_create_widget(VRackKvmSwitch *kvm)
{
	gint i;
	GSList *group = NULL;
	GtkWidget *bbox;
	gchar *snum;

	kvm->widget = gtk_frame_new("KVM switch");
	bbox = gtk_hbutton_box_new();
	gtk_container_add(GTK_CONTAINER(kvm->widget), bbox);

	kvm->w_ports = g_new0(GtkWidget *, kvm->n_ports);
	for(i = 0; i < kvm->n_ports; i ++) {
		snum = g_strdup_printf("%02d", i);
		kvm->w_ports[i] = gtk_radio_button_new_with_label(group, snum);
		g_free(snum);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(kvm->w_ports[i]));
		gtk_box_pack_start(GTK_BOX(bbox), kvm->w_ports[i], FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(kvm->w_ports[i]), "toggled",
			G_CALLBACK(kvm_toggle_cb), kvm);
		g_object_set_data(G_OBJECT(kvm->w_ports[i]), "port",
			GINT_TO_POINTER(i));
	}
}

static GdkPixbuf *kvm_get_pixbuf_cb(gpointer opaque)
{
	VRackKvmSwitch *kvm = opaque;
	VRackKvmSource *source;

	if(kvm->selected_port == -1)
		return NULL;
	source = kvm->input_sources[kvm->selected_port];
	if(source)
		return source->get_pixbuf(source->opaque);
	return NULL;
}

static gboolean kvm_get_size_cb(gpointer opaque, guint32 *w, guint32 *h)
{
	VRackKvmSwitch *kvm = opaque;
	VRackKvmSource *source;

	if(kvm->selected_port == -1)
		return FALSE;
	source = kvm->input_sources[kvm->selected_port];
	if(source)
		return source->get_size(source->opaque, w, h);
	return FALSE;
}

