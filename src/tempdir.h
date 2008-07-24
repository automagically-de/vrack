#ifndef _TEMPDIR_H
#define _TEMPDIR_H

#include <glib.h>

gchar *tempdir_create(const gchar *base, const gchar *name);
void tempdir_remove(const gchar *tmpdir);

#endif
