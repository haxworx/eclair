#include <Ecore_Evas.h>
#include <Edje.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv)
{
   ecore_evas_init(); edje_init();
   Ecore_Evas *canvas = ecore_evas_buffer_new(1000, 1000);
   assert(canvas);
   Evas_Object *row = edje_object_add(ecore_evas_get(canvas));
   for (int theme = 1; theme < argc; theme++) {
      assert(edje_object_file_set(row, argv[theme], "eclair_playlist_entry"));
      const char *durations[] = {"0:59", "12:34", "1:23:45"};
      const double scales[] = {1, 1.5, 2};
      for (unsigned s = 0; s < 3; s++) {
         edje_object_scale_set(row, scales[s]);
         for (int width = 120; width <= 240; width += 120) {
            evas_object_resize(row, width * scales[s], 15 * scales[s]);
            edje_object_part_text_set(row, "playlist_entry_name",
               "A very long artist and track name that must never paint over the duration");
            for (unsigned d = 0; d < 3; d++) {
               edje_object_part_text_set(row, "playlist_entry_length", durations[d]);
               edje_object_calc_force(row);
               Evas_Object *title = (Evas_Object *)edje_object_part_object_get(row, "playlist_entry_name");
               Evas_Object *clip = evas_object_clip_get(title);
               assert(clip);
               int cx, cw, tx, tw;
               evas_object_geometry_get(clip, &cx, NULL, &cw, NULL);
               edje_object_part_geometry_get(row, "playlist_entry_length", &tx, NULL, &tw, NULL);
               if (cw <= 0 || tw <= 0 || cx + cw >= tx) {
                  fprintf(stderr, "scale %.1f width %d duration %s: title clip ends %d, duration starts %d\n",
                          scales[s], width, durations[d], cx + cw, tx);
                  return 1;
               }
            }
         }
      }
   }
   ecore_evas_free(canvas);
   edje_shutdown(); ecore_evas_shutdown();
}
