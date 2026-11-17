#include "eclair.h"
#include "eclair_window.h"
#include "eclair_scale.h"
#include "eclair_playlist_container.h"
#include "eclair_media_file.h"
#include <math.h>
#include <Elementary.h>
#include <Edje.h>
#include <stdio.h>

static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return EINA_FALSE; }

int main(int argc, char **argv)
{
   Eclair app = {0};
   Eclair_Window *window;
   Evas_Object *playlist;
   int width, height;
   elm_init(argc, argv);
   eclair_scale_init();
   edje_scale_set(1.0);
   app.config.properties = eina_hash_string_superfast_new(free);
   window = eclair_window_create(argv[1], "main", "Rendering test", ECLAIR_SOFTWARE, &app, EINA_TRUE);
   if (!window) return 1;
   app.gui_window = window;
   playlist = eclair_playlist_container_object_add(window->evas, &app);
   app.playlist_container = playlist;
   app.playlist_container_owner = window;
   eclair_playlist_container_set_entry_theme_path(playlist, argv[1]);
   Eclair_Media_File track = {.path = "/tmp/Sample track.ogg", .length = 215};
   app.playlist.playlist = eina_list_append(NULL, &track);
   eclair_playlist_container_set_media_list(playlist, &app.playlist.playlist);
   edje_object_part_swallow(window->edje_object, "playlist_container", playlist);
   evas_object_show(playlist);
   eclair_window_add_default_callbacks(window, &app);
   for (int i = 0; i < 10; i++) {
      ecore_main_loop_iterate_may_block(EINA_FALSE);
      elm_win_render(window->window);
   }
   ecore_timer_add(0.3, finish, NULL);
   ecore_main_loop_begin();
   evas_object_geometry_get(window->window, NULL, NULL, &width, &height);
   printf("Rendered window %dx%d at scale %.2f\n", width, height, eclair_scale_get());
   int output_w, output_h;
   evas_output_size_get(window->evas, &output_w, &output_h);
   int font_size = 0, row_height = 0, text_height = 0;
   if (!track.container_object) return 2;
   const Evas_Object *row_text = edje_object_part_object_get(track.container_object->text, "playlist_entry_name");
   evas_object_text_font_get(row_text, NULL, &font_size);
   evas_object_geometry_get(row_text, NULL, NULL, NULL, &text_height);
   evas_object_geometry_get(track.container_object->text, NULL, NULL, NULL, &row_height);
   printf("Playlist font %.1fpx in %dpx row (text height %d)\n", font_size * evas_object_scale_get(row_text), row_height, text_height);
   edje_object_part_text_set(window->edje_object, "current_media_name", "Sample track");
   edje_object_calc_force(window->edje_object);
   const Evas_Object *title = edje_object_part_object_get(window->edje_object, "current_media_name");
   int title_size;
   evas_object_text_font_get(title, NULL, &title_size);
   int bad_text = evas_object_text_ellipsis_get(row_text) >= 0.0 || fabs(font_size * evas_object_scale_get(row_text) - 10 * eclair_scale_get()) > 0.1 ||
      row_height < text_height || fabs(title_size * evas_object_scale_get(title) - 11 * eclair_scale_get()) > 0.1;
   int failed = bad_text || !evas_object_visible_get(window->window) || output_w != width || output_h != height ||
      width != eclair_scale_pixels(window->layout_width) || height != eclair_scale_pixels(window->layout_height);
   eclair_window_del(window);
   eina_list_free(app.playlist.playlist);
   eina_hash_free(app.config.properties);
   elm_shutdown();
   return failed;
}
