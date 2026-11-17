#include <Elementary.h>
#include <Ecore_File.h>
#include <stdint.h>
#include "eclair_scale.h"
#include "eclair_ui.h"
#include "eclair_media_file.h"
#include "eclair_window.h"
#include "eclair_dialogs.h"
#include "config.h"
#include "eclair.h"
#include "eclair_config.h"
#include "eclair_playlist.h"
#include "eclair_playlist_container.h"
#include "eclair_utils.h"
#include "eclair_menu.h"

static void _eclair_dialogs_menu_on_about(Eclair_Menu_Item *item, void *data);

static void _eclair_dialogs_file_open(Eclair_Dialogs_Manager *manager, int action);
static void _eclair_dialogs_menu_on_invert_selection(Eclair_Menu_Item *item, void *data);
static void _eclair_dialogs_menu_on_remove_unselected(Eclair_Menu_Item *item, void *data);
static void _eclair_dialogs_menu_on_remove_all(Eclair_Menu_Item *item, void *data);
static void _eclair_dialogs_menu_on_playlist_window(Eclair_Menu_Item *item, void *data);

/* Defer skin destruction until the menu/file selector callback returns. */
static void _eclair_dialogs_theme_apply(void *data)
{
   Eclair_Dialogs_Manager *manager = data;
   char *path = manager->pending_theme;
   Eclair *app = manager->eclair;
   manager->theme_job = NULL;
   manager->pending_theme = NULL;
   if (!eclair_theme_set(app, path)) {
      _eclair_dialogs_file_open(manager, 3);
      if (manager->file_error)
         elm_object_text_set(manager->file_error, "Unable to load this player theme. The current theme is still active.");
   }
   free(path);
}

static void _eclair_dialogs_theme_queue(Eclair_Dialogs_Manager *manager, const char *path)
{
   if (manager->theme_job) ecore_job_del(manager->theme_job);
   free(manager->pending_theme);
   manager->pending_theme = strdup(path);
   manager->theme_job = ecore_job_add(_eclair_dialogs_theme_apply, manager);
}

static void _eclair_dialogs_theme_default(Eclair_Menu_Item *item, void *data)
{
   _eclair_dialogs_theme_queue(data, PACKAGE_DATA_DIR "/themes/default.edj");
}
static void _eclair_dialogs_theme_gant(Eclair_Menu_Item *item, void *data)
{
   _eclair_dialogs_theme_queue(data, PACKAGE_DATA_DIR "/themes/gant.edj");
}
static void _eclair_dialogs_theme_nocturne(Eclair_Menu_Item *item, void *data)
{
   _eclair_dialogs_theme_queue(data, PACKAGE_DATA_DIR "/themes/nocturne.edj");
}
static void _eclair_dialogs_theme_folio(Eclair_Menu_Item *item, void *data)
{
   _eclair_dialogs_theme_queue(data, PACKAGE_DATA_DIR "/themes/folio.edj");
}
static void _eclair_dialogs_theme_frequency(Eclair_Menu_Item *item, void *data)
{
   _eclair_dialogs_theme_queue(data, PACKAGE_DATA_DIR "/themes/frequency.edj");
}
static void _eclair_dialogs_theme_voltage(Eclair_Menu_Item *item, void *data)
{
   _eclair_dialogs_theme_queue(data, PACKAGE_DATA_DIR "/themes/voltage.edj");
}
static void _eclair_dialogs_theme_undertow(Eclair_Menu_Item *item, void *data)
{
   _eclair_dialogs_theme_queue(data, PACKAGE_DATA_DIR "/themes/undertow.edj");
}
static void _eclair_dialogs_theme_custom(Eclair_Menu_Item *item, void *data)
{
   _eclair_dialogs_file_open(data, 3);
}

