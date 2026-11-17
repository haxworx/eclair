#ifndef ECLAIR_TEST_MEDIA_FIXTURE_H
#define ECLAIR_TEST_MEDIA_FIXTURE_H
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/* UI tests need existing paths; they do not start a decoder. */
static char fixture_dir[] = "/tmp/eclair-media-XXXXXX";
static char fixture_paths[16][512];
static unsigned fixture_count;
static void fixture_cleanup(void)
{
   for (unsigned i = 0; i < fixture_count; i++) unlink(fixture_paths[i]);
   rmdir(fixture_dir);
}
static char *media_fixture(const char *name)
{
   if (!fixture_count) { assert(mkdtemp(fixture_dir)); atexit(fixture_cleanup); }
   assert(fixture_count < 16);
   char *path = fixture_paths[fixture_count++];
   assert(snprintf(path, 512, "%s/%s", fixture_dir, name) < 512);
   FILE *file = fopen(path, "w");
   assert(file); fclose(file);
   return path;
}
#endif
