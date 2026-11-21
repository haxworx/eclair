#include "eclair_video.h"
#include <Emotion.h>
#include <Edje.h>
#include "eclair.h"
#include "eclair_window.h"

static void _eclair_video_playback_finished_cb(void *data, Evas_Object *obj, void *event_info);
static void _eclair_video_audio_level_change_cb(void *data, Evas_Object *obj, void *event_info);

static void _eclair_video_visible_set(Eclair_Video *video, Eina_Bool visible)
{
   if (!video->layout || video->visible == visible) return;

   video->visible = visible;
   edje_object_signal_emit(video->layout, visible ? "signal_video_show" : "signal_video_hide",
                           "eclair_bin");
   edje_object_message_signal_process(video->layout);
}

void eclair_video_hide(Eclair_Video *video)
{
   _eclair_video_visible_set(video, EINA_FALSE);
}

static void _eclair_video_frame_cb(void *data, Evas_Object *obj, void *event_info)
{
   Eclair_Video *video = data;
   Eina_Bool active = video->eclair->state != ECLAIR_STOP;

   _eclair_video_visible_set(video, active && emotion_object_video_handled_get(obj));
   if (active) eclair_update(video->eclair);
}

void eclair_video_init(Eclair_Video *video, Eclair *eclair)
{
   *video = (Eclair_Video){ .eclair = eclair };
}

void eclair_video_shutdown(Eclair_Video *video)
{
   if (video->create_job) ecore_job_del(video->create_job);
   if (video->subtitles_object) evas_object_del(video->subtitles_object);
   if (video->video_object) evas_object_del(video->video_object);
   eclair_video_init(video, video->eclair);
}

Eina_Bool eclair_video_attach(Eclair_Video *video, Eclair_Window *window)
{
   Eclair *eclair = video->eclair;
   Evas_Object *previous = video->video_object;
   const char *path = previous && eclair->state != ECLAIR_STOP ?
      emotion_object_file_get(previous) : NULL;
   Evas_Object *player = emotion_object_add(window->evas);
   const char *module = eclair->video_module == ECLAIR_VIDEO_GSTREAMER ? "gstreamer1" : NULL;

   if (!player) return EINA_FALSE;
   if (!emotion_object_init(player, module) ||
       (path && !emotion_object_file_set(player, path)))
   {
      evas_object_del(player);
      return EINA_FALSE;
   }

   emotion_object_keep_aspect_set(player, EMOTION_ASPECT_KEEP_BOTH);
   emotion_object_bg_color_set(player, 0, 0, 0, 255);
   evas_object_pass_events_set(player, EINA_TRUE);
   video->visible = EINA_FALSE;
   video->layout = NULL;
   if (edje_object_part_exists(window->edje_object, "artwork_video") &&
       edje_object_part_swallow(window->edje_object, "artwork_video", player))
      video->layout = window->edje_object;

   if (video->subtitles_object) evas_object_del(video->subtitles_object);
   video->subtitles_object = NULL;
   if (video->layout && edje_object_part_exists(window->edje_object, "artwork_subtitles"))
   {
      Evas_Object *subtitle = evas_object_textblock_add(window->evas);
      evas_object_pass_events_set(subtitle, EINA_TRUE);
      evas_object_textblock_valign_set(subtitle, 1.0);
      edje_object_part_swallow(window->edje_object, "artwork_subtitles", subtitle);
      video->subtitles_object = subtitle;
   }

   if (previous)
   {
      emotion_object_audio_volume_set(player, emotion_object_audio_volume_get(previous));
      if (path)
      {
         double position = eclair_position_get(eclair);
         emotion_object_play_set(previous, EINA_FALSE);
         emotion_object_position_set(player, position);
         emotion_object_play_set(player, eclair->state == ECLAIR_PLAYING);
      }
      evas_object_del(previous);
   }

   video->video_object = player;
   evas_object_smart_callback_add(player, "open_done", _eclair_video_frame_cb, video);
   evas_object_smart_callback_add(player, "frame_decode", _eclair_video_frame_cb, video);
   evas_object_smart_callback_add(player, "playback_finished", _eclair_video_playback_finished_cb, video);
   evas_object_smart_callback_add(player, "audio_level_change", _eclair_video_audio_level_change_cb, video);
   return EINA_TRUE;
}

static void _eclair_video_create_cb(void *data)
{
   Eclair_Video *video = data;
   Eclair *eclair = video->eclair;

   video->create_job = NULL;
   if (!eclair_video_attach(video, eclair->gui_window)) return;
   eclair_audio_level_set(eclair, emotion_object_audio_volume_get(video->video_object));
   eclair_update_current_file_info(eclair, EINA_FALSE);
   if (eclair->start_playing) eclair_play_current(eclair);
}

void eclair_video_start(Eclair_Video *video)
{
   video->create_job = ecore_job_add(_eclair_video_create_cb, video);
}

static void _eclair_video_playback_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
   Eclair_Video *video;

   if ((video = data))
      eclair_play_next(video->eclair);
}

//Called when the audio volume is changed by an external application
static void _eclair_video_audio_level_change_cb(void *data, Evas_Object *obj, void *event_info)
{
   Eclair_Video *video;
   Eclair *eclair;

   if ((video = data) && (eclair = video->eclair) && video->video_object)
      eclair_audio_level_set(eclair, emotion_object_audio_volume_get(video->video_object));
}
