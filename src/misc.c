#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#include <glib.h>

gboolean misc_wait_for_file(const gchar *path, guint32 msec)
{
	gint i;

	for(i = 0; i < msec; i ++) {
		if(g_file_test(path, G_FILE_TEST_EXISTS))
			break;
		g_usleep(1000);
	}
	if(g_file_test(path, G_FILE_TEST_EXISTS))
		return TRUE;
	return FALSE;
}

GIOChannel *misc_connect_to_socket(const gchar *path)
{
	struct sockaddr_un sun;
	int fd, ret;

	sun.sun_family = PF_UNIX;
	g_snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", path);

	fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(fd < 0) {
		g_warning("switch: failed to open socket '%s': %s (%d)",
			path, strerror(errno), errno);
		return NULL;
	}
	ret = connect(fd, (struct sockaddr *)(&sun), sizeof(sun));
	if(ret < 0) {
		g_warning("switch: failed to connect to socket '%s': %s (%d)",
			path, strerror(errno), errno);
		close(fd);
		return NULL;
	}
	return g_io_channel_unix_new(fd);
}

