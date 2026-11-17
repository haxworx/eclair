#include <Ecore_File.h>
#include "media-fixture.h"
#include "eclair.h"
#include "eclair_window.h"
#include "eclair_scale.h"
#include "eclair_callbacks.h"
#include "eclair_playlist_container.h"
#include "eclair_media_file.h"
#include <assert.h>

static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static void settle(void) { ecore_timer_add(0.1, finish, NULL); ecore_main_loop_begin(); }
static void open_playlist(Eclair_Menu_Item *item, void *data) { eclair_dialogs_playlist_window(data); }

static void click(Evas *canvas, Evas_Object *object)
{
   int x, y, w, h;
   evas_object_geometry_get(object, &x, &y, &w, &h);
   assert(w > 0 && h > 0);
   evas_event_feed_mouse_in(canvas, 0, NULL);
   evas_event_feed_mouse_move(canvas, x + w / 2, y + h / 2, 0, NULL);
   evas_event_feed_mouse_down(canvas, 1, EVAS_BUTTON_NONE, 0, NULL);
   evas_event_feed_mouse_up(canvas, 1, EVAS_BUTTON_NONE, 0, NULL);
   settle();
}
int main(int argc, char **argv)
{
   elm_init(argc, argv);
   eclair_scale_init();
   Eclair app = {0};
   app.config.properties = eina_hash_string_superfast_new(free);
   app.dialogs_manager.eclair = &app;
   app.gui_window = eclair_window_create(argv[1], "main", "Input test", ECLAIR_SOFTWARE, &app, EINA_TRUE);
   assert(app.gui_window);
   eclair_window_add_default_callbacks(app.gui_window, &app);
   settle();
   elm_win_render(app.gui_window->window);
   settle();
   Eclair_Menu *menu = eclair_menu_new(app.gui_window->window);
   Eclair_Menu *child = eclair_menu_new(app.gui_window->window);
   Eclair_Menu_Item *windows = eclair_menu_add_item(menu, "Windows");
   Eclair_Menu_Item *playlist = eclair_menu_add_item(child, "Playlist Manager");
   eclair_menu_attach_to_item(child, windows);
   eclair_menu_item_callback_set(playlist, open_playlist, &app.dialogs_manager);
   app.dialogs_manager.menu = menu;
   evas_event_feed_mouse_in(app.gui_window->evas, 0, NULL);
   int body_x, body_y;
   edje_object_part_geometry_get(app.gui_window->edje_object, "body", &body_x, &body_y, NULL, NULL);
   evas_event_feed_mouse_move(app.gui_window->evas, body_x + eclair_scale_pixels(5), body_y + eclair_scale_pixels(5), 0, NULL);
   evas_event_feed_mouse_down(app.gui_window->evas, 3, EVAS_BUTTON_NONE, 0, NULL);
   evas_event_feed_mouse_up(app.gui_window->evas, 3, EVAS_BUTTON_NONE, 0, NULL);
   settle();
   assert(!menu->widget || !evas_object_visible_get(menu->widget));
   click(app.gui_window->evas, (Evas_Object *)edje_object_part_object_get(app.gui_window->edje_object, "button_menu"));
   assert(menu->widget);
   settle();
   elm_win_render(app.gui_window->window);
   settle();
   Evas_Object *target = windows->view ? windows->view : elm_object_item_track(windows->widget_item);
   int menu_x, menu_y, menu_w, menu_h;
   evas_object_geometry_get(target, &menu_x, &menu_y, &menu_w, &menu_h);
   evas_event_feed_mouse_move(app.gui_window->evas, menu_x + menu_w / 2, menu_y + menu_h / 2, 0, NULL);
   settle();
   if (windows->widget_item) elm_object_item_untrack(windows->widget_item);
   elm_win_render(app.gui_window->window);
   settle();
   target = playlist->view ? playlist->view : elm_object_item_track(playlist->widget_item);
   click(app.gui_window->evas, target);
   if (playlist->widget_item) elm_object_item_untrack(playlist->widget_item);
   assert(app.dialogs_manager.playlist_window);
   evas_object_del(app.dialogs_manager.playlist_window);
   eclair_menu_free(menu);
   app.dialogs_manager.menu = NULL;
   /* Exercise the same URI-list callback used by every display backend. */
   app.meta_tag_manager.meta_tag_delete_thread = 1;
   eclair_playlist_init(&app.playlist, &app);
   app.playlist_container_owner = app.gui_window;
   app.playlist_container = eclair_playlist_container_object_add(app.gui_window->evas, &app);
   eclair_playlist_container_set_entry_theme_path(app.playlist_container, argv[1]);
   eclair_playlist_container_set_media_list(app.playlist_container, &app.playlist.playlist);
   edje_object_part_swallow(app.gui_window->edje_object, "playlist_container", app.playlist_container);
   evas_object_show(app.playlist_container);
   edje_object_calc_force(app.gui_window->edje_object);
   int x, y, w, h;
   evas_object_geometry_get(app.playlist_container, &x, &y, &w, &h);
   assert(w > 0 && h > 0);
   char *first_path = media_fixture("Dropped track.ogg");
   char *second_path = media_fixture("Second.ogg");
   char uris[2048];
   snprintf(uris, sizeof(uris), "# URI list\r\nfile://%s/Dropped%%20track.ogg\r\nfile://%s\r\n", fixture_dir, second_path);
   Elm_Selection_Data drop = {.x = x + w / 2, .y = y + h / 2,
      .format = ELM_SEL_FORMAT_URILIST, .data = uris, .len = strlen(uris)};
   assert(eclair_dnd_drop_cb(app.gui_window, app.gui_window->window, &drop));
   assert(eina_list_count(app.playlist.playlist) == 2);
   assert(!strcmp(((Eclair_Media_File *)eina_list_data_get(app.playlist.playlist))->path, first_path));
   /* Coordinates in the transferred selection may be unset by EFL. Music
    * must work over the player body and over the artwork, not just the list. */
   drop.x = 0x7fffffff; drop.y = -100;
   char *third_path = media_fixture("Third.ogg");
   snprintf(uris, sizeof(uris), "file://localhost%s\r\n", third_path);
   drop.len = strlen(uris);
   app.gui_window->drop_over_cover = EINA_TRUE;
   assert(eclair_dnd_drop_cb(app.gui_window, app.gui_window->window, &drop));
   assert(eina_list_count(app.playlist.playlist) == 3);
   assert(!eclair_playlist_current_media_file(&app.playlist)->cover_path);

   char folder[1024];
   snprintf(folder, sizeof(folder), "%s/Album folder", fixture_dir);
   assert(ecore_file_mkdir(folder));
   char *nested = media_fixture("Album folder/Nested.ogg");
   char *picture = media_fixture("Album folder/cover.jpg");
   char *notes = media_fixture("Album folder/notes.txt");
   snprintf(uris, sizeof(uris), "file://%s/Album%%20folder\r\n", fixture_dir);
   drop.len = strlen(uris);
   assert(eclair_dnd_drop_cb(app.gui_window, app.gui_window->window, &drop));
   assert(eina_list_count(app.playlist.playlist) == 4);

   snprintf(uris, sizeof(uris), "file://%s\r\n", notes);
   drop.len = strlen(uris);
   assert(!eclair_dnd_drop_cb(app.gui_window, app.gui_window->window, &drop));
   assert(eina_list_count(app.playlist.playlist) == 4);

   /* A real image still sets the cover, with URI escaping decoded once. */
   app.cover = evas_object_image_add(app.gui_window->evas);
   app.cover_owner = app.gui_window;
   app.cover_manager.eclair = &app;
   snprintf(app.config.covers_dir_path, PATH_MAX, "%s/", fixture_dir);
   char *image_path = media_fixture("Cover 100%25.png");
   media_fixture("Dropped track.png"); /* Cached cover is cleaned up too. */
   Evas_Object *image = evas_object_image_add(app.gui_window->evas);
   unsigned int pixel = 0xffffffff;
   evas_object_image_size_set(image, 1, 1);
   evas_object_image_data_copy_set(image, &pixel);
   assert(evas_object_image_save(image, image_path, NULL, NULL));
   evas_object_del(image);
   snprintf(uris, sizeof(uris), "file://%s/Cover%%20100%%2525.png\r\n", fixture_dir);
   drop.len = strlen(uris);
   app.gui_window->drop_over_cover = EINA_TRUE;
   assert(eclair_dnd_drop_cb(app.gui_window, app.gui_window->window, &drop));
   assert(eclair_playlist_current_media_file(&app.playlist)->cover_path);
   assert(eina_list_count(app.playlist.playlist) == 4);
   unlink(nested); unlink(picture); unlink(notes); rmdir(folder);
   eclair_playlist_shutdown(&app.playlist);
   eclair_window_del(app.gui_window);
   eina_hash_free(app.config.properties);
   elm_shutdown();
   return 0;
}
