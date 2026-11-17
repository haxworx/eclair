#include "eclair.h"
#include "eclair_window.h"
#include "eclair_callbacks.h"
#include "eclair_media_file.h"
#include <assert.h>
#include <string.h>

static void flush(Evas_Object *layout)
{
   for (int i = 0; i < 5; i++) edje_object_message_signal_process(layout);
}
static void check(Evas_Object *layout, const char *state)
{
   flush(layout);
   assert(!strcmp(edje_object_part_state_get(layout, "button_repeat", NULL), state));
   assert(!edje_object_part_exists(layout, "button_repeat_label"));
}
static void click(Ecore_Evas *canvas, Evas_Object *layout, const char *part)
{
   int x, y, w, h;
   Evas *evas = ecore_evas_get(canvas);
   edje_object_part_geometry_get(layout, part, &x, &y, &w, &h);
   assert(w > 0 && h > 0);
   evas_event_feed_mouse_in(evas, 0, NULL);
   evas_event_feed_mouse_move(evas, x + w/2, y + h/2, 0, NULL);
   evas_event_feed_mouse_down(evas, 1, EVAS_BUTTON_NONE, 0, NULL);
   evas_event_feed_mouse_up(evas, 1, EVAS_BUTTON_NONE, 0, NULL);
   flush(layout);
}
int main(int argc, char **argv)
{
   ecore_evas_init(); edje_init();
   Ecore_Evas *canvas = ecore_evas_buffer_new(1100, 1100);
   assert(canvas);
   Evas_Object *layout = edje_object_add(ecore_evas_get(canvas));
   Eclair_Window window = {.edje_object = layout};
   Eclair app = {.gui_window = &window};
   app.playlist.eclair = &app;
   Eclair_Media_File a = {0}, b = {0};
   app.playlist.playlist = eina_list_append(NULL, &a);
   app.playlist.playlist = eina_list_append(app.playlist.playlist, &b);
   app.playlist.current = eina_list_last(app.playlist.playlist);
   edje_object_signal_callback_add(layout, "eclair_repeat_toggle", "*", eclair_repeat_cb, &app);
   edje_object_signal_callback_add(layout, "eclair_shuffle_toggle", "*", eclair_shuffle_cb, &app);
   for (int theme = 1; theme < argc; theme++) {
      for (int scale = 2; scale <= 4; scale++) {
         assert(edje_object_file_set(layout, argv[theme], "eclair_main_body"));
         edje_object_scale_set(layout, scale / 2.0);
         int width, height;
         edje_object_size_min_get(layout, &width, &height);
         evas_object_resize(layout, width * scale / 2, height * scale / 2);
         evas_object_show(layout);
         edje_object_calc_force(layout);
         ecore_evas_manual_render(canvas);
         eclair_playlist_set_repeat(&app.playlist, 0);
         check(layout, "default");
         assert(!eclair_playlist_get_next_list(&app.playlist));
         click(canvas, layout, "button_repeat");
         assert(app.playlist.repeat);
         check(layout, "enabled");
         assert(eclair_playlist_get_next_list(&app.playlist) == app.playlist.playlist);
         /* Pointer exit must preserve the latched state. */
         evas_event_feed_mouse_move(ecore_evas_get(canvas), 0, 0, 0, NULL);
         check(layout, "enabled");
         click(canvas, layout, "button_repeat");
         assert(!app.playlist.repeat);
         check(layout, "default");
         /* The menu and theme restoration use these same state signals. */
         eclair_playlist_set_repeat(&app.playlist, 1);
         check(layout, "enabled");
         eclair_playlist_set_shuffle(&app.playlist, 0);
         click(canvas, layout, "button_shuffle");
         assert(app.playlist.shuffle);
         assert(app.playlist.shuffle_list->data == &b);
         flush(layout);
         assert(!strcmp(edje_object_part_state_get(layout, "button_shuffle", NULL), "enabled"));
         check(layout, "enabled"); /* The two modes toggle independently. */
         eclair_playlist_set_repeat(&app.playlist, 0);
         Eina_List *next = eclair_playlist_get_next_list(&app.playlist);
         assert(next && next->data == &a);
         eclair_playlist_current_set_list(&app.playlist, next);
         assert(!eclair_playlist_get_next_list(&app.playlist));
         app.playlist.current = eina_list_last(app.playlist.playlist);
         evas_event_feed_mouse_move(ecore_evas_get(canvas), 0, 0, 0, NULL);
         flush(layout);
         assert(!strcmp(edje_object_part_state_get(layout, "button_shuffle", NULL), "enabled"));
         click(canvas, layout, "button_shuffle");
         assert(!app.playlist.shuffle);
         assert(!strcmp(edje_object_part_state_get(layout, "button_shuffle", NULL), "default"));
         check(layout, "default");
         /* Menu-driven changes update the same visual state. */
         eclair_playlist_set_shuffle(&app.playlist, 1);
         flush(layout);
         assert(!strcmp(edje_object_part_state_get(layout, "button_shuffle", NULL), "enabled"));
         eclair_playlist_set_shuffle(&app.playlist, 0);
         eclair_playlist_set_repeat(&app.playlist, 1);
      }
   }
   app.playlist.playlist = eina_list_remove(app.playlist.playlist, &b);
   app.playlist.current = app.playlist.playlist;
   eclair_playlist_set_shuffle(&app.playlist, 1);
   assert(eclair_playlist_get_next_list(&app.playlist) == app.playlist.playlist);
   eclair_playlist_set_repeat(&app.playlist, 0);
   assert(!eclair_playlist_get_next_list(&app.playlist));
   eina_list_free(app.playlist.shuffle_list);
   eina_list_free(app.playlist.playlist);
   ecore_evas_free(canvas); edje_shutdown(); ecore_evas_shutdown();
}
