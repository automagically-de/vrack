#define _POSIX_C_SOURCE 1
#define _XOPEN_SOURCE 1

#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "main.h"
#include "switch.h"
#include "tempdir.h"

#define PIDFILENAME "vde_switch.pid"

struct _VRackSwitch {
	guint32 n_ports;
	GPid pid;
	gchar *tmpdir;
};

VRackSwitch *switch_new(VRackCtxt *ctxt, guint32 n_ports)
{
	VRackSwitch *sw;
	gchar *s_stdout = NULL, *s_stderr = NULL, *cmd, *cnt, *name;
	gint status;
	GError *error = NULL;

	sw = g_new0(VRackSwitch, 1);
	sw->n_ports = n_ports;
	sw->tmpdir = tempdir_create(ctxt->tmpdir, "switch.0");

	cmd = g_strdup_printf("vde_switch "
		"--daemon "
		"--pidfile %s%c" PIDFILENAME " "
		"--numports %d "
		"--sock %s "
		"--mgmt %s%cmgmt.sock ",
		sw->tmpdir, G_DIR_SEPARATOR,
		sw->n_ports,
		sw->tmpdir,
		sw->tmpdir, G_DIR_SEPARATOR);
	g_debug("starting switch: %s", cmd);

	/* starting vde process */
	if(g_spawn_command_line_sync(cmd, &s_stdout, &s_stderr, &status, &error)) {
		g_strstrip(s_stdout);
		g_strstrip(s_stderr);
		g_debug("switch stdout:\n%s\nswitch stderr:\n%s\n",
			s_stdout, s_stderr);

		if(s_stdout)
			g_free(s_stdout);
		if(s_stderr)
			g_free(s_stderr);
	}
	if(error != NULL) {
		g_warning("starting switch resulted in an error: %s", error->message);
		g_error_free(error);
	}

	/* get pid of process */
	error = NULL;
	name = g_strdup_printf("%s%c" PIDFILENAME, sw->tmpdir, G_DIR_SEPARATOR);
	if(!g_file_get_contents(name, &cnt, NULL, &error)) {
		g_warning("switch: could not get pid from '%s'", error->message);
		g_error_free(error);
	}
	g_free(name);
	if(cnt) {
		sw->pid = atoi(cnt);
		g_free(cnt);
		g_debug("switch: started with pid %d", sw->pid);
	}

	return sw;
}

void switch_shutdown(VRackSwitch *sw)
{
	gint i;
	gchar *pidfile;

	g_return_if_fail(sw->pid != 0);

	kill(sw->pid, SIGTERM);
	pidfile = g_strdup_printf("%s%c" PIDFILENAME, sw->tmpdir, G_DIR_SEPARATOR);
	for(i = 0; i < 2000; i ++) {
		if(!g_file_test(pidfile, G_FILE_TEST_IS_REGULAR)) {
			tempdir_remove(sw->tmpdir);
			break;
		}
		g_usleep(1000);
	}
	if(g_file_test(pidfile, G_FILE_TEST_IS_REGULAR))
		g_debug("switch: failed to terminate process %d", sw->pid);
	g_free(pidfile);
	g_free(sw);
}