//Initialize dialogs manager
void eclair_dialogs_init(Eclair_Dialogs_Manager *dialogs_manager, Eclair *eclair)
{
   if (!dialogs_manager || !eclair)
      return;

   memset(dialogs_manager, 0, sizeof(*dialogs_manager));
   dialogs_manager->eclair = eclair;

   //Create the menu
   dialogs_manager->menu = eclair_menu_new(eclair->gui_window->window);
   dialogs_manager->menu_playlist_item = eclair_menu_add_item_with_icon(dialogs_manager->menu, "Playlist", "view-list-details");

   Eclair_Menu_Item *themes = eclair_menu_add_item_with_icon(dialogs_manager->menu, "Theme", "preferences-desktop-theme");
   Eclair_Menu *choices = eclair_menu_new(eclair->gui_window->window);
   eclair_menu_attach_to_item(choices, themes);
   Eclair_Menu_Item *choice = eclair_menu_add_item_with_icon(choices, "Default", "preferences-desktop-theme");
   eclair_menu_item_callback_set(choice, _eclair_dialogs_theme_default, dialogs_manager);
   choice = eclair_menu_add_item_with_icon(choices, "Gant", "preferences-desktop-theme");
   eclair_menu_item_callback_set(choice, _eclair_dialogs_theme_gant, dialogs_manager);
   choice = eclair_menu_add_item_with_icon(choices, "Nocturne", "preferences-desktop-theme");
   eclair_menu_item_callback_set(choice, _eclair_dialogs_theme_nocturne, dialogs_manager);
   choice = eclair_menu_add_item_with_icon(choices, "Folio", "preferences-desktop-theme");
   eclair_menu_item_callback_set(choice, _eclair_dialogs_theme_folio, dialogs_manager);
   choice = eclair_menu_add_item_with_icon(choices, "Frequency", "preferences-desktop-theme");
   eclair_menu_item_callback_set(choice, _eclair_dialogs_theme_frequency, dialogs_manager);
   choice = eclair_menu_add_item_with_icon(choices, "Voltage", "preferences-desktop-theme");
   eclair_menu_item_callback_set(choice, _eclair_dialogs_theme_voltage, dialogs_manager);
   choice = eclair_menu_add_item_with_icon(choices, "Undertow", "preferences-desktop-theme");
   eclair_menu_item_callback_set(choice, _eclair_dialogs_theme_undertow, dialogs_manager);
   choice = eclair_menu_add_item_with_icon(choices, "Choose Custom Theme…", "document-open");
   eclair_menu_item_callback_set(choice, _eclair_dialogs_theme_custom, dialogs_manager);

   dialogs_manager->menu_playlist_menu = eclair_menu_new(eclair->gui_window->window);
   Eclair_Menu *playlist = dialogs_manager->menu_playlist_menu;
   Eclair_Menu_Item *editor = eclair_menu_add_item_with_icon(playlist, "Playlist Editor", "view-list-details");
   eclair_menu_item_callback_set(editor, _eclair_dialogs_menu_on_playlist_window, eclair);
   eclair_menu_add_seperator(playlist);
   Eclair_Menu_Item *action = eclair_menu_add_item_with_icon(playlist, "Invert Selection", "edit-select-all");
   eclair_menu_item_callback_set(action, _eclair_dialogs_menu_on_invert_selection, eclair);
   action = eclair_menu_add_item_with_icon(playlist, "Remove Unselected", "list-remove");
   eclair_menu_item_callback_set(action, _eclair_dialogs_menu_on_remove_unselected, eclair);
   action = eclair_menu_add_item_with_icon(playlist, "Remove All", "edit-delete");
   eclair_menu_item_callback_set(action, _eclair_dialogs_menu_on_remove_all, eclair);
   eclair_menu_attach_to_item(playlist, dialogs_manager->menu_playlist_item);
   dialogs_manager->menu_about_item = eclair_menu_add_item_with_icon(dialogs_manager->menu, "About", "help-about");
   eclair_menu_item_callback_set(dialogs_manager->menu_about_item, _eclair_dialogs_menu_on_about, dialogs_manager);
}

//Shutdown dialogs manager
void eclair_dialogs_shutdown(Eclair_Dialogs_Manager *dialogs_manager)
{
   if (!dialogs_manager)
      return;

   if (dialogs_manager->theme_job) ecore_job_del(dialogs_manager->theme_job);
   dialogs_manager->theme_job = NULL;
   free(dialogs_manager->pending_theme);
   dialogs_manager->pending_theme = NULL;
   if (dialogs_manager->file_window) evas_object_del(dialogs_manager->file_window);
   if (dialogs_manager->playlist_window) evas_object_del(dialogs_manager->playlist_window);
   if (dialogs_manager->about_window) evas_object_del(dialogs_manager->about_window);
   eclair_menu_free(dialogs_manager->menu);
}

//Open the file chooser dialog to add files
void eclair_dialogs_open_fc_add_files(Eclair_Dialogs_Manager *dialogs_manager)
{
   _eclair_dialogs_file_open(dialogs_manager, 0);
}

