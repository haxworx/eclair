#ifndef _ECLAIR_VIDEO_H_
#define _ECLAIR_VIDEO_H_

#include <Evas.h>
#include <Ecore.h>
#include "eclair_types.h"

struct _Eclair_Video
{
   Evas_Object *video_object;
   Evas_Object *subtitles_object;
   Ecore_Job *create_job;
   Evas_Object *layout;
   Eina_Bool visible;
   Eclair *eclair;
};

void eclair_video_init(Eclair_Video *video, Eclair *eclair);
void eclair_video_shutdown(Eclair_Video *video);
void eclair_video_start(Eclair_Video *video);
Eina_Bool eclair_video_attach(Eclair_Video *video, Eclair_Window *window);
void eclair_video_hide(Eclair_Video *video);

#endif
