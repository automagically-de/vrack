#ifndef _MISC_H
#define _MISC_H

#include <glib.h>

gboolean misc_wait_for_file(const gchar *path, guint32 msec);
GIOChannel *misc_connect_to_socket(const gchar *path);

#endif /* _MISC_H */