//Open the file chooser dialog to load playlist
void eclair_dialogs_open_fc_load_playlist(Eclair_Dialogs_Manager *dialogs_manager)
{
   _eclair_dialogs_file_open(dialogs_manager, 1);
}

//Open the file chooser dialog to save the playlist
void eclair_dialogs_open_fc_save_playlist(Eclair_Dialogs_Manager *dialogs_manager)
{
   _eclair_dialogs_file_open(dialogs_manager, 2);
}

//Popup the eclair menu 
void eclair_dialogs_popup_menu(Eclair_Dialogs_Manager *dialogs_manager)
{
   if (dialogs_manager)
      eclair_menu_popup(dialogs_manager->menu);
}

//Open the playlist search window
void eclair_dialogs_search_window(Eclair_Dialogs_Manager *dialogs_manager)
{
   eclair_dialogs_playlist_window(dialogs_manager);
   if (dialogs_manager && dialogs_manager->playlist_search)
      elm_object_focus_set(dialogs_manager->playlist_search, EINA_TRUE);
}


//------------------------------
// Menu callbacks
//------------------------------

static void _eclair_dialogs_menu_on_invert_selection(Eclair_Menu_Item *item, void *data)
{
   Eclair *eclair;

   if ((eclair = data))
      eclair_playlist_container_invert_selection(eclair->playlist_container);
}

static void _eclair_dialogs_menu_on_remove_unselected(Eclair_Menu_Item *item, void *data)
{
   Eclair *eclair;

   if ((eclair = data))
      eclair_playlist_remove_unselected_media_files(&eclair->playlist);
}

static void _eclair_dialogs_menu_on_remove_all(Eclair_Menu_Item *item, void *data)
{
   Eclair *eclair;

   if ((eclair = data))
      eclair_playlist_empty(&eclair->playlist);
}

static void _eclair_dialogs_menu_on_playlist_window(Eclair_Menu_Item *item, void *data)
{
   Eclair *eclair = data;
   if (eclair) eclair_dialogs_playlist_window(&eclair->dialogs_manager);
}

/* Elementary windows opened by the player menu. */
static Evas_Object *_eclair_dialogs_window(Eclair_Dialogs_Manager *manager, const char *name, const char *title)
{
   Evas_Object *win = elm_win_util_dialog_add(manager->eclair->gui_window ? manager->eclair->gui_window->window : NULL, name, title);
   if (!win) return NULL;
   elm_win_autodel_set(win, EINA_TRUE);
   double profile = elm_config_scale_get();
   elm_object_scale_set(win, eclair_scale_get() / (profile > 0 ? profile : 1));
   evas_object_resize(win, eclair_scale_pixels(780), eclair_scale_pixels(520));
   return win;
}

static Evas_Object *_eclair_dialogs_box(Evas_Object *parent, Eina_Bool horizontal)
{
   Evas_Object *box = elm_box_add(parent);
   elm_box_horizontal_set(box, horizontal);
   if (horizontal) {
      elm_box_align_set(box, 0.5, 0.5);
      elm_box_homogeneous_set(box, EINA_TRUE);
   }
   elm_box_padding_set(box, eclair_scale_pixels(6), eclair_scale_pixels(6));
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);
   return box;
}

static Evas_Object *_eclair_dialogs_button(Evas_Object *box, const char *label, Evas_Smart_Cb callback, void *data)
{
   Evas_Object *button = elm_button_add(box);
   elm_object_text_set(button, label);
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, button);
   evas_object_smart_callback_add(button, "clicked", callback, data);
   evas_object_show(button);
   return button;
}

static void _eclair_dialogs_about_deleted(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Eclair_Dialogs_Manager *manager = data;
   manager->about_window = NULL;
}

static void _eclair_dialogs_about_close(void *data, Evas_Object *obj, void *event)
{
   evas_object_del(data);
}

