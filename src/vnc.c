#include <gdk/gdkkeysyms.h>

#include "main.h"
#include "vnc.h"
#include "rack.h"

struct VRackVncPriv {
	gchar *host;
	guint32 token;
	guint32 port;
	VRackKvmSource *source;
};

static GdkPixbuf *vnc_get_pixbuf_cb(gpointer opaque);
static gboolean vnc_get_size_cb(gpointer opaque, guint32 *w, guint32 *h);
static gboolean vnc_disconnect(VRackCtxt *ctxt);

gboolean vnc_init(VRackCtxt *ctxt)
{
	struct VRackVncPriv *vncpriv;

	vncpriv = g_new0(struct VRackVncPriv, 1);
	vncpriv->source = g_new0(VRackKvmSource, 1);
	vncpriv->source->get_pixbuf = vnc_get_pixbuf_cb;
	vncpriv->source->get_size = vnc_get_size_cb;
	vncpriv->source->opaque = ctxt;
	ctxt->vnc_priv = vncpriv;

	return TRUE;
}

GtkWidget *vnc_create_widget(VRackCtxt *ctxt)
{
	/* real vnc widget */
	ctxt->vnc = VNC_DISPLAY(vnc_display_new());

	vnc_display_set_scaling(              ctxt->vnc, FALSE);
	vnc_display_set_read_only(            ctxt->vnc, FALSE);
	vnc_display_set_pointer_local(        ctxt->vnc, FALSE);
	vnc_display_set_lossy_encoding(       ctxt->vnc, FALSE);
	vnc_display_set_keyboard_grab(        ctxt->vnc, FALSE);
	vnc_display_set_pointer_grab(         ctxt->vnc, FALSE);
	vnc_display_set_shared_flag(          ctxt->vnc, FALSE);
	vnc_display_force_grab(               ctxt->vnc, FALSE);

	return GTK_WIDGET(ctxt->vnc);
}

VRackKvmSource *vnc_get_source(VRackCtxt *ctxt, guint32 token,
	const gchar *host, guint32 port)
{
	struct VRackVncPriv *vncpriv = ctxt->vnc_priv;
	gchar *sp;

	g_return_val_if_fail(vncpriv != NULL, NULL);

	if(token == vncpriv->token)
		return vncpriv->source;

	vnc_disconnect(ctxt);

	sp = g_strdup_printf("%d", port);
	if(!vnc_display_open_host(ctxt->vnc, host, sp)) {
		g_warning("vnc: failed to connect to %s:%d", host, port);
		g_free(sp);
		return NULL;
	}

	vncpriv->token = token;
	vncpriv->host = g_strdup(host);
	vncpriv->port = port;

	return vncpriv->source;
}

/*****************************************************************************/

static GdkPixbuf *vnc_get_pixbuf_cb(gpointer opaque)
{
	VRackCtxt *ctxt = opaque;

	return vnc_display_get_pixbuf(ctxt->vnc);
}

static gboolean vnc_get_size_cb(gpointer opaque, guint32 *w, guint32 *h)
{
	VRackCtxt *ctxt = opaque;

	*w = vnc_display_get_width(ctxt->vnc);
	*h = vnc_display_get_height(ctxt->vnc);

	return TRUE;
}

static gboolean vnc_disconnect(VRackCtxt *ctxt)
{
	struct VRackVncPriv *vncpriv = ctxt->vnc_priv;

	vnc_display_close(ctxt->vnc);

	if(vncpriv->host != NULL)
		g_free(vncpriv->host);
	vncpriv->token = 0;
	vncpriv->port = 0;

	return TRUE;
}
