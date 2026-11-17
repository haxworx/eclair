#include "media-fixture.h"
#include "eclair.h"
#include "eclair_window.h"
#include "eclair_callbacks.h"
#include "eclair_scale.h"
#include "eclair_media_file.h"
#include "eclair_playlist_container.h"
#include <assert.h>
#include <unistd.h>

/* Overwriting a downloaded cover must refresh the existing image object. */
static void cover_write(Evas *evas, const char *path, int width)
{
   unsigned int pixels[] = { 0xffff0000, 0xff00ff00, 0xff0000ff };
   Evas_Object *image = evas_object_image_add(evas);
   evas_object_image_size_set(image, width, 1);
   evas_object_image_data_copy_set(image, pixels);
   assert(evas_object_image_save(image, path, NULL, NULL));
   evas_object_del(image);
}

static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static void settle(void) { ecore_timer_add(.1, finish, NULL); ecore_main_loop_begin(); }
static void *complete(void *data)
{
   Eclair *app = data;
   Eclair_Media_File *file = app->playlist.playlist->data;
   /* The main loop is stopped while the worker produces this result. */
   free(file->title);
   file->title = strdup("Worker result");
   eclair_media_file_update_from_worker(app, file);
   return NULL;
}
static void queue(Eclair *app)
{
   Eclair_Media_File *file = app->playlist.playlist->data;
   file->in_meta_tag_process = 1;
   pthread_t worker;
   assert(!pthread_create(&worker, NULL, complete, app));
   assert(!pthread_join(worker, NULL));
   /* The processing flag cannot be released before main-loop delivery. */
   assert(file->in_meta_tag_process);
}
int main(int argc, char **argv)
{
   elm_init(argc, argv); eclair_scale_init();
   Eclair app = {0};
   app.config.properties = eina_hash_string_superfast_new(free);
   app.meta_tag_manager.meta_tag_delete_thread = 1;
   app.gui_window = eclair_window_create(argv[1], "main", "Worker update", ECLAIR_SOFTWARE, &app, 1);
   assert(app.gui_window && eclair_media_updates_init(&app));
   eclair_playlist_init(&app.playlist, &app);
   app.playlist_container_owner = app.gui_window;
   app.playlist_container = eclair_playlist_container_object_add(app.gui_window->evas, &app);
   eclair_playlist_container_set_entry_theme_path(app.playlist_container, argv[1]);
   edje_object_part_swallow(app.gui_window->edje_object, "playlist_container", app.playlist_container);
   eclair_playlist_container_set_media_list(app.playlist_container, &app.playlist.playlist);
   evas_object_show(app.playlist_container);
   assert(eclair_playlist_add_uri(&app.playlist, media_fixture("Worker.ogg"), 0, 0));
   app.playlist.current = app.playlist.playlist;
   Eclair_Media_File *file = app.playlist.playlist->data;
   app.cover_owner = app.gui_window;
   app.cover = evas_object_image_add(app.gui_window->evas);
   edje_object_part_swallow(app.gui_window->edje_object, "cover", app.cover);
   settle();
   file->cover_path = strdup(argv[2]);
   queue(&app);
   const char *path = NULL;
   evas_object_image_file_get(app.cover, &path, NULL);
   assert(!path); /* No UI update in the worker. */
   settle();
   assert(!file->in_meta_tag_process);
   evas_object_image_file_get(app.cover, &path, NULL);
   assert(path && !strcmp(path, argv[2]));
   if (edje_object_part_exists(app.gui_window->edje_object, "record_cover")) {
      Evas_Object *label = edje_object_part_swallow_get(app.gui_window->edje_object, "record_cover");
      assert(label && evas_object_visible_get(label));
      int x, y, w, h, ox, oy;
      edje_object_part_geometry_get(app.gui_window->edje_object, "record_cover", &x, &y, &w, &h);
      evas_object_geometry_get(app.gui_window->edje_object, &ox, &oy, NULL, NULL);
      eclair_dnd_position_cb(app.gui_window, app.gui_window->edje_object,
                             ox+x+w/2, oy+y+h/2, ELM_XDND_ACTION_COPY);
      assert(app.gui_window->drop_over_cover);
      eclair_gui_cover_set(&app, NULL, EINA_TRUE);
      assert(!evas_object_visible_get(label));
      eclair_gui_cover_set(&app, argv[2], EINA_TRUE);
      assert(evas_object_visible_get(label));
   }
   /* Cover changes replace the placeholder without moving the fixed queue. */
   int queue_x, queue_y, queue_w, queue_h;
   edje_object_part_geometry_get(app.gui_window->edje_object, "playlist_container",
                                &queue_x, &queue_y, &queue_w, &queue_h);
   edje_object_message_signal_process(app.gui_window->edje_object);
   assert(!strcmp(edje_object_part_state_get(app.gui_window->edje_object,
                                            "cover_placeholder", NULL), "covered"));
   eclair_gui_cover_set(&app, NULL, EINA_TRUE);
   edje_object_message_signal_process(app.gui_window->edje_object);
   assert(!evas_object_visible_get(app.cover));
   assert(!strcmp(edje_object_part_state_get(app.gui_window->edje_object,
                                            "cover_placeholder", NULL), "default"));
   int qx, qy, qw, qh;
   edje_object_part_geometry_get(app.gui_window->edje_object, "playlist_container", &qx, &qy, &qw, &qh);
   assert(qx == queue_x && qy == queue_y && qw == queue_w && qh == queue_h);
   eclair_gui_cover_set(&app, argv[2], EINA_TRUE);
   assert(evas_object_visible_get(app.cover));
   char downloaded[] = "/tmp/eclair-cover-refresh-XXXXXX.png";
   int fd = eina_file_mkstemp(downloaded, NULL);
   assert(fd >= 0);
   close(fd);
   cover_write(app.gui_window->evas, downloaded, 2);
   eclair_gui_cover_set(&app, downloaded, EINA_TRUE);
   int cover_width, cover_height;
   evas_object_image_size_get(app.cover, &cover_width, &cover_height);
   assert(cover_width == 2 && cover_height == 1);
   cover_write(app.gui_window->evas, downloaded, 3);
   eclair_gui_cover_set(&app, downloaded, EINA_TRUE);
   evas_object_image_size_get(app.cover, &cover_width, &cover_height);
   assert(cover_width == 3 && cover_height == 1);
   unlink(downloaded);
   /* Removing a track before delivery must discard its pending UI refresh. */
   queue(&app);
   file->delete_me = 1;
   ecore_timer_del(app.playlist.media_files_destructor_timer);
   app.playlist.media_files_destructor_timer = NULL;
   app.playlist.current = NULL;
   app.playlist.playlist = eina_list_remove(app.playlist.playlist, file);
   app.playlist.removed_media_files = eina_list_append(app.playlist.removed_media_files, file);
   free(file->cover_path);
   file->cover_path = strdup("/tmp/should-not-be-displayed.png");
   settle();
   assert(!file->in_meta_tag_process);
   evas_object_image_file_get(app.cover, &path, NULL);
   assert(!path || strcmp(path, "/tmp/should-not-be-displayed.png"));
   app.playlist.removed_media_files = eina_list_remove(app.playlist.removed_media_files, file);
   app.playlist.playlist = eina_list_append(app.playlist.playlist, file);
   /* Shutdown discards completions even if their pipe callback never ran. */
   queue(&app);
   eclair_media_updates_shutdown(&app);
   assert(!app.media_updates && !app.media_updates_pipe);
   assert(!file->in_meta_tag_process);
   eclair_playlist_shutdown(&app.playlist);
   eclair_window_del(app.gui_window);
   eina_hash_free(app.config.properties);
   elm_shutdown();
}