void eclair_dialogs_about_window(Eclair_Dialogs_Manager *manager)
{
   if (!manager || !manager->eclair) return;
   if (manager->about_window) { elm_win_activate(manager->about_window); return; }
   Evas_Object *win = _eclair_dialogs_window(manager, "eclair-about", "About Eclair");
   if (!win) return;
   evas_object_size_hint_min_set(win, eclair_scale_pixels(400), eclair_scale_pixels(420));
   manager->about_window = win;
   evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _eclair_dialogs_about_deleted, manager);
   const char *theme = manager->eclair->gui_theme_file;
   eclair_ui_theme_apply(win, theme);
   Evas_Object *box = _eclair_dialogs_box(win, EINA_FALSE);
   if (theme && edje_file_group_exists(theme, "eclair/about")) {
      Evas_Object *frame = elm_layout_add(win);
      elm_layout_file_set(frame, theme, "eclair/about");
      evas_object_name_set(frame, "about-frame");
      elm_object_part_text_set(frame, "version", "VERSION " VERSION "  /  BUILT WITH EFL");
      evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
      elm_win_resize_object_add(win, frame);
      elm_object_part_content_set(frame, "eclair.content", box);
      evas_object_show(frame);
   } else elm_win_resize_object_add(win, box);
   Evas_Object *scroll = elm_scroller_add(win);
   elm_object_style_set(scroll, "eclair-about");
   elm_scroller_policy_set(scroll, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   evas_object_size_hint_weight_set(scroll, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroll, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, scroll);
   Evas_Object *credits = elm_label_add(win);
   evas_object_name_set(credits, "about-credits");
   elm_label_line_wrap_set(credits, ELM_WRAP_WORD);
   evas_object_size_hint_weight_set(credits, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(credits, EVAS_HINT_FILL, 0.0);
   elm_object_text_set(credits,
      "<align=center>A little music. A little character.<br/>"
      "Music and video, powered by Enlightenment Foundation Libraries.<br/><br/>"
      "<b>Simon Treny (MoOm)</b><br/>Original lead developer · simon.treny@free.fr<br/><br/>"
      "<b>vko</b><br/>Original build-system maintainer · vkoeppel@gmail.com<br/><br/>"
      "<b>Alastair Poole</b><br/>Modernisation and maintenance · EFL janitor<br/>netstar@netstar.im<br/><br/>"
      "Copyright © 2005 Simon TRENY and various contributors.<br/>"
      "Copyright © 2026 Alastair Poole.<br/>"
      "With thanks to everyone who has contributed to Eclair.<br/><br/>"
      "Released under the permissive license in COPYING.<br/>"
      "Eclair " VERSION "</align>");
   elm_object_content_set(scroll, credits);
   evas_object_show(credits);
   evas_object_show(scroll);
   Evas_Object *close = _eclair_dialogs_button(box, "Close", _eclair_dialogs_about_close, win);
   evas_object_name_set(close, "about-close");
   evas_object_resize(win, eclair_scale_pixels(480), eclair_scale_pixels(600));
   evas_object_show(win);
}

static void _eclair_dialogs_menu_on_about(Eclair_Menu_Item *item, void *data)
{
   eclair_dialogs_about_window(data);
}

static void _eclair_dialogs_file_deleted(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Eclair_Dialogs_Manager *manager = data;
   manager->file_window = manager->file_selector = manager->file_error = NULL;
}

static Eina_Bool _eclair_dialogs_save(Eclair_Dialogs_Manager *manager, const char *path)
{
   if (!eclair_playlist_save(&manager->eclair->playlist, path)) {
      elm_object_text_set(manager->file_error, "Unable to save the playlist. Check the destination and permissions.");
      return EINA_FALSE;
   }
   evas_object_del(manager->file_window);
   return EINA_TRUE;
}

static void _eclair_dialogs_popup_cancel(void *data, Evas_Object *obj, void *event)
{
   evas_object_del(data);
}

static void _eclair_dialogs_saved_path_free(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   free(evas_object_data_get(obj, "save_path"));
}

static void _eclair_dialogs_overwrite(void *data, Evas_Object *obj, void *event)
{
   Evas_Object *popup = data;
   Eclair_Dialogs_Manager *manager = evas_object_data_get(popup, "manager");
   if (!_eclair_dialogs_save(manager, evas_object_data_get(popup, "save_path")))
      evas_object_del(popup);
}

static void _eclair_dialogs_file_done(void *data, Evas_Object *selector, void *event)
{
   Eclair_Dialogs_Manager *manager = data;
   const char *path = event;
   if (!path) { evas_object_del(manager->file_window); return; }
   if (manager->file_action == 3) {
      if (!eclair_theme_valid(path)) {
         elm_object_text_set(manager->file_error, "Choose a player .edj with eclair_main_body and eclair_playlist_entry groups.");
         return;
      }
      _eclair_dialogs_theme_queue(manager, path);
      evas_object_del(manager->file_window);
      return;
   }
   if (manager->file_action == 2) {
      if (ecore_file_exists(path)) {
         Evas_Object *popup = elm_popup_add(manager->file_window);
         elm_object_part_text_set(popup, "title,text", "Replace playlist?");
         elm_object_text_set(popup, "This file already exists. Replace it with the current playlist?");
         evas_object_data_set(popup, "manager", manager);
         evas_object_data_set(popup, "save_path", strdup(path));
         evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, _eclair_dialogs_saved_path_free, NULL);
         Evas_Object *replace = elm_button_add(popup), *cancel = elm_button_add(popup);
         elm_object_text_set(replace, "Replace");
         elm_object_text_set(cancel, "Cancel");
         evas_object_smart_callback_add(replace, "clicked", _eclair_dialogs_overwrite, popup);
         evas_object_smart_callback_add(cancel, "clicked", _eclair_dialogs_popup_cancel, popup);
         elm_object_part_content_set(popup, "button1", replace);
         elm_object_part_content_set(popup, "button2", cancel);
         evas_object_show(popup);
      } else _eclair_dialogs_save(manager, path);
      return;
   }
   if (manager->file_action == 1) {
      if (ecore_file_is_dir(path) || !eclair_playlist_add_m3u(&manager->eclair->playlist, (char *)path, EINA_TRUE, EINA_FALSE)) {
         elm_object_text_set(manager->file_error, "Unable to read this M3U playlist. Choose a readable playlist file.");
         return;
      }
   } else {
      const Eina_List *paths = elm_fileselector_selected_paths_get(selector), *l;
      const char *selected;
      if (paths) {
         EINA_LIST_FOREACH(paths, l, selected)
            eclair_playlist_add_uri(&manager->eclair->playlist, (char *)selected, EINA_FALSE, EINA_FALSE);
      } else eclair_playlist_add_uri(&manager->eclair->playlist, (char *)path, EINA_FALSE, EINA_FALSE);
      eclair_playlist_container_update(manager->eclair->playlist_container);
   }
   manager->playlist_signature = 0;
   evas_object_del(manager->file_window);
}

