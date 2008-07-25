#include <gdk/gdkkeysyms.h>

#include "vnc.h"

gboolean vnc_init(VRackCtxt *ctxt)
{
	static guint keyvals[] = { GDK_Shift_L };

	vnc_display_set_scaling(              ctxt->vnc, FALSE);
	vnc_display_set_read_only(            ctxt->vnc, FALSE);
	vnc_display_set_pointer_local(        ctxt->vnc, FALSE);
	vnc_display_set_lossy_encoding(       ctxt->vnc, FALSE);
	vnc_display_set_keyboard_grab(        ctxt->vnc, FALSE);
	vnc_display_set_pointer_grab(         ctxt->vnc, FALSE);
	vnc_display_set_shared_flag(          ctxt->vnc, FALSE);
	vnc_display_force_grab(               ctxt->vnc, FALSE);

	vnc_display_send_keys(ctxt->vnc, keyvals, 1);

	g_debug("pointer is %sabsolute",
		vnc_display_is_pointer_absolute(ctxt->vnc) ? "" : "not ");

	if(!vnc_display_open_host(ctxt->vnc, "localhost", "5910")) {
		g_warning("vnc: failed to open localhost:5901");
		return FALSE;
	} else {
		g_debug("vnc: connected to %s (open: %u)",
			vnc_display_get_name(ctxt->vnc),
			vnc_display_is_open(ctxt->vnc));
	}

	return TRUE;
}
