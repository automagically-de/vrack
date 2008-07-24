#include <glib.h>
#include <glib/gstdio.h>

gchar *tempdir_create(const gchar *base, const gchar *name)
{
	gchar *path;

	if(!g_file_test(base, G_FILE_TEST_IS_DIR))
		return NULL;

	path = g_strdup_printf("%s%c%s", base, G_DIR_SEPARATOR, name);
	if(g_mkdir(path, 0700) == 0) {
		return path;
	} else {
		g_warning("failed to create directory '%s'", path);
		g_free(path);
		return NULL;
	}
}

void tempdir_remove(const gchar *tmpdir)
{
	g_rmdir(tmpdir);
}