static void _eclair_dialogs_file_open(Eclair_Dialogs_Manager *manager, int action)
{
   if (!manager || !manager->eclair) return;
   if (manager->file_window) evas_object_del(manager->file_window);
   const char *titles[] = {"Add media files", "Load M3U playlist", "Save M3U playlist", "Choose a custom player theme"};
   Evas_Object *win = _eclair_dialogs_window(manager, "eclair-files", titles[action]);
   if (!win) return;
   manager->file_window = win;
   manager->file_action = action;
   evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _eclair_dialogs_file_deleted, manager);
   Evas_Object *box = _eclair_dialogs_box(win, EINA_FALSE);
   elm_win_resize_object_add(win, box);
   Evas_Object *selector = elm_fileselector_add(win);
   manager->file_selector = selector;
   elm_fileselector_is_save_set(selector, action == 2);
   elm_fileselector_multi_select_set(selector, action == 0);
   const char *home_dir = getenv("HOME");
   elm_fileselector_path_set(selector, home_dir ? home_dir : "/");
   if (action == 2) elm_fileselector_current_name_set(selector, "playlist.m3u");
   evas_object_size_hint_weight_set(selector, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(selector, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, selector);
   evas_object_smart_callback_add(selector, "done", _eclair_dialogs_file_done, manager);
   evas_object_show(selector);
   manager->file_error = elm_label_add(win);
   elm_box_pack_end(box, manager->file_error);
   evas_object_show(manager->file_error);
   evas_object_resize(win, eclair_scale_pixels(780), eclair_scale_pixels(520));
   evas_object_show(win);
}

typedef struct {
   Eclair_Dialogs_Manager *manager;
   Eclair_Media_File *track;
   char *path;
   Eina_Bool selected;
   Elm_Object_Item *item;
} Eclair_Playlist_Row;

/* Validate identity against the live playlist before touching asynchronous rows. */
static Eclair_Media_File *_eclair_dialogs_track(Eclair_Playlist_Row *row)
{
   Eina_List *l;
   Eclair_Media_File *file;
   EINA_LIST_FOREACH(row->manager->eclair->playlist.playlist, l, file)
      if (file == row->track && file->path && !strcmp(file->path, row->path)) return file;
   return NULL;
}

static char *_eclair_dialogs_row_text(void *data, Evas_Object *obj, const char *part)
{
   Eclair_Playlist_Row *row = data;
   Eclair_Media_File *track = _eclair_dialogs_track(row);
   if (!track || strcmp(part, "elm.text")) return NULL;
   char *title = eclair_utils_mediafile_to_artist_title_string(track);
   return title ? title : strdup(track->path ? track->path : "Media");
}

