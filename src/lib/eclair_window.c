#include "eclair_scale.h"
#include "eclair_ui.h"
#include "eclair_menu.h"
#include "eclair_window.h"
#include "eclair_turntable.h"
#include "eclair.h"
#include "eclair_callbacks.h"
#include "eclair_playlist_container.h"
#include "eclair_config.h"
typedef struct {
   const char *track, *thumb, *confine;
   Eina_Bool horizontal;
} Eclair_Slider;

static const Eclair_Slider sliders[] = {
   {"progress_bar_track", "progress_bar_drag", "progress_bar_confine", EINA_TRUE},
   {"volume_bar_track", "volume_bar_drag", "volume_bar_confine", EINA_TRUE},
   {"playlist_scrollbar_track", "playlist_scrollbar_drag", "playlist_scrollbar_confine", EINA_FALSE}
};

static void _eclair_window_slider_position(Eclair_Window *window, Evas_Object *track, int px, int py)
{
   const Eclair_Slider *slider = evas_object_data_get(track, "eclair_slider");
   int x, y, w, h, tw, th, ex, ey;
   evas_object_geometry_get(window->edje_object, &ex, &ey, NULL, NULL);
   edje_object_part_geometry_get(window->edje_object, slider->confine, &x, &y, &w, &h);
   edje_object_part_geometry_get(window->edje_object, slider->thumb, NULL, NULL, &tw, &th);
   int travel = slider->horizontal ? w - tw : h - th;
   if (travel <= 0) return;
   double value = slider->horizontal ? (px - ex - x - tw / 2.0) / travel
                                    : (py - ey - y - th / 2.0) / travel;
   if (value < 0.0) value = 0.0;
   if (value > 1.0) value = 1.0;
   edje_object_part_drag_value_set(window->edje_object, slider->thumb,
                                   slider->horizontal ? value : 0.0,
                                   slider->horizontal ? 0.0 : value);
   if (slider == &sliders[0]) eclair_progress_rate_set(window->eclair, value);
   else if (slider == &sliders[1]) eclair_audio_level_set(window->eclair, value);
   else eclair_playlist_container_scroll_percent_set(window->eclair->playlist_container, value);
}

static void _eclair_window_slider_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Down *ev = event;
   Eclair_Window *window = data;
   if (ev->button != 1) return;
   if (evas_object_data_get(obj, "eclair_slider") == &sliders[0])
      window->eclair->use_progress_bar_drag_for_time = EINA_TRUE;
   _eclair_window_slider_position(window, obj, ev->canvas.x, ev->canvas.y);
}

static void _eclair_window_slider_move(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Move *ev = event;
   if (ev->buttons & 1)
      _eclair_window_slider_position(data, obj, ev->cur.canvas.x, ev->cur.canvas.y);
}

static void _eclair_window_slider_up(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Up *ev = event;
   Eclair_Window *window = data;
   if (ev->button != 1) return;
   _eclair_window_slider_position(window, obj, ev->canvas.x, ev->canvas.y);
   if (evas_object_data_get(obj, "eclair_slider") == &sliders[0])
      window->eclair->use_progress_bar_drag_for_time = EINA_FALSE;
}

static void _eclair_window_move_start_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Eclair_Window *window = data;
   Evas_Event_Mouse_Down *event = event_info;
   if (event->button != 1) return;
   elm_win_move_resize_start(window->window, ELM_WIN_MOVE_RESIZE_MOVE);
}

static void _eclair_window_resize_start_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source);
static void _eclair_window_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _eclair_window_close_request_cb(void *data, Evas_Object *obj, void *event);

