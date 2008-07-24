#include <gdk/gdkkeysyms.h>

#include "vnc.h"

gboolean vnc_init(VRack *rack)
{
	static guint keyvals[] = { GDK_Shift_L };

	vnc_display_set_scaling(              rack->vnc, FALSE);
	vnc_display_set_read_only(            rack->vnc, FALSE);
	vnc_display_set_pointer_local(        rack->vnc, FALSE);
	vnc_display_set_lossy_encoding(       rack->vnc, FALSE);
	vnc_display_set_keyboard_grab(        rack->vnc, TRUE);
	vnc_display_set_pointer_grab(         rack->vnc, TRUE);
	vnc_display_set_shared_flag(          rack->vnc, TRUE);
	vnc_display_force_grab(               rack->vnc, TRUE);

	vnc_display_send_keys(rack->vnc, keyvals, 1);

	g_debug("pointer is %sabsolute",
		vnc_display_is_pointer_absolute(rack->vnc) ? "" : "not ");

	if(!vnc_display_open_host(rack->vnc, "localhost", "5901")) {
		g_warning("vnc: failed to open localhost:5901");
		return FALSE;
	} else {
		g_debug("vnc: connected to %s (open: %u)",
			vnc_display_get_name(rack->vnc),
			vnc_display_is_open(rack->vnc));
	}

	return TRUE;
}
