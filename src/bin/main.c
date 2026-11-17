#include "eclair.h"
#include <Elementary.h>
#include <Ecore_File.h>
#include <Edje.h>
#include <Emotion.h>

int main(int argc, char *argv[])
{
   Eclair eclair = {0};

   if (!eclair_init(&eclair, &argc, &argv))
      return (eclair.help_requested || eclair.config.already_running) ? 0 : 1;
   
   ecore_main_loop_begin();

   elm_shutdown();
   emotion_shutdown();
   edje_shutdown();
   ecore_file_shutdown();
   ecore_evas_shutdown();
   ecore_shutdown();

   return 0;
}
