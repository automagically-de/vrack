#include <gtk/gtk.h>
#include <vncdisplay.h>

#include "main.h"
#include "rack.h"
#include "display.h"

struct _VRackDisplay {
	VRackItem *rackitem;

	GtkWidget *widget;
	GtkWidget *da;
	GdkPixbuf *backbuffer;
	VncDisplay *vnc;
	guint32 button_mask;
};

static GtkWidget *display_create_widget(VRackDisplay *dpy);
static void display_get_scaled_pos(VRackDisplay *dpy, gdouble x, gdouble y,
	gint *lx, gint *ly);

static gboolean display_update_cb(gpointer data);
static void display_da_resized_cb(GtkWidget *widget, GdkEventConfigure *ec,
	gpointer data);
static gboolean display_da_motion_cb(GtkWidget *widget, GdkEventMotion *em,
	gpointer data);
static gboolean display_da_button_press_cb(GtkWidget *widget,
	GdkEventButton *eb, gpointer data);
static gboolean display_da_button_release_cb(GtkWidget *widget,
	GdkEventButton *eb,	gpointer data);
static void display_vnc_credential_cb(GtkWidget *vnc, GValueArray *credList);
static void display_vnc_initialized_cb(GtkWidget *vnc, GtkWindow *window,
	gpointer data);
static void display_vnc_connected_cb(GtkWidget *vnc, gpointer data);
static void display_vnc_auth_fail_cb(GtkWidget *vnc, const gchar *msg,
	gpointer data);

VRackDisplay *display_new(VRackCtxt *ctxt)
{
	VRackDisplay *dpy;

	dpy = g_new0(VRackDisplay, 1);

	dpy->widget = display_create_widget(dpy);
	/* FIXME: evil hack */
	ctxt->vnc = dpy->vnc;
	dpy->rackitem = rack_item_new(dpy->widget, 8);
	g_idle_add(display_update_cb, dpy);

	return dpy;
}

void display_shutdown(VRackDisplay *dpy)
{
	g_free(dpy);
}

/*****************************************************************************/
/* non-callback private stuff                                                */

static GtkWidget *display_create_widget(VRackDisplay *dpy)
{
	GtkWidget *widget, *vbox;

	widget = gtk_frame_new("display");
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(widget), vbox);

	/* real vnc widget */
	dpy->vnc = VNC_DISPLAY(vnc_display_new());
	g_signal_connect(G_OBJECT(dpy->vnc), "vnc-auth-credential",
		G_CALLBACK(display_vnc_credential_cb), dpy);
	g_signal_connect(G_OBJECT(dpy->vnc), "vnc-connected",
		G_CALLBACK(display_vnc_connected_cb), dpy);
	g_signal_connect(G_OBJECT(dpy->vnc), "vnc-initialized",
		G_CALLBACK(display_vnc_initialized_cb), dpy);
	g_signal_connect(G_OBJECT(dpy->vnc), "vnc-auth-failure",
		G_CALLBACK(display_vnc_auth_fail_cb), dpy);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(dpy->vnc), TRUE, TRUE, 0);

	/* drawing area for scaled output */
	dpy->da = gtk_drawing_area_new();
	gtk_widget_set_size_request(dpy->da, 240, 180);
	gtk_widget_add_events(dpy->da,
		GDK_POINTER_MOTION_MASK |
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
	g_signal_connect(G_OBJECT(dpy->da), "configure-event",
		G_CALLBACK(display_da_resized_cb), dpy);
	g_signal_connect(G_OBJECT(dpy->da), "motion-notify-event",
		G_CALLBACK(display_da_motion_cb), dpy);
	g_signal_connect(G_OBJECT(dpy->da), "button-press-event",
		G_CALLBACK(display_da_button_press_cb), dpy);
	g_signal_connect(G_OBJECT(dpy->da), "button-release-event",
		G_CALLBACK(display_da_button_release_cb), dpy);
	gtk_box_pack_start(GTK_BOX(vbox), dpy->da, TRUE, TRUE, 0);

	return widget;
}

static void display_get_scaled_pos(VRackDisplay *dpy, gdouble x, gdouble y,
	gint *lx, gint *ly)
{
	gdouble scale_x, scale_y;

	scale_x = (gdouble)vnc_display_get_width(dpy->vnc) /
		(gdouble)dpy->da->allocation.width;
	scale_y = (gdouble)vnc_display_get_height(dpy->vnc) /
		(gdouble)dpy->da->allocation.height;

	*lx = (gint)(x * scale_x);
	*ly = (gint)(y * scale_y);
}

