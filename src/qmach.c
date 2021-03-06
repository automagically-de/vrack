#define _POSIX_C_SOURCE 1
#define _XOPEN_SOURCE 1

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <glib/gstdio.h>

#include <unistd.h>
#include <sys/types.h>

#include "main.h"
#include "vnc.h"
#include "rack.h"
#include "kvm.h"
#include "qmach.h"
#include "tempdir.h"
#include "misc.h"
#include "cmdq.h"

#define PIDFILE "qemu.pid"
#define MONSOCK "monitor.sock"
#define SER0SOCK "serial0.sock"

struct _VRackQMach {
	VRackItem *rackitem;

	VRackCtxt *ctxt;
	VRackKvmSource *kvm_source;

	guint32 vnc_token;
	gchar *vnc_host;
	guint32 vnc_port;

	gchar *description;
	gchar *command;
	gchar *tmpdir;
	GIOChannel *mon;
	GIOChannel *ser0;
	CmdQ *qmon;
	GtkWidget *widget;
};

static GtkWidget *qmach_create_widget(VRackQMach *qm);
static void qmach_cleanup(VRackQMach *qm);
static gboolean qmach_mon_prompt_cb(const gchar *text, gpointer user_data);
#if 0
static void qmach_mon_help_cb(GIOChannel *io, const gchar *text,
	gpointer user_data);
#endif

static GdkPixbuf *qmach_kvm_get_pixbuf_cb(gpointer opaque);
static gboolean qmach_kvm_get_size_cb(gpointer opaque, guint32 *w, guint32 *h);
static gboolean qmach_kvm_send_mouse_move_cb(gpointer opaque,
	guint32 x, guint32 y);
static gboolean qmach_kvm_send_mouse_button_cb(gpointer opaque,
	guint32 button_mask);

/*****************************************************************************/

VRackQMach *qmach_new(VRackCtxt *ctxt, const gchar *description)
{
	VRackQMach *qm;
	gchar *name, *s_stdout = NULL, *s_stderr = NULL;
	gint status = 0;
	GError *error = NULL;

	qm = g_new0(VRackQMach, 1);
	qm->ctxt = ctxt;

	qm->kvm_source = g_new(VRackKvmSource, 1);
	qm->kvm_source->opaque = qm;
	qm->kvm_source->get_pixbuf = qmach_kvm_get_pixbuf_cb;
	qm->kvm_source->get_size = qmach_kvm_get_size_cb;
	qm->kvm_source->send_mouse_move = qmach_kvm_send_mouse_move_cb;
	qm->kvm_source->send_mouse_button = qmach_kvm_send_mouse_button_cb;

	qm->vnc_host = g_strdup("localhost");
	qm->vnc_port = 10;

	qm->description = g_strdup(description);
	name = g_strdup_printf("qmach.0x%04x", g_random_int_range(0, 1024*64));
	qm->tmpdir = tempdir_create(ctxt->tmpdir, name);
	g_free(name);

	qm->command = g_strdup_printf("qemu "
		"-daemonize "
		"-pidfile %s%c" PIDFILE " "
		"-monitor unix:%s%c" MONSOCK ",server,nowait "
		"-vnc %s:%u "
		/* arch specific */
		"-serial unix:%s%c" SER0SOCK ",server,nowait "
		"-cdrom %s -boot d",
		qm->tmpdir, G_DIR_SEPARATOR,
		qm->tmpdir, G_DIR_SEPARATOR,
		qm->vnc_host, qm->vnc_port,
		qm->tmpdir, G_DIR_SEPARATOR,
		"/media/disc2/ISOs/dsl-n-01RC4.iso");
	g_debug("qmach: starting qemu: %s", qm->command);

	qm->vnc_token = g_str_hash(qm->command);

	/* starting qemu process */
	if(g_spawn_command_line_sync(qm->command, &s_stdout, &s_stderr, &status,
		&error)) {
		g_strstrip(s_stdout);
		g_strstrip(s_stderr);
#if 0
		g_debug("qmach: stdout:\n%s\nswitch stderr:\n%s\n",
			s_stdout, s_stderr);
#endif
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
	if(!misc_wait_for_file(name, 5000)) {
		g_warning("qmach:  pid file '%s' not created", name);
		g_free(name);
		qmach_cleanup(qm);
		return NULL;
	}

	/* open monitor socket */
	name = g_strdup_printf("%s%c" MONSOCK, qm->tmpdir, G_DIR_SEPARATOR);
	if(!misc_wait_for_file(name, 5000)) {
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

	qm->widget = qmach_create_widget(qm);
	qm->rackitem = rack_item_new(qm->widget, 2);

	cmdq_add(qm->qmon, "", NULL);
	return qm;
}

void qmach_shutdown(VRackQMach *qm)
{
	gchar *name;

	if(qm == NULL)
		return;

	cmdq_add(qm->qmon, "quit", NULL);
	/* remove sockets/pidfile */
	name = g_strdup_printf("%s%c" PIDFILE, qm->tmpdir, G_DIR_SEPARATOR);
	g_unlink(name);
	g_free(name);
	name = g_strdup_printf("%s%c" MONSOCK, qm->tmpdir, G_DIR_SEPARATOR);
	g_unlink(name);
	g_free(name);
	name = g_strdup_printf("%s%c" SER0SOCK, qm->tmpdir, G_DIR_SEPARATOR);
	g_unlink(name);
	g_free(name);

	tempdir_remove(qm->tmpdir);

	/* clean up */
	qmach_cleanup(qm);
}

VRackKvmSource *qmach_get_kvm_source(VRackQMach *qm)
{
	return qm->kvm_source;
}

/*****************************************************************************/

static GtkWidget *qmach_create_widget(VRackQMach *qm)
{
	GtkWidget *widget;

	widget = gtk_frame_new("QMach");
	gtk_widget_set_tooltip_text(widget, qm->command);
	return widget;
}

static void qmach_cleanup(VRackQMach *qm)
{
	int fd;

	rack_item_free(qm->rackitem);

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

	retval = (strcmp(text + (strlen(text) - 7), "(qemu) ") == 0);
	return retval;
}

#if 0
static void qmach_mon_help_cb(GIOChannel *io, const gchar *text,
	gpointer user_data)
{
	g_debug("qmach_mon_help_cb: %s", text);
}
#endif

/*****************************************************************************/
/* KVM interface                                                             */

static GdkPixbuf *qmach_kvm_get_pixbuf_cb(gpointer opaque)
{
	VRackQMach *qm = opaque;
	VRackKvmSource *vncsrc;

	vncsrc = vnc_get_source(qm->ctxt, qm->vnc_token, qm->vnc_host,
		5900 + qm->vnc_port);
	if(vncsrc)
		return vncsrc->get_pixbuf(vncsrc->opaque);
	return NULL;
}

static gboolean qmach_kvm_get_size_cb(gpointer opaque, guint32 *w, guint32 *h)
{
	VRackQMach *qm = opaque;
	VRackKvmSource *vncsrc;

	vncsrc = vnc_get_source(qm->ctxt, qm->vnc_token, qm->vnc_host,
		5900 + qm->vnc_port);
	if(vncsrc)
		return vncsrc->get_size(vncsrc->opaque, w, h);
	return FALSE;
}

static gboolean qmach_kvm_send_mouse_move_cb(gpointer opaque,
	guint32 x, guint32 y)
{
	/* TODO */
	return FALSE;
}

static gboolean qmach_kvm_send_mouse_button_cb(gpointer opaque,
	guint32 button_mask)
{
	/* TODO */
	return FALSE;
}

