#include "eclair_scale.h"
#include "eclair_subtitles.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int _eclair_subtitles_sort_cb(const void *sub1, const void *sub2);

//Init subtitles module
void eclair_subtitles_init(Eclair_Subtitles *subtitles)
{
   if (!subtitles)
      return;

   subtitles->subtitles = NULL;
   subtitles->enable = 0;
}

//Free the loaded subtitles
void eclair_subtitles_free(Eclair_Subtitles *subtitles)
{
   Eclair_Subtitle *sub;

   if (!subtitles)
      return;

   EINA_LIST_FREE(subtitles->subtitles, sub)
   {
      free(sub->subtitle);
      free(sub);
   }
   subtitles->subtitles = NULL;
}

//Return current subtitle (may be NULL)
char *eclair_subtitles_get_current_subtitle(Eclair_Subtitles *subtitles, double current_time)
{
   Eina_List *l;
   Eclair_Subtitle *subtitle;

   if (!subtitles || !subtitles->subtitles)
      return NULL;

   for (l = subtitles->subtitles; l && (subtitle = l->data); l = l->next)
   {
      if (subtitle->start <= current_time && subtitle->end >= current_time)
         return subtitle->subtitle;
   }

   return NULL;
}

//Display subtitles corresponding to the current time
void eclair_subtitles_display_current_subtitle(Eclair_Subtitles *subtitles, double current_time, Evas_Object *subtitles_object)
{
   const char *text;
   char *markup;
   Evas_Textblock_Style *style;
   char style_text[256];

   if (!subtitles || !subtitles_object) return;
   text = subtitles->enable ? eclair_subtitles_get_current_subtitle(subtitles, current_time) : NULL;
   style = evas_textblock_style_new();
   snprintf(style_text, sizeof(style_text), "DEFAULT='font=Sans:style=Bold font_size=%d color=#ffffffff outline_color=#000000ff style=outline wrap=word align=center'", eclair_scale_pixels(12));
   evas_textblock_style_set(style, style_text);
   evas_object_textblock_style_set(subtitles_object, style);
   evas_textblock_style_free(style);
   markup = evas_textblock_text_utf8_to_markup(subtitles_object, text ? text : "");
   evas_object_textblock_text_markup_set(subtitles_object, markup ? markup : "");
   free(markup);
}

//Return 0 if the two subtitles starts at the same time
//-1 if data1 starts before data2
//1 if data1 starts after data2
int _eclair_subtitles_sort_cb(const void *data1, const void *data2)
{
   const Eclair_Subtitle *sub1, *sub2;

   if (!(sub1 = data1))
      return 1;
   if (!(sub2 = data2))
      return -1;

   if (sub1->start < sub2->start)
      return 1;
   if (sub1->start == sub2->start)
      return 0;
   return -1;
}

//Sort subtitles by start time
void eclair_subtitles_sort(Eclair_Subtitles *subtitles)
{
   if (!subtitles || !subtitles->subtitles)
      return;

   subtitles->subtitles = eina_list_sort(subtitles->subtitles, eina_list_count(subtitles->subtitles), _eclair_subtitles_sort_cb);
}

//Try to load subtitles from the media filename
Eina_Bool eclair_subtitles_load_from_media_file(Eclair_Subtitles *subtitles, const char *media_file)
{
   char *subtitles_filename;
   char *extension_start;
   int extension_start_id;

   if (!subtitles) return EINA_FALSE;
   subtitles->enable = EINA_FALSE;
   if (!media_file || !(extension_start = strrchr(media_file, '.')))
      return EINA_FALSE;
   
   extension_start_id = extension_start - media_file + 1;
   subtitles_filename = malloc(strlen(media_file) + 10);
   strcpy(subtitles_filename, media_file);

   //Try to load srt subtitles
   sprintf(&subtitles_filename[extension_start_id], "srt");
   subtitles->enable = eclair_subtitles_load_srt(subtitles, subtitles_filename);

   free(subtitles_filename);

   return subtitles->enable;
}

//Load subtitles from a srt file
Eina_Bool eclair_subtitles_load_srt(Eclair_Subtitles *subtitles, const char *path)
{
   FILE *subtitles_file;
   char line[4096];
   int tmp;
   Eina_Bool have_num, have_time, have_text;
   int start[4], end[4];
   Eclair_Subtitle *new_subtitle;

   have_num = have_time = have_text = 0;
   new_subtitle = NULL;

   if (!subtitles || !path)
      return 0;

   eclair_subtitles_free(subtitles);

   if (!(subtitles_file = fopen(path, "rt")))
      return 0;

   while (fgets(line, 4096, subtitles_file))
   {
      if (line[strspn(line, " \t\r\n")] == '\0')
         continue;

      if (!have_num || have_text)
      {
         if (sscanf(line, "%d\n", &tmp) == 1)
         {
            if (new_subtitle)
            {
               //TODO: improve
               if (new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] == '\n' ||
                  new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] == '\r')
                  new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] = 0;
               if (new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] == '\n' ||
                  new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] == '\r')
                  new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] = 0;
               subtitles->subtitles = eina_list_append(subtitles->subtitles, new_subtitle);
            }
            new_subtitle = calloc(1, sizeof(Eclair_Subtitle));
            have_time = have_text = 0;
            have_num = 1;
            continue;
         }
      }
      if (!have_num)
         continue;
      
      if (!have_time)
      {
         if (sscanf(line, "%d:%d:%d,%d --> %d:%d:%d,%d", &start[0], &start[1], 
            &start[2], &start[3], &end[0],  &end[1],  &end[2],  &end[3]) == 8)
         {
            have_time = 1;
            new_subtitle->start = start[0] * 3600 + start[1] * 60 + start[2] + start[3] * 0.001;
            new_subtitle->end = end[0] * 3600 + end[1] * 60 + end[2] + end[3] * 0.001;
            continue;
         }
      }
      if (!have_time)
         continue;

      if (new_subtitle->subtitle)
      {
         new_subtitle->subtitle = realloc(new_subtitle->subtitle, strlen(new_subtitle->subtitle) + strlen(line) + 1);
         strcat(new_subtitle->subtitle, line);
      }
      else
         new_subtitle->subtitle = strdup(line);
      have_text = 1;
   }

   if (new_subtitle && have_text)
   {
      if (new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] == '\n' ||
         new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] == '\r')
         new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] = 0;
      if (new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] == '\n' ||
         new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] == '\r')
         new_subtitle->subtitle[strlen(new_subtitle->subtitle) - 1] = 0;
      subtitles->subtitles = eina_list_append(subtitles->subtitles, new_subtitle);
   }

   fclose(subtitles_file);

   eclair_subtitles_sort(subtitles);

   return 1;
}
