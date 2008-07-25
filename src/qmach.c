#define _POSIX_C_SOURCE 1
#define _XOPEN_SOURCE 1

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>

#include "main.h"
#include "qmach.h"
#include "tempdir.h"
#include "misc.h"
#include "cmdq.h"

#define PIDFILE "qemu.pid"
#define MONSOCK "monitor.sock"
#define SER0SOCK "serial0.sock"

struct _VRackQMach {
	gchar *description;
	gchar *tmpdir;
	GIOChannel *mon;
	GIOChannel *ser0;
	CmdQ *qmon;
};

static void qmach_cleanup(VRackQMach *qm);
static gboolean qmach_mon_prompt_cb(const gchar *text, gpointer user_data);

VRackQMach *qmach_new(VRackCtxt *ctxt, const gchar *description)
{
	VRackQMach *qm;
	gchar *name, *cmd, *s_stdout = NULL, *s_stderr = NULL;
	gint status = 0;
	GError *error = NULL;

	qm = g_new0(VRackQMach, 1);
	qm->description = g_strdup(description);
	name = g_strdup_printf("qmach.0x%04x", g_random_int_range(0, 1024*64));
	qm->tmpdir = tempdir_create(ctxt->tmpdir, name);
	g_free(name);

	cmd = g_strdup_printf("qemu "
		"-daemonize "
		"-pidfile %s%c" PIDFILE " "
		"-monitor unix:%s%c" MONSOCK ",server,nowait "
		"-vnc localhost:%u "
		/* arch specific */
		"-serial unix:%s%c" SER0SOCK ",server,nowait "
		"-cdrom %s -boot d",
		qm->tmpdir, G_DIR_SEPARATOR,
		qm->tmpdir, G_DIR_SEPARATOR,
		10,
		qm->tmpdir, G_DIR_SEPARATOR,
		"/media/disc2/ISOs/dsl-n-01RC4.iso");
	g_debug("qmach: starting qemu: %s", cmd);

	/* starting qemu process */
	if(g_spawn_command_line_sync(cmd, &s_stdout, &s_stderr, &status, &error)) {
		g_strstrip(s_stdout);
		g_strstrip(s_stderr);
		g_debug("qmach: stdout:\n%s\nswitch stderr:\n%s\n",
			s_stdout, s_stderr);

		if(s_stdout)
			g_free(s_stdout);
		if(s_stderr)
			g_free(s_stderr);
	}
	if(error != NULL) {
		g_warning("qmach: starting qemu failed: %s", error->message);
		g_error_free(error);
		qmach_cleanup(qm);
		return NULL;
	}

	/* wait for pid file (successful start) */
	error = NULL;
	name = g_strdup_printf("%s%c" PIDFILE, qm->tmpdir, G_DIR_SEPARATOR);
	if(!misc_wait_for_file(name, 2000)) {
		g_warning("qmach:  pid file '%s' not created", name);
		g_free(name);
		qmach_cleanup(qm);
		return NULL;
	}

	/* open monitor socket */
	name = g_strdup_printf("%s%c" MONSOCK, qm->tmpdir, G_DIR_SEPARATOR);
	if(!misc_wait_for_file(name, 2000)) {
		g_warning("qmach: could not find monitor socket '%s'", name);
		g_free(name);
		qmach_cleanup(qm);
		return NULL;
	}

	qm->mon = misc_connect_to_socket(name);
	g_free(name);
	if(qm->mon == NULL) {
		qmach_cleanup(qm);
		return NULL;
	}

	qm->qmon = cmdq_create(qm->mon, qmach_mon_prompt_cb, qm);

	cmdq_add(qm->qmon, "", NULL);
	return qm;
}

void qmach_shutdown(VRackQMach *qm)
{
	gint i;
	gchar *pidfile;

	if(qm == NULL)
		return;

	pidfile = g_strdup_printf("%s%c" PIDFILE, qm->tmpdir, G_DIR_SEPARATOR);
	cmdq_add(qm->qmon, "quit", NULL);

	/* wait for termination */
	for(i = 0; i < 2000; i ++) {
		if(!g_file_test(pidfile, G_FILE_TEST_IS_REGULAR)) {
			tempdir_remove(qm->tmpdir);
			break;
		}
		g_usleep(1000);
	}
	if(g_file_test(pidfile, G_FILE_TEST_IS_REGULAR))
		g_debug("qmach: failed to terminate process");

	g_free(pidfile);
	qmach_cleanup(qm);
}

static void qmach_cleanup(VRackQMach *qm)
{
	int fd;

	if(qm->mon) {
		fd = g_io_channel_unix_get_fd(qm->mon);
		g_io_channel_unref(qm->mon);
		close(fd);
	}
	if(qm->description)
		g_free(qm->description);
	if(qm->tmpdir) {
		g_free(qm->tmpdir);
	}
	cmdq_destroy(qm->qmon);
	g_free(qm);
}

static gboolean qmach_mon_prompt_cb(const gchar *text, gpointer user_data)
{
	gboolean retval;

	g_debug("qmach prompt: %s", text + (strlen(text) - 7));

	retval = (strstr(text, "(qemu)") != NULL);
	return retval;
}

