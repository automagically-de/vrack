#include <gtk/gtk.h>

#include "main.h"

gboolean gui_init(VRackCtxt *ctxt)
{
	gtk_init(ctxt->argcp, ctxt->argvp);

	return TRUE;
}

gboolean gui_create(VRackCtxt *ctxt, GtkWidget *rackwidget)
{
	GtkWidget *vbox;

	ctxt->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(ctxt->window), "VRack");
	g_signal_connect(G_OBJECT(ctxt->window), "delete-event",
		G_CALLBACK(gtk_main_quit), NULL);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(ctxt->window), vbox);

	gtk_box_pack_start(GTK_BOX(vbox), rackwidget, TRUE, TRUE, 5);

	return TRUE;
}

gboolean gui_run(VRackCtxt *ctxt)
{
	gtk_widget_show_all(ctxt->window);
	gtk_widget_hide(GTK_WIDGET(ctxt->vnc));
	gtk_main();

	return TRUE;
}
