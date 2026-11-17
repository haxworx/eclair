#include "media-fixture.h"
#include "eclair.h"
#include "eclair_window.h"
#include "eclair_scale.h"
#include "eclair_playlist_container.h"
#include "eclair_media_file.h"
#include <assert.h>
static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static void settle(void) { ecore_timer_add(0.7, finish, NULL); ecore_main_loop_begin(); }
static void received(void *data, Evas_Object *obj, const char *emission, const char *source) { (*(int *)data)++; }
static void click(Eclair_Window *window, const char *name)
{
   Evas_Object *part = (Evas_Object *)edje_object_part_object_get(window->edje_object, name);
   int x, y, w, h;
   evas_object_geometry_get(part, &x, &y, &w, &h);
   evas_event_feed_mouse_in(window->evas, 0, NULL);
   evas_event_feed_mouse_move(window->evas, x+w/2, y+h/2, 0, NULL);
   evas_event_feed_mouse_down(window->evas, 1, EVAS_BUTTON_NONE, 0, NULL);
   edje_object_message_signal_process(window->edje_object);
   elm_win_render(window->window);
   settle();
   evas_event_feed_mouse_up(window->evas, 1, EVAS_BUTTON_NONE, 0, NULL);
   edje_object_message_signal_process(window->edje_object);
   settle();
}
int main(int argc, char **argv)
{
   elm_init(argc, argv); eclair_scale_init();
   Eclair app = {0};
   app.config.properties = eina_hash_string_superfast_new(free);
   app.gui_window = eclair_window_create(argv[1], "main", "Gant input", ECLAIR_SOFTWARE, &app, 1);
   assert(app.gui_window);
   settle(); elm_win_render(app.gui_window->window); settle();
   const char *parts[] = {"button_play", "button_open", "button_stop", "button_next", "button_prev"};
   const char *signals[] = {"eclair_play", "eclair_open", "eclair_stop", "eclair_next", "eclair_prev"};
   for (int i=0;i<5;i++) {
      int count=0;
      edje_object_signal_callback_add(app.gui_window->edje_object, signals[i], "*", received, &count);
      click(app.gui_window, parts[i]);
      assert(count == 1);
      edje_object_signal_callback_del_full(app.gui_window->edje_object, signals[i], "*", received, &count);
   }
   app.meta_tag_manager.meta_tag_delete_thread = 1;
   eclair_playlist_init(&app.playlist, &app);
   app.playlist_container_owner = app.gui_window;
   app.playlist_container = eclair_playlist_container_object_add(app.gui_window->evas, &app);
   eclair_playlist_container_set_entry_theme_path(app.playlist_container, argv[1]);
   edje_object_part_swallow(app.gui_window->edje_object, "playlist_container", app.playlist_container);
   eclair_playlist_container_set_media_list(app.playlist_container, &app.playlist.playlist);
   assert(eclair_playlist_add_uri(&app.playlist, media_fixture("First.ogg"), 0, 0));
   assert(eclair_playlist_add_uri(&app.playlist, media_fixture("Second.ogg"), 0, 0));
   eclair_playlist_container_update(app.playlist_container);
   evas_object_show(app.playlist_container);
   edje_object_calc_force(app.gui_window->edje_object);
   settle(); elm_win_render(app.gui_window->window); settle();
   Eclair_Playlist_Container *container = evas_object_smart_data_get(app.playlist_container);
   int x, y, w, h;
   evas_object_geometry_get(app.playlist_container, &x, &y, &w, &h);
   assert(w > 0 && h > container->entry_height);
   /* The Gant swallow must pass input to its track rows. */
   Eclair_Media_File *first = eina_list_nth(app.playlist.playlist, 0);
   Eclair_Media_File *second = eina_list_nth(app.playlist.playlist, 1);
   evas_event_feed_mouse_move(app.gui_window->evas, x + w/2, y + container->entry_height/2, 0, NULL);
   evas_event_feed_mouse_down(app.gui_window->evas, 1, EVAS_BUTTON_NONE, 0, NULL);
   evas_event_feed_mouse_up(app.gui_window->evas, 1, EVAS_BUTTON_NONE, 0, NULL);
   assert(first->selected && !second->selected);
   evas_event_feed_mouse_move(app.gui_window->evas, x + w/2, y + container->entry_height + container->entry_height/2, 0, NULL);
   evas_event_feed_mouse_down(app.gui_window->evas, 1, EVAS_BUTTON_NONE, 0, NULL);
   evas_event_feed_mouse_up(app.gui_window->evas, 1, EVAS_BUTTON_NONE, 0, NULL);
   assert(!first->selected && second->selected);
   eclair_playlist_shutdown(&app.playlist);
   eclair_window_del(app.gui_window); eina_hash_free(app.config.properties); elm_shutdown();
}
