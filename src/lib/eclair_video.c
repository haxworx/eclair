#include "eclair_scale.h"
#include "eclair_video.h"
#include "config.h"
#include <Emotion.h>
#include <Elementary.h>
#include "eclair.h"
#include "eclair_callbacks.h"
#include "eclair_window.h"

static void _eclair_video_create_emotion_object(void *param);
static void _eclair_video_frame_decode_cb(void *data, Evas_Object *obj, void *event_info);
static void _eclair_video_playback_finished_cb(void *data, Evas_Object *obj, void *event_info);
static void _eclair_video_frame_resize_change_cb(void *data, Evas_Object *obj, void *event_info);
static void _eclair_video_audio_level_change_cb(void *data, Evas_Object *obj, void *event_info);
static void _eclair_video_audio_level_change_cb(void *data, Evas_Object *obj, void *event_info);
static void _eclair_video_window_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _eclair_video_window_close_cb(void *data, Evas_Object *obj, void *event);

//Initialize video vars
void eclair_video_init(Eclair_Video *video, Eclair *eclair)
{
   if (!video)
      return;

   video->create_job = NULL;
   video->window = NULL;
   video->video_object = NULL;
   video->black_background = NULL;
   video->subtitles_object = NULL;
   video->previous_video_width = 0;
   video->previous_video_height = 0;
   video->previous_video_ratio = 0.0;
   video->eclair = eclair;
}

//Shutdown the video module
void eclair_video_shutdown(Eclair_Video *video)
{
   if (!video)
      return;

   if (video->create_job) ecore_job_del(video->create_job);
   video->create_job = NULL;
   if (video->window) evas_object_del(video->window);
   video->window = NULL;
   video->video_object = video->black_background = video->subtitles_object = NULL;
}
//Create the video window and object
void eclair_create_video_window(Eclair_Video *video, Eclair_Engine engine)
{
   Evas *evas;

   if (!video)
      return;

   elm_config_accel_preference_set(engine == ECLAIR_GL ? "gl" : "none");
   video->window = elm_win_add(NULL, "eclair-video", ELM_WIN_BASIC);
   if (!video->window) return;
   elm_win_title_set(video->window, "eclair");
   evas_object_event_callback_add(video->window, EVAS_CALLBACK_RESIZE,
                                  _eclair_video_window_resize_cb, video);
   evas_object_smart_callback_add(video->window, "delete,request", _eclair_video_window_close_cb, video);
   evas = evas_object_evas_get(video->window);
   video->black_background = evas_object_rectangle_add(evas);
   evas_object_color_set(video->black_background, 0, 0, 0, 255);
   evas_object_layer_set(video->black_background, 0);
   evas_object_show(video->black_background);

   video->subtitles_object = evas_object_textblock_add(evas);
   evas_font_path_append(evas, PACKAGE_DATA_DIR "/fonts/");
   evas_object_layer_set(video->subtitles_object, 2);
   evas_object_show(video->subtitles_object);

   video->create_job = ecore_job_add(_eclair_video_create_emotion_object, video);
}

//Initialize the video object on the Ecore main loop
static void _eclair_video_create_emotion_object(void *param)
{
   Eclair_Video *video;
   Eclair *eclair;
   Evas *evas;
   Evas_Object *new_video_object;
   int result = 0;

   if (!(video = param) || !(eclair = video->eclair))
      return;

   video->create_job = NULL;

   //TODO: make functions to display whatever we want on the media bar
   if (eclair->gui_window)
      eclair_all_windows_text_set(eclair, "current_media_name", "Initializing - Please wait...");

   evas = evas_object_evas_get(video->window);
   new_video_object = emotion_object_add(evas);
   evas_object_layer_set(new_video_object, 1);
   evas_object_show(new_video_object);

   switch (eclair->video_module)
   {
      case ECLAIR_VIDEO_GSTREAMER:
         result |= emotion_object_init(new_video_object, "gstreamer1");
         break;
      case ECLAIR_VIDEO_AUTO:
      default:
         result |= emotion_object_init(new_video_object, NULL);
         break;
   }
   if (!result)
   {
      evas_object_del(new_video_object);
      return;
   }

   evas_object_focus_set(new_video_object, 1);
   evas_object_event_callback_add(new_video_object, EVAS_CALLBACK_KEY_DOWN, eclair_key_press_cb, eclair);
   evas_object_smart_callback_add(new_video_object, "frame_decode", _eclair_video_frame_decode_cb, video);
   evas_object_smart_callback_add(new_video_object, "playback_finished", _eclair_video_playback_finished_cb, video);
   evas_object_smart_callback_add(new_video_object, "audio_level_change", _eclair_video_audio_level_change_cb, video);
   evas_object_smart_callback_add(new_video_object, "frame_resize", _eclair_video_frame_resize_change_cb, video);

   eclair_audio_level_set(eclair, emotion_object_audio_volume_get(new_video_object));
   video->video_object = new_video_object;
   eclair_update_current_file_info(eclair, 0);

   if (eclair->start_playing)
      eclair_play_current(eclair);

   return;
}

