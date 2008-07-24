#include <gtk/gtk.h>
#include <vncdisplay.h>

#include "main.h"

static gboolean gui_idle_cb(gpointer data)
{
	VRack *rack = (VRack *)data;
	GdkPixbuf *pixbuf;
	guint32 w, h, rowstride;
	guint8 *pixels;
	gdouble scale_x, scale_y;

	g_return_val_if_fail(rack != NULL, FALSE);

	pixbuf = vnc_display_get_pixbuf(rack->vnc);
	if(pixbuf == NULL)
		return TRUE;

	g_return_val_if_fail(rack->backbuffer != NULL, TRUE);

	w = rack->da->allocation.width;
	h = rack->da->allocation.height;

	scale_x = (gdouble)w / (gdouble)gdk_pixbuf_get_width(pixbuf);
	scale_y = (gdouble)h / (gdouble)gdk_pixbuf_get_height(pixbuf);

	/* resize frame */
	gdk_pixbuf_composite(pixbuf, rack->backbuffer,
		0, 0, w, h,
		0.0, 0.0, scale_x, scale_y,
		GDK_INTERP_BILINEAR, 255);

	g_object_unref(G_OBJECT(pixbuf));

	rowstride = gdk_pixbuf_get_rowstride(rack->backbuffer);
	pixels = gdk_pixbuf_get_pixels(rack->backbuffer);

	gdk_draw_rgb_image_dithalign(
		rack->da->window,
		rack->da->style->black_gc,
		0, 0, w, h,
		GDK_RGB_DITHER_NORMAL,
		pixels, rowstride,
		0, 0);

	g_usleep(10000);

	return TRUE;
}

static void gui_da_resized_cb(GtkWidget *widget, GdkEventConfigure *ec,
	gpointer data)
{
	VRack *rack = (VRack *)data;

	g_return_if_fail(rack != NULL);

	if(rack->backbuffer) {
		g_object_unref(G_OBJECT(rack->backbuffer));
		rack->backbuffer = NULL;
	}

	if(ec->width == 0)
		return;

	rack->backbuffer = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
		ec->width, ec->height);
}

static void gui_get_scaled_pos(VRack *rack, gdouble x, gdouble y,
	gint *lx, gint *ly)
{
	gdouble scale_x, scale_y;

	scale_x = (gdouble)vnc_display_get_width(rack->vnc) /
		(gdouble)rack->da->allocation.width;
	scale_y = (gdouble)vnc_display_get_height(rack->vnc) /
		(gdouble)rack->da->allocation.height;

	*lx = (gint)(x * scale_x);
	*ly = (gint)(y * scale_y);
}

static gboolean gui_da_motion_cb(GtkWidget *widget, GdkEventMotion *em,
	gpointer data)
{
	VRack *rack = (VRack *)data;
	gint lx, ly;

	g_return_val_if_fail(rack != NULL, FALSE);

	gui_get_scaled_pos(rack, em->x, em->y, &lx, &ly);

	vnc_display_send_pointer(rack->vnc, lx, ly, rack->button_mask);

	return TRUE;
}

static gboolean gui_da_button_press_cb(GtkWidget *widget, GdkEventButton *eb,
	gpointer data)
{
	VRack *rack = (VRack *)data;
	gint lx, ly;

	g_return_val_if_fail(rack != NULL, FALSE);

	rack->button_mask |= (1 << (eb->button - 1));

	gui_get_scaled_pos(rack, eb->x, eb->y, &lx, &ly);

	g_debug("mouse: press  : %dx%d, %04x", lx, ly, rack->button_mask);

	vnc_display_send_pointer(rack->vnc, lx, ly, rack->button_mask);

	return TRUE;
}

static gboolean gui_da_button_release_cb(GtkWidget *widget, GdkEventButton *eb,
	gpointer data)
{
	VRack *rack = (VRack *)data;
	gint lx, ly;

	g_return_val_if_fail(rack != NULL, FALSE);

	rack->button_mask &= ~(1 << (eb->button - 1));

	gui_get_scaled_pos(rack, eb->x, eb->y, &lx, &ly);

	g_debug("mouse: release: %dx%d, %04x", lx, ly, rack->button_mask);

	vnc_display_send_pointer(rack->vnc, lx, ly, rack->button_mask);

	return TRUE;
}