static void _eclair_dialogs_row_selected(void *data, Evas_Object *obj, void *event)
{
   Eclair_Playlist_Row *row = data;
   Eclair_Media_File *track = _eclair_dialogs_track(row);
   if (track) {
      row->selected = track->selected = elm_check_state_get(obj);
      elm_genlist_item_fields_update(row->item, NULL, ELM_GENLIST_ITEM_FIELD_STATE);
      eclair_playlist_container_update(row->manager->eclair->playlist_container);
   }
}

static Eina_Bool _eclair_dialogs_row_state(void *data, Evas_Object *obj, const char *part)
{
   Eclair_Playlist_Row *row = data;
   Eclair_Media_File *track = _eclair_dialogs_track(row);
   return track && !strcmp(part, "chosen") && track->selected;
}

static Evas_Object *_eclair_dialogs_row_content(void *data, Evas_Object *obj, const char *part)
{
   Eclair_Playlist_Row *row = data;
   Eclair_Media_File *track = _eclair_dialogs_track(row);
   if (!track || strcmp(part, "elm.swallow.icon")) return NULL;
   Evas_Object *check = elm_check_add(obj);
   elm_check_state_set(check, track->selected);
   elm_object_access_info_set(check, "Select track");
   evas_object_propagate_events_set(check, EINA_FALSE);
   evas_object_smart_callback_add(check, "changed", _eclair_dialogs_row_selected, row);
   return check;
}

static void _eclair_dialogs_row_free(void *data, Evas_Object *obj)
{
   Eclair_Playlist_Row *row = data;
   free(row->path);
   free(row);
}

static Eina_Bool _eclair_dialogs_contains(const char *text, const char *query)
{
   if (!query || !*query) return EINA_TRUE;
   if (!text) return EINA_FALSE;
   for (; *text; text++) if (!strncasecmp(text, query, strlen(query))) return EINA_TRUE;
   return EINA_FALSE;
}

static unsigned long _eclair_dialogs_hash(unsigned long hash, const char *text)
{
   if (text) for (; *text; text++) hash = hash * 33 + (unsigned char)*text;
   return hash;
}