/*****************************************************************************/
/* callback functions                                                        */

static gboolean display_update_cb(gpointer data)
{
	VRackDisplay *dpy = (VRackDisplay *)data;
	GdkPixbuf *pixbuf;
	guint32 w, h, rowstride;
	guint8 *pixels;
	gdouble scale_x, scale_y;

	g_return_val_if_fail(dpy != NULL, FALSE);

	pixbuf = vnc_display_get_pixbuf(dpy->vnc);
	if(pixbuf == NULL)
		return TRUE;

	g_return_val_if_fail(dpy->backbuffer != NULL, TRUE);

	w = dpy->da->allocation.width;
	h = dpy->da->allocation.height;

	scale_x = (gdouble)w / (gdouble)gdk_pixbuf_get_width(pixbuf);
	scale_y = (gdouble)h / (gdouble)gdk_pixbuf_get_height(pixbuf);

	/* resize frame */
	gdk_pixbuf_composite(pixbuf, dpy->backbuffer,
		0, 0, w, h,
		0.0, 0.0, scale_x, scale_y,
		GDK_INTERP_BILINEAR, 255);

	g_object_unref(G_OBJECT(pixbuf));

	rowstride = gdk_pixbuf_get_rowstride(dpy->backbuffer);
	pixels = gdk_pixbuf_get_pixels(dpy->backbuffer);

	gdk_draw_rgb_image_dithalign(
		dpy->da->window,
		dpy->da->style->black_gc,
		0, 0, w, h,
		GDK_RGB_DITHER_NORMAL,
		pixels, rowstride,
		0, 0);

	g_usleep(10000);

	return TRUE;
}

static void display_da_resized_cb(GtkWidget *widget, GdkEventConfigure *ec,
	gpointer data)
{
	VRackDisplay *dpy = (VRackDisplay *)data;

	g_return_if_fail(dpy != NULL);

	if(dpy->backbuffer) {
		g_object_unref(G_OBJECT(dpy->backbuffer));
		dpy->backbuffer = NULL;
	}

	if(ec->width == 0)
		return;

	dpy->backbuffer = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
		ec->width, ec->height);
}

static gboolean display_da_motion_cb(GtkWidget *widget, GdkEventMotion *em,
	gpointer data)
{
	VRackDisplay *dpy = (VRackDisplay *)data;
	gint lx, ly;

	g_return_val_if_fail(dpy != NULL, FALSE);

	display_get_scaled_pos(dpy, em->x, em->y, &lx, &ly);

	vnc_display_send_pointer(dpy->vnc, lx, ly, dpy->button_mask);

	return TRUE;
}

static gboolean display_da_button_press_cb(GtkWidget *widget,
	GdkEventButton *eb, gpointer data)
{
	VRackDisplay *dpy = (VRackDisplay *)data;
	gint lx, ly;

	g_return_val_if_fail(dpy != NULL, FALSE);

	dpy->button_mask |= (1 << (eb->button - 1));

	display_get_scaled_pos(dpy, eb->x, eb->y, &lx, &ly);

	g_debug("mouse: press  : %dx%d, %04x", lx, ly, dpy->button_mask);

	vnc_display_send_pointer(dpy->vnc, lx, ly, dpy->button_mask);

	return TRUE;
}


static gboolean display_da_button_release_cb(GtkWidget *widget,
	GdkEventButton *eb,	gpointer data)
{
	VRackDisplay *dpy = (VRackDisplay *)data;
	gint lx, ly;

	g_return_val_if_fail(dpy != NULL, FALSE);

	dpy->button_mask &= ~(1 << (eb->button - 1));

	display_get_scaled_pos(dpy, eb->x, eb->y, &lx, &ly);

	g_debug("mouse: release: %dx%d, %04x", lx, ly, dpy->button_mask);

	vnc_display_send_pointer(dpy->vnc, lx, ly, dpy->button_mask);

	return TRUE;
}

static void display_vnc_credential_cb(GtkWidget *vnc, GValueArray *credList)
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

static void display_vnc_initialized_cb(GtkWidget *vnc, GtkWindow *window,
	gpointer data)
{
	g_debug("initialized");
}

static void display_vnc_connected_cb(GtkWidget *vnc, gpointer data)
{
	g_debug("vnc: connected to %s (open: %u)",
		vnc_display_get_name(VNC_DISPLAY(vnc)),
		vnc_display_is_open(VNC_DISPLAY(vnc)));
}

static void display_vnc_auth_fail_cb(GtkWidget *vnc, const gchar *msg,
	gpointer data)
{
	g_debug("auth failed: %s", msg ? msg : "");
}


