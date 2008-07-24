#include <stdlib.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <vncdisplay.h>

#include "main.h"
#include "gui.h"
#include "vnc.h"

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	VRack *rack = g_new0(VRack, 1);
	rack->argcp = &argc;
	rack->argvp = &argv;

	context = g_option_context_new("- VRack");
	g_option_context_add_group(context, vnc_display_get_option_group());
	g_option_context_parse(context, rack->argcp, rack->argvp, &error);
	if(error) {
		g_printerr("options: %s", error->message);
		g_error_free(error);
	}

	gui_init(rack);
	vnc_init(rack);

	gui_run(rack);

	g_free(rack);
	return EXIT_SUCCESS;
}