static Eina_Bool _eclair_dialogs_playlist_refresh(void *data)
{
   Eclair_Dialogs_Manager *manager = data;
   Eclair *app = manager->eclair;
   Eina_List *l;
   Eclair_Media_File *track;
   unsigned long signature = 5381;
   signature = _eclair_dialogs_hash(signature, elm_entry_entry_get(manager->playlist_search));
   EINA_LIST_FOREACH(app->playlist.playlist, l, track) {
      signature = _eclair_dialogs_hash(signature, track->path);
      signature = _eclair_dialogs_hash(signature, track->artist);
      signature = _eclair_dialogs_hash(signature, track->title);
      signature = signature * 33 + (uintptr_t)track;
   }
   Eina_Bool rebuild = signature != manager->playlist_signature;
   manager->playlist_signature = signature;
   Elm_Genlist_Item_Class *item_class = NULL;
   if (rebuild) {
      elm_genlist_clear(manager->playlist_rows);
      item_class = elm_genlist_item_class_new();
      item_class->item_style = "default";
      item_class->func.text_get = _eclair_dialogs_row_text;
      item_class->func.content_get = _eclair_dialogs_row_content;
      item_class->func.state_get = _eclair_dialogs_row_state;
      item_class->func.del = _eclair_dialogs_row_free;
   } else {
      /* Updating selection must not rebuild the list or jump its scroll position. */
      for (Elm_Object_Item *item = elm_genlist_first_item_get(manager->playlist_rows);
           item; item = elm_genlist_item_next_get(item)) {
         Eclair_Playlist_Row *row = elm_object_item_data_get(item);
         track = _eclair_dialogs_track(row);
         if (track && row->selected != track->selected) {
            row->selected = track->selected;
            elm_genlist_item_fields_update(item, "elm.swallow.icon", ELM_GENLIST_ITEM_FIELD_CONTENT);
            elm_genlist_item_fields_update(item, NULL, ELM_GENLIST_ITEM_FIELD_STATE);
         }
      }
   }
   char *query = elm_entry_markup_to_utf8(elm_entry_entry_get(manager->playlist_search));
   unsigned int visible = 0, selected = 0, total = 0;
   EINA_LIST_FOREACH(app->playlist.playlist, l, track) {
      total++;
      if (track->selected) selected++;
      char *title = eclair_utils_mediafile_to_artist_title_string(track);
      const char *name = title ? title : track->path;
      if (!_eclair_dialogs_contains(name, query) && !_eclair_dialogs_contains(track->path, query)) { free(title); continue; }
      visible++;
      if (rebuild) {
         Eclair_Playlist_Row *row = calloc(1, sizeof(*row));
         row->manager = manager;
         row->track = track;
         row->path = strdup(track->path ? track->path : "");
         row->selected = track->selected;
         Elm_Object_Item *item = elm_genlist_item_append(manager->playlist_rows, item_class,
            row, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
         row->item = item;
         elm_object_item_tooltip_text_set(item, row->path);
      }
      free(title);
   }
   if (item_class) elm_genlist_item_class_free(item_class);
   char status[128];
   snprintf(status, sizeof(status), "%u of %u tracks shown, %u selected", visible, total, selected);
   elm_object_text_set(manager->playlist_status, status);
   free(query);
   return ECORE_CALLBACK_RENEW;
}

static void _eclair_dialogs_query_changed(void *data, Evas_Object *obj, void *event)
{
   Eclair_Dialogs_Manager *manager = data;
   manager->playlist_signature = 0;
   _eclair_dialogs_playlist_refresh(manager);
}

static void _eclair_dialogs_playlist_deleted(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Eclair_Dialogs_Manager *manager = data;
   if (manager->playlist_refresh) ecore_timer_del(manager->playlist_refresh);
   manager->playlist_refresh = NULL;
   manager->playlist_window = manager->playlist_rows = manager->playlist_search = manager->playlist_status = NULL;
}

static void _eclair_dialogs_native_add(void *data, Evas_Object *obj, void *event) { eclair_dialogs_open_fc_add_files(data); }
static void _eclair_dialogs_native_load(void *data, Evas_Object *obj, void *event) { eclair_dialogs_open_fc_load_playlist(data); }
static void _eclair_dialogs_native_save(void *data, Evas_Object *obj, void *event) { eclair_dialogs_open_fc_save_playlist(data); }
static void _eclair_dialogs_native_remove(void *data, Evas_Object *obj, void *event)
{
   Eclair_Dialogs_Manager *manager = data;
   eclair_playlist_remove_selected_media_files(&manager->eclair->playlist);
   manager->playlist_signature = 0;
   _eclair_dialogs_playlist_refresh(manager);
}
static void _eclair_dialogs_native_select(void *data, Evas_Object *obj, void *event)
{
   Eclair_Dialogs_Manager *manager = data;
   Eina_List *l;
   Eclair_Media_File *track;
   EINA_LIST_FOREACH(manager->eclair->playlist.playlist, l, track) track->selected = EINA_TRUE;
   eclair_playlist_container_update(manager->eclair->playlist_container);
   _eclair_dialogs_playlist_refresh(manager);
}
static void _eclair_dialogs_native_clear(void *data, Evas_Object *obj, void *event)
{
   Eclair_Dialogs_Manager *manager = data;
   Eina_List *l;
   Eclair_Media_File *track;
   EINA_LIST_FOREACH(manager->eclair->playlist.playlist, l, track) track->selected = EINA_FALSE;
   eclair_playlist_container_update(manager->eclair->playlist_container);
   _eclair_dialogs_playlist_refresh(manager);
}
static void _eclair_dialogs_native_play_selected(void *data, Evas_Object *obj, void *event)
{
   Eclair_Dialogs_Manager *manager = data;
   Eina_List *l;
   Eclair_Media_File *track;
   EINA_LIST_FOREACH(manager->eclair->playlist.playlist, l, track) {
      if (!track->selected) continue;
      eclair_playlist_current_set(&manager->eclair->playlist, track);
      eclair_play_current(manager->eclair);
      return;
   }
}
static void _eclair_dialogs_native_close(void *data, Evas_Object *obj, void *event)
{
   Eclair_Dialogs_Manager *manager = data;
   evas_object_del(manager->playlist_window);
}
static void _eclair_dialogs_native_pause(void *data, Evas_Object *obj, void *event) { eclair_pause(((Eclair_Dialogs_Manager *)data)->eclair); }
static void _eclair_dialogs_native_stop(void *data, Evas_Object *obj, void *event) { eclair_stop(((Eclair_Dialogs_Manager *)data)->eclair); }

void eclair_dialogs_playlist_window(Eclair_Dialogs_Manager *manager)
{
   if (!manager || !manager->eclair) return;
   if (manager->playlist_window) { elm_win_activate(manager->playlist_window); return; }
   Evas_Object *win = _eclair_dialogs_window(manager, "eclair-playlist", "Eclair Playlist Manager");
   if (!win) return;
   /* Keep native decorations: Enlightenment supplies its active theme's
    * compositor shadow (including Flat) around this opaque editor window. */
   elm_win_borderless_set(win, EINA_FALSE);
   elm_win_alpha_set(win, EINA_FALSE);
   const char *theme = manager->eclair->gui_theme_file;
   eclair_ui_theme_apply(win, theme);
   manager->playlist_window = win;
   evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _eclair_dialogs_playlist_deleted, manager);
   Evas_Object *box = _eclair_dialogs_box(win, EINA_FALSE);
   if (theme && edje_file_group_exists(theme, "eclair/editor")) {
      Evas_Object *frame = elm_layout_add(win);
      elm_layout_file_set(frame, theme, "eclair/editor");
      evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
      elm_win_resize_object_add(win, frame);
      elm_object_part_content_set(frame, "eclair.content", box);
      evas_object_show(frame);
   } else elm_win_resize_object_add(win, box);
   Evas_Object *toolbar = _eclair_dialogs_box(box, EINA_TRUE);
   evas_object_name_set(toolbar, "playlist-toolbar");
   evas_object_size_hint_weight_set(toolbar, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(box, toolbar);
   _eclair_dialogs_button(toolbar, "Add Files", _eclair_dialogs_native_add, manager);
   _eclair_dialogs_button(toolbar, "Load Playlist", _eclair_dialogs_native_load, manager);
   _eclair_dialogs_button(toolbar, "Save Playlist", _eclair_dialogs_native_save, manager);
   _eclair_dialogs_button(toolbar, "Pause / Resume", _eclair_dialogs_native_pause, manager);
   _eclair_dialogs_button(toolbar, "Stop", _eclair_dialogs_native_stop, manager);
   manager->playlist_search = elm_entry_add(win);
   elm_entry_single_line_set(manager->playlist_search, EINA_TRUE);
   elm_object_part_text_set(manager->playlist_search, "guide", "Search playlist titles or paths");
   evas_object_size_hint_weight_set(manager->playlist_search, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(manager->playlist_search, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, manager->playlist_search);
   evas_object_smart_callback_add(manager->playlist_search, "changed,user", _eclair_dialogs_query_changed, manager);
   evas_object_show(manager->playlist_search);
   manager->playlist_rows = elm_genlist_add(win);
   elm_genlist_mode_set(manager->playlist_rows, ELM_LIST_SCROLL);
   elm_genlist_select_mode_set(manager->playlist_rows, ELM_OBJECT_SELECT_MODE_NONE);
   elm_scroller_policy_set(manager->playlist_rows, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_AUTO);
   evas_object_size_hint_weight_set(manager->playlist_rows, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(manager->playlist_rows, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, manager->playlist_rows);
   evas_object_show(manager->playlist_rows);
   Evas_Object *actions = _eclair_dialogs_box(box, EINA_TRUE);
   evas_object_name_set(actions, "playlist-actions");
   evas_object_size_hint_weight_set(actions, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(box, actions);
   _eclair_dialogs_button(actions, "Select All", _eclair_dialogs_native_select, manager);
   _eclair_dialogs_button(actions, "Select None", _eclair_dialogs_native_clear, manager);
   _eclair_dialogs_button(actions, "Remove Selected", _eclair_dialogs_native_remove, manager);
   _eclair_dialogs_button(actions, "Play Selected", _eclair_dialogs_native_play_selected, manager);
   _eclair_dialogs_button(actions, "Close", _eclair_dialogs_native_close, manager);
   manager->playlist_status = elm_label_add(win);
   elm_box_pack_end(box, manager->playlist_status);
   evas_object_show(manager->playlist_status);
   manager->playlist_signature = 0;
   _eclair_dialogs_playlist_refresh(manager);
   manager->playlist_refresh = ecore_timer_add(0.5, _eclair_dialogs_playlist_refresh, manager);
   evas_object_resize(win, eclair_scale_pixels(780), eclair_scale_pixels(520));
   evas_object_show(win);
}
