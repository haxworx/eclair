#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT
#include <Ecore_Evas.h>
#include <Edje.h>
#include <Edje_Edit.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
   ecore_evas_init();
   edje_init();
   Ecore_Evas *window = ecore_evas_buffer_new(1600, 1200);
   Evas *evas = ecore_evas_get(window);
   int failed = 0;
   for (int file = 1; file < argc; file++) {
      Eina_List *groups = edje_file_collection_list(argv[file]), *g;
      char *group;
      EINA_LIST_FOREACH(groups, g, group) {
         Evas_Object *base = edje_edit_object_add(evas);
         Evas_Object *scaled = edje_edit_object_add(evas);
         if (!edje_object_file_set(base, argv[file], group) ||
             !edje_object_file_set(scaled, argv[file], group)) return 2;
         int w, h;
         edje_object_size_min_get(base, &w, &h);
         if (w < 20) w = 400;
         if (h < 20) h = 200;
         edje_object_scale_set(base, 1.0);
         evas_object_resize(base, w, h);
         Eina_List *parts = edje_edit_parts_list_get(base), *l;
         char *part;
         {
            edje_object_calc_force(base);
            for (int factor = 0; factor < 2; factor++) {
               double scale = factor ? 2.0 : 1.5;
               edje_object_scale_set(scaled, scale);
               evas_object_resize(scaled, lround(w * scale), lround(h * scale));
               edje_object_calc_force(scaled);
               EINA_LIST_FOREACH(parts, l, part) {
                  Edje_Part_Type type = edje_edit_part_type_get(base, part);
                  if (type == EDJE_PART_TYPE_TEXT || type == EDJE_PART_TYPE_TEXTBLOCK) continue;
                  int a[4], b[4];
                  edje_object_part_geometry_get(base, part, &a[0], &a[1], &a[2], &a[3]);
                  edje_object_part_geometry_get(scaled, part, &b[0], &b[1], &b[2], &b[3]);
                  for (int dim = 0; dim < 4; dim++) {
                     /* This width follows a timestamp's hinted font metrics.
                      * playlist-row tests its separation at every scale. */
                     if (dim == 2 && !strcmp(part, "playlist_entry_name_clip")) continue;
                     if (fabs(b[dim] - a[dim] * scale) > 2.0) {
                        fprintf(stderr, "%s:%s:%s scale %.1f dimension %d: %d vs %.1f expected\n",
                                argv[file], group, part, scale, dim, b[dim], a[dim] * scale);
                        failed++;
                     }
                  }
               }
            }
         }
         edje_edit_string_list_free(parts);
         evas_object_del(base);
         evas_object_del(scaled);
      }
      edje_file_collection_list_free(groups);
   }
   ecore_evas_free(window);
   edje_shutdown();
   ecore_evas_shutdown();
   return failed ? 1 : 0;
}