//Resize the video object and the black background object according to the size of the window
void eclair_video_window_resize(Eclair_Video *video)
{
   Evas_Coord window_width, window_height, video_width, video_height, X, Y;
   double ratio;

   if (!video || !video->window)
      return;

   evas_object_geometry_get(video->window, NULL, NULL, &window_width, &window_height);

   if (video->video_object)
   {
      ratio = emotion_object_ratio_get(video->video_object);
      if (ratio > 0.0)
      {
         if (window_width / ratio > window_height)
         {
            video_height = window_height;
            video_width = video_height * ratio;
         }
         else
         {
            video_width = window_width;
            video_height = video_width / ratio;
         }

         X = (window_width - video_width) / 2.0;
         Y = (window_height - video_height) / 2.0;
      }
      else
      {
         video_width = window_width;
         video_height = window_height;
         X = Y = 0;
      }
      evas_object_move(video->video_object, X, Y);
      evas_object_resize(video->video_object, video_width, video_height);
   }
   if (video->black_background)
   {
      evas_object_move(video->black_background, 0, 0);
      evas_object_resize(video->black_background, window_width, window_height);
   }
}


//------------------------------
// Callbacks
//------------------------------

//Called when a new frame is decoded
static void _eclair_video_frame_decode_cb(void *data, Evas_Object *obj, void *event_info)
{
   Eclair_Video *video;

   if ((video = data))
      eclair_update(video->eclair);
}

//Called when the current media playback is finished
static void _eclair_video_playback_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
   Eclair_Video *video;

   if ((video = data))
      eclair_play_next(video->eclair);
}

//Called when the video has to be resized
static void _eclair_video_frame_resize_change_cb(void *data, Evas_Object *obj, void *event_info)
{
   Eclair_Video *video;
   int w, h;
   double ratio;

   if (!(video = data) || !video->window || !video->video_object)
      return;

   emotion_object_size_get(video->video_object, &w, &h);
   ratio = emotion_object_ratio_get(video->video_object);

   if (video->previous_video_width == w && video->previous_video_height == h && video->previous_video_ratio == ratio)
      return;

   if (ratio > 0.0)
      evas_object_resize(video->window, eclair_scale_pixels(h * ratio), eclair_scale_pixels(h));
   else
      evas_object_resize(video->window, eclair_scale_pixels(w), eclair_scale_pixels(h));

   video->previous_video_width = w;
   video->previous_video_height = h;
   video->previous_video_ratio = ratio;
}

//Called when the audio volume is changed by an external application
static void _eclair_video_audio_level_change_cb(void *data, Evas_Object *obj, void *event_info)
{
   Eclair_Video *video;
   Eclair *eclair;

   if ((video = data) && (eclair = video->eclair) && video->video_object)
      eclair_audio_level_set(eclair, emotion_object_audio_volume_get(video->video_object));
}

static void _eclair_video_window_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   eclair_video_window_resize(data);
}

static void _eclair_video_window_close_cb(void *data, Evas_Object *obj, void *event)
{
   Eclair_Video *video = data;
   eclair_stop(video->eclair);
}
