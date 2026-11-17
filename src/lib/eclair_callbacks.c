#include "eclair_scale.h"
#include <Ecore_Input.h>
#include "eclair_callbacks.h"
#include <string.h>
#include <Emotion.h>
#include <Ecore.h>
#include <Ecore_Evas.h>

#include "eclair.h"
#include "eclair_playlist.h"
#include "eclair_playlist_container.h"
#include "eclair_dialogs.h"
#include "eclair_window.h"

//Called when eclair is closed
Eina_Bool eclair_exit_cb(void *data, int type, void *event)
{
   eclair_shutdown(data);

   return 1;
}

//Called when a key is pressed
void eclair_key_press_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Eclair *eclair;
   Evas_Event_Key_Down *ev;

   if (!(eclair = data))
      return;

   ev = event_info;
   if (strcmp(ev->key, "f") == 0 && eclair->video.window)
      elm_win_fullscreen_set(eclair->video.window, !elm_win_fullscreen_get(eclair->video.window));
   else if (strcmp(ev->key, "space") == 0 || strcmp(ev->key, "p") == 0)
   {
      if (eclair->state == ECLAIR_PLAYING)
         eclair_pause(eclair);
      else
         eclair_play(eclair);
   }
   else if (strcmp(ev->key, "escape") == 0 || strcmp(ev->key, "q") == 0)
   {
      eclair_shutdown(eclair);
      return;
   }
   else if (eclair->video.video_object && (strcmp(ev->key, "Left") == 0 ||
      strcmp(ev->key, "Right") == 0 || strcmp(ev->key, "Down") == 0 ||
      strcmp(ev->key, "Up") == 0 || strcmp(ev->key, "Prior") == 0 ||
      strcmp(ev->key, "Next") == 0 || strcmp(ev->key, "Home") == 0))
   {
      double media_position;

      media_position = eclair_position_get(eclair);   
      if (strcmp(ev->key, "Left") == 0)
         media_position -= 15.0;
      else if (strcmp(ev->key, "Right") == 0)
         media_position += 15.0;
      else if (strcmp(ev->key, "Down") == 0)
         media_position -= 60.0;
      else if (strcmp(ev->key, "Up") == 0)
         media_position += 60.0;
      else if (strcmp(ev->key, "Next") == 0)
         media_position -= 600.0;
      else if (strcmp(ev->key, "Prior") == 0)
         media_position += 600.0;
      else if (strcmp(ev->key, "Home") == 0)
         media_position = 0.0;
      eclair_position_set(eclair, media_position);
   }
   else if (eclair->gui_window && (strcmp(ev->key, "KP_Divide") == 0 || strcmp(ev->key, "slash") == 0 ||
      strcmp(ev->key, "KP_Multiply") == 0 || strcmp(ev->key, "asterisk") == 0))
   {
      double volume;

      edje_object_part_drag_value_get(eclair->gui_window->edje_object, "volume_bar_drag", &volume, 0);
      if (strcmp(ev->key, "KP_Divide") == 0 || strcmp(ev->key, "slash") == 0)
         volume -= (1.0 / 100);
      else if (strcmp(ev->key, "KP_Multiply") == 0 || strcmp (ev->key, "asterisk") == 0)
         volume += (1.0 / 100);

      eclair_audio_level_set(eclair, volume);
   }
   else if (strcmp(ev->key, "Delete") == 0)
      eclair_playlist_remove_selected_media_files(&eclair->playlist);
   else if (strcmp(ev->key, "a") == 0 && evas_key_modifier_is_set(ev->modifiers, "Control"))
      eclair_playlist_container_select_all(eclair->playlist_container);
}

//Called when the gui send a message
void eclair_message_cb(void *data, Evas_Object *obj, Edje_Message_Type type, int id, void *msg)
{
   Eclair *eclair;
   
   if (!(eclair = data))
      return;

   switch (id)
   {
      default:
         break;
   }
}

void eclair_dnd_enter_cb(void *data, Evas_Object *obj)
{
   Eclair_Window *window = data;
   window->drop_over_cover = EINA_FALSE;
}

void eclair_dnd_position_cb(void *data, Evas_Object *obj, int x, int y, Elm_Xdnd_Action action)
{
   Eclair_Window *window = data;
   Eclair *app = window->eclair;
   int cx, cy, w, h, ox, oy;
   window->drop_over_cover = EINA_FALSE;
   if (app->cover_owner != window) return;
   evas_object_geometry_get(window->edje_object, &ox, &oy, NULL, NULL);
   const char *parts[] = {"cover", "record_cover", NULL};
   for (int i = 0; parts[i]; i++) {
      if (!edje_object_part_exists(window->edje_object, parts[i])) continue;
      edje_object_part_geometry_get(window->edje_object, parts[i], &cx, &cy, &w, &h);
      if (x >= ox + cx && x < ox + cx + w && y >= oy + cy && y < oy + cy + h) {
         window->drop_over_cover = EINA_TRUE;
         break;
      }
   }
}

/* The position callback supplies coordinates; some EFL versions do not
 * populate Elm_Selection_Data.x/y when delivering the transferred contents. */
