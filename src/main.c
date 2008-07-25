#include <stdlib.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <vncdisplay.h>

#include "main.h"
#include "gui.h"
#include "vnc.h"
#include "switch.h"
#include "qmach.h"
#include "tempdir.h"

static void main_cleanup(VRackCtxt *ctxt);

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	VRackCtxt *ctxt;
	VRackSwitch *sw;
	VRackQMach *qm;
	gchar *name;

	ctxt = g_new0(VRackCtxt, 1);
	ctxt->argcp = &argc;
	ctxt->argvp = &argv;

	/* create temporary directory */
	name = g_strdup_printf("vrack-0x%04x", g_random_int_range(0, 1024*64));
	ctxt->tmpdir = tempdir_create("/tmp", name);
	g_free(name);
	g_return_val_if_fail(ctxt->tmpdir != NULL, EXIT_FAILURE);

	context = g_option_context_new("- VRack");
	g_option_context_add_group(context, vnc_display_get_option_group());
	g_option_context_parse(context, ctxt->argcp, ctxt->argvp, &error);
	if(error) {
		g_printerr("options: %s", error->message);
		g_error_free(error);
	}

	gui_init(ctxt);
	vnc_init(ctxt);

	sw = switch_new(ctxt, 16);
	if(sw)
		ctxt->switches = g_slist_append(ctxt->switches, sw);

	qm = qmach_new(ctxt, "");
	if(qm)
		ctxt->machines = g_slist_append(ctxt->machines, qm);

	gui_run(ctxt);

	main_cleanup(ctxt);

	return EXIT_SUCCESS;
}

static void main_cleanup(VRackCtxt *ctxt)
{
	GSList *item;

	/* remove machines */
	item = ctxt->machines;
	while(item) {
		qmach_shutdown((VRackQMach *)item->data);
		item = g_slist_remove(item, item->data);
	}
	/* remove switches */
	item = ctxt->switches;
	while(item) {
		switch_shutdown((VRackSwitch *)item->data);
		item = g_slist_remove(item, item->data);
	}
	/* clean up */
	tempdir_remove(ctxt->tmpdir);
	g_free(ctxt->tmpdir);
	g_free(ctxt);
}
