#include "eclair.h"
#include "eclair_media_file.h"
#include "media-fixture.h"
#include <Emotion.h>
#include <assert.h>

int main(void)
{
   ecore_init();
   emotion_init();
   Eclair app = {0};
   app.meta_tag_manager.meta_tag_delete_thread = EINA_TRUE;
   eclair_playlist_init(&app.playlist, &app);
   char *track = media_fixture("Song.MP3");
   char *video = media_fixture("Movie.mp4");
   char *cover = media_fixture("cover.jpg");
   char *text = media_fixture("notes.txt");
   assert(!eclair_playlist_add_uri(&app.playlist, cover, 0, 0));
   assert(!eclair_playlist_add_uri(&app.playlist, text, 0, 0));
   assert(!eclair_playlist_add_uri(&app.playlist, "/does-not-exist.mp3", 0, 0));
   assert(!eclair_playlist_add_uri(&app.playlist, "", 0, 0));
   assert(!app.playlist.playlist && !app.playlist.current);
   assert(eclair_playlist_add_uri(&app.playlist, track, 0, 0));
   assert(eclair_playlist_add_uri(&app.playlist, video, 0, 0));
   assert(eina_list_count(app.playlist.playlist) == 2);
   eclair_playlist_empty(&app.playlist);
   assert(eclair_playlist_add_dir(&app.playlist, fixture_dir, 0, 0));
   assert(eina_list_count(app.playlist.playlist) == 2);
   eclair_playlist_empty(&app.playlist);
   char *list = media_fixture("list.M3U8");
   FILE *file = fopen(list, "w"); assert(file);
   fputs("#EXTM3U\n\nSong.MP3\ncover.jpg\nnotes.txt\nmissing.mp3\nMovie.mp4\n", file);
   fclose(file);
   assert(eclair_playlist_add_uri(&app.playlist, list, 0, 0));
   assert(eina_list_count(app.playlist.playlist) == 2);
   assert(!strcmp(eclair_playlist_current_media_file(&app.playlist)->path, track));
   char uri[1024];
   snprintf(uri, sizeof(uri), "file://localhost%s", track);
   assert(eclair_playlist_add_uri(&app.playlist, uri, 0, 0));
   assert(eina_list_count(app.playlist.playlist) == 3);
   assert(!eclair_playlist_add_uri(&app.playlist, "file://another-host/Music/song.mp3", 0, 0));
   /* Live streams need not have a filename extension. */
   assert(eclair_playlist_add_uri(&app.playlist, "https://example.invalid/radio", 0, 0));
   assert(eina_list_count(app.playlist.playlist) == 4);
   eclair_playlist_shutdown(&app.playlist);
   eina_hash_free(app.config.properties);
   emotion_shutdown();
   ecore_shutdown();
}
