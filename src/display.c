#include <gtk/gtk.h>
#include <vncdisplay.h>

#include "main.h"
#include "rack.h"
#include "display.h"
#include "kvm.h"

struct _VRackDisplay {
	VRackItem *rackitem;

	VRackKvmSource *source;
	GtkWidget *widget;
	GtkWidget *da;
	GdkPixbuf *backbuffer;
	guint32 button_mask;
};

static GtkWidget *display_create_widget(VRackDisplay *dpy);
static void display_get_scaled_pos(VRackDisplay *dpy,
	guint32 source_width, guint32 source_height,
	gdouble x, gdouble y, gint *lx, gint *ly);

static gboolean display_update_cb(gpointer data);

static void display_da_resized_cb(GtkWidget *widget, GdkEventConfigure *ec,
	gpointer data);
static gboolean display_da_motion_cb(GtkWidget *widget, GdkEventMotion *em,
	gpointer data);
static gboolean display_da_button_press_cb(GtkWidget *widget,
	GdkEventButton *eb, gpointer data);
static gboolean display_da_button_release_cb(GtkWidget *widget,
	GdkEventButton *eb,	gpointer data);

VRackDisplay *display_new(VRackCtxt *ctxt)
{
	VRackDisplay *dpy;

	dpy = g_new0(VRackDisplay, 1);

	dpy->widget = display_create_widget(dpy);
	dpy->rackitem = rack_item_new(dpy->widget, 8);

	g_timeout_add(100, display_update_cb, dpy);

	return dpy;
}

void display_shutdown(VRackDisplay *dpy)
{
	g_free(dpy);
}

gboolean display_set_kvm_source(VRackDisplay *dpy, VRackKvmSource *source)
{
	dpy->source = source;
	return TRUE;
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

static void display_get_scaled_pos(VRackDisplay *dpy,
	guint32 source_width, guint32 source_height,
	gdouble x, gdouble y, gint *lx, gint *ly)
{
	gdouble scale_x, scale_y;

	scale_x = (gdouble)source_width /
		(gdouble)dpy->da->allocation.width;
	scale_y = (gdouble)source_height /
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
	if(dpy->source == NULL)
		return TRUE;

	pixbuf = dpy->source->get_pixbuf(dpy->source->opaque);
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
	gint lx = 0, ly = 0;
	guint32 srcw, srch;

	g_return_val_if_fail(dpy != NULL, FALSE);
	if(dpy->source == NULL)
		return TRUE;

	if(dpy->source->get_size(dpy->source->opaque, &srcw, &srch)) {
		display_get_scaled_pos(dpy, srcw, srch, em->x, em->y, &lx, &ly);
		if(dpy->source->send_mouse_move)
			dpy->source->send_mouse_move(dpy->source->opaque, lx, ly);
	}

	g_debug("display: mouse move to %d, %d", lx, ly);

	return TRUE;
}

static gboolean display_da_button_press_cb(GtkWidget *widget,
	GdkEventButton *eb, gpointer data)
{
	VRackDisplay *dpy = (VRackDisplay *)data;

	g_return_val_if_fail(dpy != NULL, FALSE);
	if(dpy->source == NULL)
		return TRUE;

	dpy->button_mask |= (1 << (eb->button - 1));
	if(dpy->source->send_mouse_button)
		dpy->source->send_mouse_button(dpy->source->opaque, dpy->button_mask);

	g_debug("display: mouse buttons: %04x", dpy->button_mask);

	return TRUE;
}

static gboolean display_da_button_release_cb(GtkWidget *widget,
	GdkEventButton *eb,	gpointer data)
{
	VRackDisplay *dpy = (VRackDisplay *)data;

	g_return_val_if_fail(dpy != NULL, FALSE);
	if(dpy->source == NULL)
		return TRUE;

	dpy->button_mask &= ~(1 << (eb->button - 1));
	dpy->source->send_mouse_button(dpy->source->opaque, dpy->button_mask);

	g_debug("display: mouse release: %04x", dpy->button_mask);

	return TRUE;
}

