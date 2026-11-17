#include "media-fixture.h"
#include "eclair.h"
#include "eclair_window.h"
#include "eclair_scale.h"
#include "eclair_media_file.h"
#include <assert.h>
#include <unistd.h>
#include <Ecore_File.h>

static void check_icons(Eclair_Menu *menu)
{
   Eina_List *l;
   Eclair_Menu_Item *item;
   Evas_Object *icon = elm_icon_add(menu->parent_window);
   EINA_LIST_FOREACH(menu->items, l, item) {
      if (item->icon_path && !elm_icon_standard_set(icon, item->icon_path)) { fprintf(stderr, "Missing icon: %s\n", item->icon_path); abort(); }
      if (item->child) check_icons(item->child);
   }
   evas_object_del(icon);
}

int main(int argc, char **argv)
{
   elm_init(argc, argv);
   ecore_file_init();
   eclair_scale_init();
   char directory[] = "/tmp/eclair-preferences-XXXXXX";
   assert(mkdtemp(directory));
   assert(!setenv("XDG_CONFIG_HOME", directory, 1));
   Eclair app = {0};
   assert(eclair_config_init(&app.config));
   char expected[PATH_MAX];
   snprintf(expected, sizeof(expected), "%s/eclair/", directory);
   assert(!strcmp(expected, app.config.config_dir_path));
   assert(ecore_file_is_dir(app.config.covers_dir_path));
   app.meta_tag_manager.meta_tag_delete_thread = 1;
   eclair_playlist_init(&app.playlist, &app);
   /* Keep media, selection and playback state across repeated skin changes. */
   assert(eclair_playlist_add_uri(&app.playlist, media_fixture("theme-switch.ogg"), 0, 0));
   Eina_List *items = app.playlist.playlist;
   Eclair_Media_File *track = items->data;
   track->selected = EINA_TRUE;
   app.playlist.current = items;
   app.state = ECLAIR_PAUSE;
   assert(!eclair_theme_valid(argv[argc - 1]));
   eclair_playlist_set_repeat(&app.playlist, EINA_TRUE);
   eclair_playlist_set_shuffle(&app.playlist, EINA_TRUE);
   for (int i = 0; i < 2 * (argc - 2); i++) {
      const char *path = argv[1 + i % (argc - 2)];
      assert(eclair_theme_valid(path));
      assert(eclair_theme_set(&app, path));
      assert(app.playlist.playlist == items && app.playlist.current == items);
      assert(track->selected && app.state == ECLAIR_PAUSE);
      assert(app.playlist.repeat && app.playlist.shuffle);
      for (int pending = 0; pending < 5; pending++)
         edje_object_message_signal_process(app.gui_window->edje_object);
      assert(!strcmp(edje_object_part_state_get(app.gui_window->edje_object,
                                                "button_repeat", NULL), "enabled"));
      assert(!strcmp(edje_object_part_state_get(app.gui_window->edje_object,
                                                "button_shuffle", NULL), "enabled"));
      check_icons(app.dialogs_manager.menu);
      Eclair_Window *window = app.gui_window;
      assert(!eclair_theme_set(&app, argv[argc - 1]));
      assert(app.gui_window == window);
      elm_win_render(window->window);
   }
   /* Exercise the menu entry and custom chooser rejection before accepting. */
   Eclair_Menu_Item *themes = NULL, *entry;
   Eina_List *node;
   EINA_LIST_FOREACH(app.dialogs_manager.menu->items, node, entry)
      if (!strcmp(entry->label, "Theme")) themes = entry;
   assert(themes);
   Eclair_Menu_Item *custom = eina_list_last_data_get(themes->child->items);
   custom->item_cb(custom, custom->cb_data);
   Eclair_Dialogs_Manager *manager = &app.dialogs_manager;
   assert(manager->file_window && manager->file_action == 3);
   evas_object_smart_callback_call(manager->file_selector, "done", argv[argc - 1]);
   assert(manager->file_window && !manager->theme_job);
   evas_object_smart_callback_call(manager->file_selector, "done", argv[1]);
   assert(!manager->file_window && manager->theme_job);
   /* Shutdown must cancel queued switches cleanly. */
   eclair_dialogs_shutdown(&app.dialogs_manager);
   eclair_playlist_shutdown(&app.playlist);
   eclair_window_del(app.gui_window);
   eclair_window_del(app.playlist_window);
   eclair_window_del(app.equalizer_window);
   eclair_window_del(app.cover_window);
   eclair_config_shutdown(&app.config);
   free(app.gui_theme_file);
   Eclair_Config reloaded;
   assert(eclair_config_init(&reloaded));
   int enabled = 0;
   assert(eclair_config_get_prop_int(&reloaded, "shuffle", "enabled", &enabled) && enabled);
   assert(eclair_config_get_prop_int(&reloaded, "repeat", "enabled", &enabled) && enabled);
   char *theme = NULL;
   assert(eclair_config_get_prop_string(&reloaded, "appearance", "theme", &theme));
   assert(!strcmp(theme, argv[argc - 2]));
   free(theme);
   eclair_config_shutdown(&reloaded);
   ecore_file_recursive_rm(directory);
   elm_shutdown();
   return 0;
}
