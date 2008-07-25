#define _POSIX_C_SOURCE 1
#define _XOPEN_SOURCE 1

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>

#include "main.h"
#include "rack.h"
#include "switch.h"
#include "tempdir.h"
#include "misc.h"
#include "cmdq.h"

#define PIDFILENAME "vde_switch.pid"
#define MGMTSOCK "mgmt.sock"

struct _VRackSwitch {
	VRackItem *rackitem;

	guint32 n_ports;
	gchar *command;
	gchar *tmpdir;
	GIOChannel *mgmt;
	CmdQ *qmgmt;
	GtkWidget *widget;
};

static GtkWidget *switch_create_widget(VRackSwitch *sw);
static void switch_cleanup(VRackSwitch *sw);
static gboolean switch_mgmt_prompt_cb(const gchar *text, gpointer user_data);
#if 0
static void switch_mgmt_help_cb(GIOChannel *io, const gchar *text,
	gpointer user_data);
#endif

VRackSwitch *switch_new(VRackCtxt *ctxt, guint32 n_ports)
{
	VRackSwitch *sw;
	gchar *s_stdout = NULL, *s_stderr = NULL, *name;
	gint status;
	GError *error = NULL;

	sw = g_new0(VRackSwitch, 1);
	sw->n_ports = n_ports;
	sw->tmpdir = tempdir_create(ctxt->tmpdir, "switch.0");

	sw->command = g_strdup_printf("vde_switch "
		"--daemon "
		"--pidfile %s%c" PIDFILENAME " "
		"--numports %d "
		"--sock %s "
		"--mgmt %s%c" MGMTSOCK " ",
		sw->tmpdir, G_DIR_SEPARATOR,
		sw->n_ports,
		sw->tmpdir,
		sw->tmpdir, G_DIR_SEPARATOR);
	g_debug("starting switch: %s", sw->command);

	/* starting vde process */
	if(g_spawn_command_line_sync(sw->command, &s_stdout, &s_stderr, &status,
		&error)) {
		g_strstrip(s_stdout);
		g_strstrip(s_stderr);
#if 0
		g_debug("switch stdout:\n%s\nswitch stderr:\n%s\n",
			s_stdout, s_stderr);
#endif
		if(s_stdout)
			g_free(s_stdout);
		if(s_stderr)
			g_free(s_stderr);
	}
	if(error != NULL) {
		g_warning("starting switch resulted in an error: %s", error->message);
		g_error_free(error);
		switch_cleanup(sw);
		return NULL;
	}

	/* wait for pid file (successful start) */
	error = NULL;
	name = g_strdup_printf("%s%c" PIDFILENAME, sw->tmpdir, G_DIR_SEPARATOR);
	if(!misc_wait_for_file(name, 2000)) {
		g_warning("switch: pid file '%s' not created", name);
		g_free(name);
		switch_cleanup(sw);
		return NULL;
	}
	g_free(name);

	/* open management socket */
	name = g_strdup_printf("%s%c" MGMTSOCK, sw->tmpdir, G_DIR_SEPARATOR);
	if(!misc_wait_for_file(name, 2000)) {
		g_warning("switch: could not find management socket '%s'", name);
		g_free(name);
		switch_cleanup(sw);
		return NULL;
	}

	sw->mgmt = misc_connect_to_socket(name);
	g_free(name);
	if(sw->mgmt == NULL) {
		switch_cleanup(sw);
		return NULL;
	}

	sw->qmgmt = cmdq_create(sw->mgmt, switch_mgmt_prompt_cb, sw);

	cmdq_add(sw->qmgmt, "", NULL); /* flush greeting */

	sw->widget = switch_create_widget(sw);
	sw->rackitem = rack_item_new(sw->widget, 1);

	return sw;
}

void switch_shutdown(VRackSwitch *sw)
{
	gint i;
	gchar *pidfile;

	if(sw == NULL)
		return;

	/* get pid */
	pidfile = g_strdup_printf("%s%c" PIDFILENAME, sw->tmpdir, G_DIR_SEPARATOR);
	cmdq_add(sw->qmgmt, "shutdown", NULL);

	/* wait for termination */
	for(i = 0; i < 2000; i ++) {
		if(!g_file_test(pidfile, G_FILE_TEST_IS_REGULAR)) {
			tempdir_remove(sw->tmpdir);
			break;
		}
		g_usleep(1000);
	}
	if(g_file_test(pidfile, G_FILE_TEST_IS_REGULAR))
		g_debug("switch: failed to terminate process");

	g_free(pidfile);
	switch_cleanup(sw);
}

/****************************************************************************/

static GtkWidget *switch_create_widget(VRackSwitch *sw)
{
	GtkWidget *widget;

	widget = gtk_frame_new("switch");
	gtk_widget_set_tooltip_text(widget, sw->command);
	return widget;
}

static void switch_cleanup(VRackSwitch *sw)
{
	int fd;

	rack_item_free(sw->rackitem);

	if(sw->mgmt) {
		fd = g_io_channel_unix_get_fd(sw->mgmt);
		g_io_channel_unref(sw->mgmt);
		close(fd);
	}
	if(sw->tmpdir)
		g_free(sw->tmpdir);
	cmdq_destroy(sw->qmgmt);
	g_free(sw);
}

static gboolean switch_mgmt_prompt_cb(const gchar *text, gpointer user_data)
{
	gboolean retval;

	retval = (strcmp(text + (strlen(text) - 5), "vde$ ") == 0);
	return retval;
}

#if 0
static void switch_mgmt_help_cb(GIOChannel *io, const gchar *text,
	gpointer user_data)
{
	g_debug("switch_mgmt_help_cb: %s", text);
}
#endif