static void gui_vnc_credential_cb(GtkWidget *vnc, GValueArray *credList)
{
	const gchar **data;
	GValue *cred;
	gint i;

	g_debug("got request for %d credential(s)", credList->n_values);

	data = g_new0(const gchar *, credList->n_values);

	for(i = 0; i < credList->n_values ; i++) {
		cred = g_value_array_get_nth(credList, i);
		switch(g_value_get_enum(cred)) {
			case VNC_DISPLAY_CREDENTIAL_USERNAME:
			case VNC_DISPLAY_CREDENTIAL_PASSWORD:
				g_debug("got username/password request");
				break;
			case VNC_DISPLAY_CREDENTIAL_CLIENTNAME:
				g_debug("clientname request");
				data[i] = "vrack";
				if(vnc_display_set_credential(VNC_DISPLAY(vnc),
					g_value_get_enum(cred), data[i]) != 0) {
					g_warning("failed to set credential type %d",
						g_value_get_enum(cred));
					vnc_display_close(VNC_DISPLAY(vnc));
				}
				break;
			default:
				g_debug("unsupported credential type %d",
					g_value_get_enum(cred));
				break;
		}
	}
	g_free(data);
}

static void gui_vnc_initialized_cb(GtkWidget *vnc, GtkWindow *window,
	gpointer data)
{
	g_debug("initialized");
}

static void gui_vnc_connected_cb(GtkWidget *vnc, gpointer data)
{
	g_debug("vnc: connected to %s (open: %u)",
		vnc_display_get_name(VNC_DISPLAY(vnc)),
		vnc_display_is_open(VNC_DISPLAY(vnc)));
}

static void gui_vnc_auth_fail_cb(GtkWidget *vnc, const gchar *msg,
	gpointer data)
{
	g_debug("auth failed: %s", msg ? msg : "");
}

gboolean gui_init(VRack *rack)
{
	GtkWidget *vbox;

	gtk_init(rack->argcp, rack->argvp);

	rack->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(rack->window), "VRack");
	g_signal_connect(G_OBJECT(rack->window), "delete-event",
		G_CALLBACK(gtk_main_quit), NULL);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(rack->window), vbox);

	rack->vnc = VNC_DISPLAY(vnc_display_new());
	g_signal_connect(G_OBJECT(rack->vnc), "vnc-auth-credential",
		G_CALLBACK(gui_vnc_credential_cb), rack);
	g_signal_connect(G_OBJECT(rack->vnc), "vnc-connected",
		G_CALLBACK(gui_vnc_connected_cb), rack);
	g_signal_connect(G_OBJECT(rack->vnc), "vnc-initialized",
		G_CALLBACK(gui_vnc_initialized_cb), rack);
	g_signal_connect(G_OBJECT(rack->vnc), "vnc-auth-failure",
		G_CALLBACK(gui_vnc_auth_fail_cb), rack);


	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(rack->vnc), TRUE, TRUE, 0);
	gtk_widget_realize(GTK_WIDGET(rack->vnc));

	rack->da = gtk_drawing_area_new();
	gtk_widget_add_events(rack->da,
		GDK_POINTER_MOTION_MASK |
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

	g_signal_connect(G_OBJECT(rack->da), "configure-event",
		G_CALLBACK(gui_da_resized_cb), rack);
	g_signal_connect(G_OBJECT(rack->da), "motion-notify-event",
		G_CALLBACK(gui_da_motion_cb), rack);
	g_signal_connect(G_OBJECT(rack->da), "button-press-event",
		G_CALLBACK(gui_da_button_press_cb), rack);
	g_signal_connect(G_OBJECT(rack->da), "button-release-event",
		G_CALLBACK(gui_da_button_release_cb), rack);

	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(rack->da), TRUE, TRUE, 0);

	g_idle_add(gui_idle_cb, rack);

	return TRUE;
}

gboolean gui_run(VRack *rack)
{
	gtk_widget_show_all(rack->window);
	gtk_widget_hide(GTK_WIDGET(rack->vnc));
	gtk_main();

	return TRUE;
}