//Create a new window from the group "eclair_"window_name"_body" of the file "edje_file" with the engine "engine" and the title "title"
//"window_name" is also used to find some props of the window in the edje file ("data" section)
//Return NULL if failed. A created window has to be deleted by eclair_window_del()
Eclair_Window *eclair_window_create(const char *edje_file, const char *window_name, const char *title, Eclair_Engine engine, Eclair *eclair, Eina_Bool main_window)
{
   Eclair_Window *new_window;
   char *prop_name, *prop_value, *config_name, *edje_group;
   int window_w, window_h, opened;

   if (!edje_file || !window_name || !eclair)
      return NULL;

   new_window = calloc(1, sizeof(Eclair_Window));
   new_window->eclair = eclair;
   new_window->borderless = 1;
   new_window->shaped = 1;
   new_window->resizable = 1;
   new_window->main_window = main_window;
   new_window->window_name = strdup(window_name);

   elm_config_accel_preference_set(engine == ECLAIR_GL ? "gl" : "none");
   new_window->window = elm_win_add(NULL, window_name, ELM_WIN_BASIC);
   eclair_ui_theme_source_set(new_window->window, edje_file);
   if (!new_window->window) {
      free(new_window->window_name);
      free(new_window);
      return NULL;
   }
   if (title) elm_win_title_set(new_window->window, title);
   new_window->evas = evas_object_evas_get(new_window->window);

   new_window->edje_object = eclair_scale_edje_add(new_window->evas);
   edje_group = malloc(strlen("eclair_") + strlen(window_name) + strlen("_body") + 1);
   sprintf(edje_group, "eclair_%s_body", window_name);
   if (!edje_object_file_set(new_window->edje_object, edje_file, edje_group))
   {
      evas_object_del(new_window->edje_object);
      evas_object_del(new_window->window);
      free(new_window->window_name);
      free(new_window);
      free(edje_group);
      return NULL;
   }
   free(edje_group);
   eclair_turntable_attach(new_window->edje_object);
   evas_object_data_set(new_window->window, "eclair-player-layout", new_window->edje_object);
   /* The target must be an ancestor of the painted canvas objects. A window
    * widget is not necessarily in their Evas smart-parent chain. */
   elm_drop_target_add(new_window->edje_object, ELM_SEL_FORMAT_URILIST,
      eclair_dnd_enter_cb, new_window, NULL, NULL,
      eclair_dnd_position_cb, new_window, eclair_dnd_drop_cb, new_window);
   for (unsigned int i = 0; i < sizeof(sliders) / sizeof(sliders[0]); i++) {
      Evas_Object *track = (Evas_Object *)edje_object_part_object_get(new_window->edje_object, sliders[i].track);
      if (!track) continue;
      evas_object_data_set(track, "eclair_slider", &sliders[i]);
      evas_object_event_callback_add(track, EVAS_CALLBACK_MOUSE_DOWN, _eclair_window_slider_down, new_window);
      evas_object_event_callback_add(track, EVAS_CALLBACK_MOUSE_MOVE, _eclair_window_slider_move, new_window);
      evas_object_event_callback_add(track, EVAS_CALLBACK_MOUSE_UP, _eclair_window_slider_up, new_window);
   }
   /* Decorative surfaces initiate a compositor-managed move; controls keep
    * their normal actions above these parts. */
   const char *move_parts[] = {"body", "window_button_bar", "body_top_left",
      "body_top_right", "body_top", "body_bottom_left", "body_bottom_right",
      "body_bottom", "body_left", "body_right", "left_side",
      "player_surface", "playlist_background", NULL};
   for (int i = 0; move_parts[i]; i++) {
      Evas_Object *part = (Evas_Object *)edje_object_part_object_get(new_window->edje_object, move_parts[i]);
      if (!part) continue;
      evas_object_pass_events_set(part, EINA_FALSE);
      /* Pixel-alpha hit testing applies to raster images. Evas vector
       * objects have no image pixels to test and would reject every click. */
      evas_object_precise_is_inside_set(part,
         strcmp(evas_object_type_get(part), "vectors") != 0);
      evas_object_event_callback_add(part, EVAS_CALLBACK_MOUSE_DOWN,
                                     _eclair_window_move_start_cb, new_window);
   }
   evas_object_move(new_window->edje_object, 0, 0);
   evas_object_size_hint_weight_set(new_window->edje_object, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(new_window->edje_object, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(new_window->edje_object);

   //Read window props
   if (new_window->window_name)
   {
      prop_name = malloc(strlen(new_window->window_name) + strlen("_borderless") + 1);
      sprintf(prop_name, "%s_borderless", new_window->window_name);
      if ((prop_value = edje_file_data_get(edje_file, prop_name)) && strcmp(prop_value, "0") == 0)
      {
         new_window->borderless = 0;
         free(prop_value);
      }
      free(prop_name);

      prop_name = malloc(strlen(new_window->window_name) + strlen("_shaped") + 1);
      sprintf(prop_name, "%s_shaped", new_window->window_name);
      if ((prop_value = edje_file_data_get(edje_file, prop_name)) && strcmp(prop_value, "0") == 0)
      {
         new_window->shaped = 0;
         free(prop_value);
      }
      free(prop_name);

      prop_name = malloc(strlen(new_window->window_name) + strlen("_resizable") + 1);
      sprintf(prop_name, "%s_resizable", new_window->window_name);
      if ((prop_value = edje_file_data_get(edje_file, prop_name)) && strcmp(prop_value, "0") == 0)
      {
         new_window->resizable = 0;
         free(prop_value);
      }
      free(prop_name);
   }
   elm_win_borderless_set(new_window->window, new_window->borderless);
   elm_win_alpha_set(new_window->window, new_window->shaped);

   edje_object_size_min_get(new_window->edje_object, &new_window->min_width, &new_window->min_height);
   edje_object_size_max_get(new_window->edje_object, &new_window->max_width, &new_window->max_height);
   if (new_window->max_width < new_window->min_width)
      new_window->max_width = new_window->min_width;
   if (new_window->max_height < new_window->min_height)
      new_window->max_height = new_window->min_height;

   elm_win_resize_object_add(new_window->window, new_window->edje_object);
   evas_object_size_hint_min_set(new_window->window, eclair_scale_pixels(new_window->min_width), eclair_scale_pixels(new_window->min_height));
   evas_object_size_hint_max_set(new_window->window, eclair_scale_pixels(new_window->max_width), eclair_scale_pixels(new_window->max_height));

   config_name = malloc(strlen(new_window->window_name) + strlen("_window") + 1);
   sprintf(config_name, "%s_window", new_window->window_name);
   if (!eclair_config_get_prop_int(&new_window->eclair->config, config_name, "x", &new_window->x) ||
      !eclair_config_get_prop_int(&new_window->eclair->config, config_name, "y", &new_window->y))
   {
      new_window->x = -1;
      new_window->y = -1;
   }

   if (eclair_config_get_prop_int(&new_window->eclair->config, config_name, "w", &window_w) &&
      eclair_config_get_prop_int(&new_window->eclair->config, config_name, "h", &window_h))
      eclair_window_resize(new_window, window_w, window_h, 1);
   else
      eclair_window_resize(new_window, new_window->min_width, new_window->min_height, 1);

   edje_object_calc_force(new_window->edje_object);

   if (main_window || !eclair_config_get_prop_int(&new_window->eclair->config, config_name, "opened", &opened) || opened)
      eclair_window_open(new_window);
   free(config_name);

   return new_window;
}

//Delete the window
void eclair_window_del(Eclair_Window *window)
{
   char *config_name;
   int window_x, window_y, window_w, window_h;

   if (!window)
      return;

   config_name = malloc(strlen(window->window_name) + strlen("_window") + 1);
   sprintf(config_name, "%s_window", window->window_name);
   evas_object_geometry_get(window->window, &window_x, &window_y, &window_w, &window_h);
   eclair_config_set_prop_int(&window->eclair->config, config_name, "x", window_x);
   eclair_config_set_prop_int(&window->eclair->config, config_name, "y", window_y);
   eclair_config_set_prop_int(&window->eclair->config, config_name, "w", window->layout_width);
   eclair_config_set_prop_int(&window->eclair->config, config_name, "h", window->layout_height);
   eclair_config_set_prop_int(&window->eclair->config, config_name, "opened", evas_object_visible_get(window->window));
   free(config_name);

   evas_object_del(window->window);
   free(window->window_name);
   free(window);
}

//Add the default callbacks to the window
static void _eclair_window_menu_cb(void *data, Evas_Object *layout, const char *signal, const char *source)
{
   Eclair_Window *window = data;
   Eclair_Menu *menu = window->eclair->dialogs_manager.menu;
   if (!menu) return;
   if (menu->widget && evas_object_visible_get(menu->widget)) {
      eclair_menu_pop_down(menu);
      return;
   }
   int x, y, h, px, py;
   evas_object_geometry_get(layout, &px, &py, NULL, NULL);
   edje_object_part_geometry_get(layout, "button_menu", &x, &y, NULL, &h);
   eclair_menu_popup_at_xy(menu, px + x, py + y + h + eclair_scale_pixels(2));
}

void eclair_window_add_default_callbacks(Eclair_Window *window, Eclair *eclair)
{
   char *signal_name;

   if (!eclair || !window || !window->edje_object)
      return;

   evas_object_focus_set(window->edje_object, 1);
   evas_object_event_callback_add(window->edje_object, EVAS_CALLBACK_KEY_DOWN, eclair_key_press_cb, eclair);
   evas_object_smart_callback_add(window->window, "delete,request", _eclair_window_close_request_cb, window);
   edje_object_signal_callback_add(window->edje_object, "eclair_open", "*", eclair_open_cb, &eclair->dialogs_manager);
   edje_object_signal_callback_add(window->edje_object, "eclair_menu_toggle", "*", _eclair_window_menu_cb, window);
   edje_object_signal_callback_add(window->edje_object, "eclair_add_file", "*", eclair_open_cb, &eclair->dialogs_manager);
   edje_object_signal_callback_add(window->edje_object, "eclair_play", "*", eclair_play_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_pause", "*", eclair_pause_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_stop", "*", eclair_stop_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_prev", "*", eclair_prev_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_next", "*", eclair_next_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_playlist_load", "*", eclair_playlist_load_cb, &eclair->dialogs_manager);
   edje_object_signal_callback_add(window->edje_object, "eclair_playlist_save", "*", eclair_playlist_save_cb, &eclair->dialogs_manager);
   edje_object_signal_callback_add(window->edje_object, "eclair_remove_all", "*", eclair_remove_all_cb, &eclair->playlist);
   edje_object_signal_callback_add(window->edje_object, "eclair_remove_selected", "*", eclair_remove_selected_cb, &eclair->playlist);
   edje_object_signal_callback_add(window->edje_object, "eclair_remove_unselected", "*", eclair_remove_unselected_cb, &eclair->playlist);
   edje_object_signal_callback_add(window->edje_object, "eclair_select_inverse", "*", eclair_select_inverse_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_select_all", "*", eclair_select_all_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_select_none", "*", eclair_select_none_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "drag*", "progress_bar_drag", eclair_progress_bar_drag_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "drag", "volume_bar_drag", eclair_volume_bar_drag_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "drag", "playlist_scrollbar_drag", eclair_playlist_container_scrollbar_drag_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_playlist_scroll_down_start", "*", eclair_playlist_container_scroll_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_playlist_scroll_down_stop", "*", eclair_playlist_container_scroll_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_playlist_scroll_up_start", "*", eclair_playlist_container_scroll_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_playlist_scroll_up_stop", "*", eclair_playlist_container_scroll_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_shuffle_toggle", "*", eclair_shuffle_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_shuffle_enable", "*", eclair_shuffle_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_shuffle_disable", "*", eclair_shuffle_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_repeat_enable", "*", eclair_repeat_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_repeat_toggle", "*", eclair_repeat_cb, eclair);
   edje_object_signal_callback_add(window->edje_object, "eclair_repeat_disable", "*", eclair_repeat_cb, eclair);
   if (window->window_name)
   {
      signal_name = malloc(strlen("eclair_") + strlen(window->window_name) + strlen("_close") + 1);
      sprintf(signal_name, "eclair_%s_close", window->window_name);
      edje_object_signal_callback_add(window->edje_object, signal_name, "*", eclair_window_close_cb, window);
      free(signal_name); 
      signal_name = malloc(strlen("eclair_") + strlen(window->window_name) + strlen("_minimize") + 1);
      sprintf(signal_name, "eclair_%s_minimize", window->window_name);
      edje_object_signal_callback_add(window->edje_object, signal_name, "*", eclair_window_minimize_cb, window);
      free(signal_name);      
      if (window->resizable)
      {
         signal_name = malloc(strlen("eclair_") + strlen(window->window_name) + strlen("_resize_start") + 1);
         sprintf(signal_name, "eclair_%s_resize_start", window->window_name);
         edje_object_signal_callback_add(window->edje_object, signal_name, "", _eclair_window_resize_start_cb, window);
         free(signal_name);

      }
   }
   evas_object_event_callback_add(window->window, EVAS_CALLBACK_RESIZE, _eclair_window_resize_cb, window);
   edje_object_message_handler_set(window->edje_object, eclair_message_cb, eclair);
}

//Resize the window and its objects
void eclair_window_resize(Eclair_Window *window, Evas_Coord width, Evas_Coord height, Eina_Bool resize_window)
{
   if (!window)
      return;

   if (width < window->min_width)
      width = window->min_width;
   else if (width > window->max_width)
      width = window->max_width;
   if (height < window->min_height)
      height = window->min_height;
   else if (height > window->max_height)
      height = window->max_height;
   
   window->layout_width = width;
   window->layout_height = height;
   width = eclair_scale_pixels(width);
   height = eclair_scale_pixels(height);
   evas_object_resize(window->edje_object, width, height);
   if (resize_window)
      evas_object_resize(window->window, width, height);
}

//Show the window
void eclair_window_open(Eclair_Window *window)
{
   char *signal_name;

   if (!window)
      return;
   
   evas_object_show(window->window);
   if (window->x >= 0 && window->y >= 0)
      evas_object_move(window->window, window->x, window->y);
   signal_name = malloc(strlen("signal_") + strlen(window->window_name) + strlen("_opened") + 1);
   sprintf(signal_name, "signal_%s_opened", window->window_name);
   eclair_send_signal_to_all_windows(window->eclair, signal_name);
   free(signal_name);
}

//Quit eclair if the window is a main window, otherwise, just hide the window
void eclair_window_close(Eclair_Window *window)
{
   char *signal_name;

   if (!window)
      return;

   if (window->main_window)
      eclair_shutdown(window->eclair);
   else
   {
      evas_object_geometry_get(window->window, &window->x, &window->y, NULL, NULL);
      evas_object_hide(window->window);
      signal_name = malloc(strlen("signal_") + strlen(window->window_name) + strlen("_closed") + 1);
      sprintf(signal_name, "signal_%s_closed", window->window_name);
      eclair_send_signal_to_all_windows(window->eclair, signal_name);
      free(signal_name);
   }
}


//------------------------------
// Callbacks
//------------------------------

//Called when the user clicks on the button to open the window
void eclair_window_open_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_window_open(data);
}

//Called when the user clicks on the close button of the window
void eclair_window_close_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_window_close(data);
}

//Called when the user clicks on the minimize button of the window
void eclair_window_minimize_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   Eclair_Window *window;

   if ((window = data))
      elm_win_iconified_set(window->window, EINA_TRUE);
}

//Called when the window object sends the "signal_resize_start" signal
static void _eclair_window_resize_start_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   Eclair_Window *window;

   if ((window = data))
      elm_win_move_resize_start(window->window, ELM_WIN_MOVE_RESIZE_BOTTOM | ELM_WIN_MOVE_RESIZE_RIGHT);
}

// Keep logical theme dimensions in step with the compositor's client size.
static void _eclair_window_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Eclair_Window *window = data;
   int width, height;
   evas_object_geometry_get(obj, NULL, NULL, &width, &height);
   eclair_window_resize(window, eclair_scale_logical(width), eclair_scale_logical(height), EINA_FALSE);
}

static void _eclair_window_close_request_cb(void *data, Evas_Object *obj, void *event)
{
   eclair_window_close(data);
}
