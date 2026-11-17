#ifndef _ECLAIR_VIDEO_H_
#define _ECLAIR_VIDEO_H_

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <pthread.h>
#include "eclair_types.h"

struct _Eclair_Video
{
   Evas_Object *window;
   Evas_Object *video_object;
   Evas_Object *black_background;
   Evas_Object *subtitles_object;
   Ecore_Job *create_job;
   int previous_video_width, previous_video_height;
   double previous_video_ratio;
   Eclair *eclair;
};

void eclair_video_init(Eclair_Video *video, Eclair *eclair);
void eclair_video_shutdown(Eclair_Video *video);
void eclair_create_video_window(Eclair_Video *video, Eclair_Engine engine);
void eclair_video_window_resize(Eclair_Video *video);

#endif
