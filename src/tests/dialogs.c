#include <assert.h>
#include <unistd.h>

#include <Elementary.h>

#include "eclair.h"
#include "eclair_compat.h"
#include "eclair_scale.h"
#include "eclair_dialogs.h"
#include "eclair_media_file.h"


static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static void settle(double seconds) { ecore_timer_add(seconds, finish, NULL); ecore_main_loop_begin(); }

static unsigned rows(Eclair_Dialogs_Manager *m)
{
   return elm_genlist_items_count(m->playlist_rows);
}
static Evas_Object *action(Evas_Object *box, const char *label)
{
   Eina_List *children = elm_box_children_get(box), *l;
   Evas_Object *button, *found = NULL;
   EINA_LIST_FOREACH(children, l, button)
      if (!strcmp(elm_object_text_get(button), label)) found = button;
   eina_list_free(children);
   assert(found);
   return found;
}
static void check_buttons(Evas_Object *box, int *width)
{
   assert(box && elm_box_homogeneous_get(box));
   double ax, ay;
   elm_box_align_get(box, &ax, &ay);
   assert(ax == 0.5);
   Eina_List *children = elm_box_children_get(box), *l;
   Evas_Object *button;
   assert(eina_list_count(children) == 5);
   EINA_LIST_FOREACH(children, l, button) {
      int w, h; evas_object_geometry_get(button, NULL, NULL, &w, &h);
      assert(w > 0 && h > 0);
      if (!*width) *width = w;
      assert(abs(w - *width) <= 1);
   }
   eina_list_free(children);
}
static void done(Eclair_Dialogs_Manager *m, const char *path)
{
   evas_object_smart_callback_call(m->file_selector, "done", (void *)path);
}
int main(int argc, char **argv)
{
   elm_init(argc, argv);
   eclair_scale_init();
   Eclair app = {0};
   if (argc > 1) app.gui_theme_file = argv[1];
   app.config.properties = eina_hash_string_superfast_new(free);
   app.meta_tag_manager.meta_tag_delete_thread = 1;
   eclair_playlist_init(&app.playlist, &app);
   Eclair_Dialogs_Manager *m = &app.dialogs_manager;
   m->eclair = &app;
   char dir[] = "/tmp/eclair-dialogs-XXXXXX";
   assert(mkdtemp(dir));
   char media[512], playlist[512];
   snprintf(media, sizeof(media), "%s/A very long media filename that should remain readable in the playlist manager.ogg", dir);
   snprintf(playlist, sizeof(playlist), "%s/list.m3u", dir);
   FILE *f = fopen(media, "w"); assert(f); fclose(f);
   eclair_dialogs_open_fc_add_files(m);
   assert(m->file_window && elm_fileselector_multi_select_get(m->file_selector));
   done(m, NULL); assert(!m->file_window);
   eclair_dialogs_open_fc_add_files(m);
   done(m, media); assert(!m->file_window);
   assert(eina_list_count(app.playlist.playlist) == 1);
   eclair_dialogs_open_fc_save_playlist(m);
   assert(elm_fileselector_is_save_get(m->file_selector));
   done(m, playlist); assert(!m->file_window);
   f = fopen(playlist, "r"); assert(f);
   char line[1024]; assert(fgets(line, sizeof(line), f)); fclose(f);
   assert(!strncmp(line, media, strlen(media)));
   eclair_dialogs_open_fc_save_playlist(m);
   done(m, playlist); assert(m->file_window); /* Existing files require Replace. */
   done(m, NULL); assert(!m->file_window);
   eclair_dialogs_open_fc_load_playlist(m);
   done(m, playlist); assert(!m->file_window);
   assert(eina_list_count(app.playlist.playlist) == 2);
   eclair_dialogs_playlist_window(m);
   assert(m->playlist_window && rows(m) == 2);
   assert(!strcmp(elm_object_widget_type_get(m->playlist_rows), "Elm_Genlist"));
   assert(elm_genlist_mode_get(m->playlist_rows) == ELM_LIST_SCROLL);
   settle(0.3);
   elm_win_render(m->playlist_window);
   settle(0.1);
   if (argc > 1) {
      const char *file = NULL;
      edje_object_file_get(elm_layout_edje_get(m->playlist_search), &file, NULL);
      char *expected = eclair_realpath(argv[1]);
      assert(file && expected && !strcmp(file, expected));
      free(expected);
      const char *preview = getenv("ECLAIR_UI_PREVIEW");
      if (preview) {
         Evas *evas = evas_object_evas_get(m->playlist_window);
         Ecore_Evas *canvas = ecore_evas_ecore_evas_get(evas);
         int w, h;
         ecore_evas_geometry_get(canvas, NULL, NULL, &w, &h);
         ecore_evas_manual_render(canvas);
         Evas_Object *image = evas_object_image_add(evas);
         evas_object_image_size_set(image, w, h);
         evas_object_image_data_copy_set(image, (void *)ecore_evas_buffer_pixels_get(canvas));
         assert(evas_object_image_save(image, preview, NULL, NULL));
         evas_object_del(image);
      }
   }
   Evas_Object *toolbar = elm_object_name_find(m->playlist_window, "playlist-toolbar", -1);
   Evas_Object *actions = elm_object_name_find(m->playlist_window, "playlist-actions", -1);
   int button_width = 0;
   check_buttons(toolbar, &button_width);
   check_buttons(actions, &button_width);
   Elm_Object_Item *first = elm_genlist_first_item_get(m->playlist_rows);
   Elm_Object_Item *second = elm_genlist_item_next_get(first);
   Evas_Object *check = elm_object_item_part_content_get(second, "elm.swallow.icon");
   assert(check && !strcmp(elm_object_widget_type_get(check), "Elm_Check"));
   assert(!elm_object_text_get(check) || !*elm_object_text_get(check));
   assert(!elm_object_item_part_content_get(second, "elm.swallow.end"));
   if (argc > 1) {
      int x, y, w, h;
      evas_object_geometry_get(check, &x, &y, &w, &h);
      Evas *evas = evas_object_evas_get(check);
      evas_event_feed_mouse_in(evas, 0, NULL);
      evas_event_feed_mouse_move(evas, x + w / 2, y + h / 2, 0, NULL);
      evas_event_feed_mouse_down(evas, 1, EVAS_BUTTON_NONE, 0, NULL);
      evas_event_feed_mouse_up(evas, 1, EVAS_BUTTON_NONE, 0, NULL);
      settle(0.05);
      assert(elm_check_state_get(check));
   } else {
      elm_check_state_set(check, EINA_TRUE);
      evas_object_smart_callback_call(check, "changed", NULL);
   }
   assert(!((Eclair_Media_File *)eina_list_nth(app.playlist.playlist, 0))->selected);
   assert(((Eclair_Media_File *)eina_list_nth(app.playlist.playlist, 1))->selected);
   /* The polling refresh must preserve item identity after a checkbox change. */
   settle(0.6);
   assert(elm_genlist_first_item_get(m->playlist_rows) == first);
   assert(elm_genlist_item_next_get(first) == second);
   elm_entry_entry_set(m->playlist_search, "no matching media");
   evas_object_smart_callback_call(m->playlist_search, "changed,user", NULL);
   assert(rows(m) == 0);
   elm_entry_entry_set(m->playlist_search, "very long");
   evas_object_smart_callback_call(m->playlist_search, "changed,user", NULL);
   assert(rows(m) == 2);
   evas_object_smart_callback_call(action(actions, "Select All"), "clicked", NULL);
   assert(((Eclair_Media_File *)eina_list_nth(app.playlist.playlist, 0))->selected);
   assert(((Eclair_Media_File *)eina_list_nth(app.playlist.playlist, 1))->selected);
   assert(elm_genlist_first_item_get(m->playlist_rows));
   evas_object_smart_callback_call(action(actions, "Select None"), "clicked", NULL);
   assert(!((Eclair_Media_File *)eina_list_nth(app.playlist.playlist, 0))->selected);
   assert(!((Eclair_Media_File *)eina_list_nth(app.playlist.playlist, 1))->selected);
   ((Eclair_Media_File *)eina_list_nth(app.playlist.playlist, 1))->selected = EINA_TRUE;
   evas_object_smart_callback_call(action(actions, "Remove Selected"), "clicked", NULL);
   assert(eina_list_count(app.playlist.playlist) == 1 && rows(m) == 1);
   if (argc > 1) {
      for (int i = 0; i < 48; i++)
         assert(eclair_playlist_add_uri(&app.playlist, media, EINA_FALSE, EINA_FALSE));
      settle(0.6);
      assert(rows(m) == 49);
      int scroll_y = 0;
      for (int attempt = 0; attempt < 10 && scroll_y == 0; attempt++) {
         elm_win_render(m->playlist_window);
         Elm_Object_Item *last = elm_genlist_last_item_get(m->playlist_rows);
         elm_genlist_item_show(last, ELM_GENLIST_ITEM_SCROLLTO_BOTTOM);
         settle(0.1);
         elm_scroller_region_get(m->playlist_rows, NULL, &scroll_y, NULL, NULL);
      }
      assert(scroll_y > 0);
      /* Layout remains usable after resizing, at every desktop scale. */
      evas_object_resize(m->playlist_window, eclair_scale_pixels(900), eclair_scale_pixels(600));
      settle(0.1);
      elm_win_render(m->playlist_window);
      int width = 0;
      check_buttons(toolbar, &width);
      check_buttons(actions, &width);
   }
   for (int i = 0; i < 10; i++) ecore_main_loop_iterate_may_block(EINA_FALSE);
   evas_object_smart_callback_call(action(actions, "Close"), "clicked", NULL);
   assert(!m->playlist_window && !m->playlist_refresh);
   eclair_dialogs_search_window(m); assert(m->playlist_window);
   evas_object_del(m->playlist_window);
   assert(!eclair_playlist_save(&app.playlist, "/dev/full"));
   eclair_playlist_shutdown(&app.playlist);
   eina_hash_free(app.config.properties);
   unlink(media); unlink(playlist); rmdir(dir);
   elm_shutdown();
   return 0;
}