Eina_Bool eclair_dnd_drop_cb(void *data, Evas_Object *obj, Elm_Selection_Data *event)
{
   Eclair_Window *window = data;
   if (!window || !window->eclair || !event || !event->data || event->len <= 0)
      return EINA_FALSE;
   Eclair *app = window->eclair;
   char *uris = strndup(event->data, event->len), *save = NULL;
   if (!uris) return EINA_FALSE;
   Eina_Bool accepted = EINA_FALSE;
   for (char *uri = strtok_r(uris, "\r\n", &save); uri; uri = strtok_r(NULL, "\r\n", &save)) {
      if (!*uri || *uri == '#') continue;
      if (eclair_playlist_add_uri(&app->playlist, uri, EINA_FALSE, EINA_TRUE))
         accepted = EINA_TRUE;
      else if (window->drop_over_cover && eclair_cover_current_set(&app->cover_manager, uri))
         accepted = EINA_TRUE;
   }
   free(uris);
   window->drop_over_cover = EINA_FALSE;
   if (accepted) eclair_playlist_container_update(app->playlist_container);
   return accepted;
}

//Called when the user clicks on open button
void eclair_open_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_dialogs_open_fc_add_files(data);
}

//Called when the user clicks on play button
void eclair_play_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_play(data);
}

//Called when the user clicks on prev button
void eclair_prev_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_play_prev(data);
}

//Called when the user clicks on next button
void eclair_next_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_play_next(data);
}

//Called when the user clicks on pause button
void eclair_pause_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_pause(data);
}

//Called when the user clicks on stop button
void eclair_stop_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_stop(data);
}

//Called when user enables or disables repeating
void eclair_repeat_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   Eclair *eclair;

   if (!(eclair = data))
      return;

   if (strcmp(emission, "eclair_repeat_toggle") == 0)
      eclair_playlist_set_repeat(&eclair->playlist, !eclair->playlist.repeat);
   else if (strcmp(emission, "eclair_repeat_enable") == 0)
      eclair_playlist_set_repeat(&eclair->playlist, 1);
   else
      eclair_playlist_set_repeat(&eclair->playlist, 0);
}

//Called when user enables or disables shuffling
void eclair_shuffle_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   Eclair *eclair;

   if (!(eclair = data))
      return;

   if (strcmp(emission, "eclair_shuffle_toggle") == 0)
      eclair_playlist_set_shuffle(&eclair->playlist, !eclair->playlist.shuffle);
   else if (strcmp(emission, "eclair_shuffle_enable") == 0)
      eclair_playlist_set_shuffle(&eclair->playlist, 1);
   else
      eclair_playlist_set_shuffle(&eclair->playlist, 0);
}

//Called when the user clicks of the "load playlist" button  
void eclair_playlist_load_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_dialogs_open_fc_load_playlist(data);
}

//Called when the user clicks of the "save playlist" button
void eclair_playlist_save_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_dialogs_open_fc_save_playlist(data);
}

//Called when the user clicks of the "remove all" button
void eclair_remove_all_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_playlist_empty(data);
}

//Called when the user clicks of the "remove selected" button
void eclair_remove_selected_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_playlist_remove_selected_media_files(data);
}

//Called when the user clicks of the "remove unselected" button
void eclair_remove_unselected_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   eclair_playlist_remove_unselected_media_files(data);
}

//Called when the user clicks of the "select all" button
void eclair_select_all_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   Eclair *eclair;

   if ((eclair = data))
      eclair_playlist_container_select_all(eclair->playlist_container);
}

//Called when the user clicks of the "select none" button
void eclair_select_none_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   Eclair *eclair;

   if ((eclair = data))
      eclair_playlist_container_select_none(eclair->playlist_container);
}

//Called when the user clicks of the "invert selection" button
void eclair_select_inverse_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   Eclair *eclair;

   if ((eclair = data))
      eclair_playlist_container_invert_selection(eclair->playlist_container);
}

//Called when the user drags the progress bar button
void eclair_progress_bar_drag_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   Eclair *eclair;
   double progress_rate;

   if (!(eclair = data) || !eclair->gui_window)
      return;

   if (strcmp(emission, "drag,start") == 0)
      eclair->use_progress_bar_drag_for_time = 1;      
   else if (strcmp(emission, "drag,stop") == 0)
   {
      eclair->use_progress_bar_drag_for_time = 0;
      edje_object_part_drag_value_get(eclair->gui_window->edje_object, "progress_bar_drag", &progress_rate, NULL);
      eclair_progress_rate_set(eclair, progress_rate);
   }
}

//Called when the user drags the volume bar button
void eclair_volume_bar_drag_cb(void *data, Evas_Object *edje_object, const char *emission, const char *source)
{
   Eclair *eclair;
   double volume;

   if (!(eclair = data) || !eclair->gui_window)
      return;

   edje_object_part_drag_value_get(eclair->gui_window->edje_object, "volume_bar_drag", &volume, NULL);
   eclair_audio_level_set(eclair, volume);
}
