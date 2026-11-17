#include "eclair_scale.h"
#include <Elementary.h>
#include <Edje.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv)
{
   elm_init(argc, argv);
   eclair_scale_init();
   double expected = atof(argv[1]);
   if (fabs(eclair_scale_get() - expected) > 0.001) return 1;
   int width = eclair_scale_pixels(200), height = eclair_scale_pixels(100);
   Ecore_Evas *window = ecore_evas_buffer_new(width, height);
   if (!window) return 2;
   Evas *evas = ecore_evas_get(window);
   Evas_Object *row = eclair_scale_edje_add(evas);
   if (!edje_object_file_set(row, argv[2], "eclair_playlist_entry")) return 3;
   edje_object_part_text_set(row, "playlist_entry_name", "Sample track");
   evas_object_resize(row, width, eclair_scale_pixels(16));
   evas_object_show(row);
   edje_object_calc_force(row);
   const Evas_Object *text = edje_object_part_object_get(row, "playlist_entry_name");
   int font_size = 0;
   evas_object_text_font_get(text, NULL, &font_size);
   /* The font is rasterized at the scaled size, directly on the native canvas. */
   if (fabs(font_size * evas_object_scale_get(text) - 10 * expected) > 0.01) return 4;
   if (evas_object_evas_get(text) != evas) return 5;
   ecore_evas_manual_render(window);
   int output_w, output_h;
   evas_output_size_get(evas, &output_w, &output_h);
   if (output_w != width || output_h != height) return 6;
   const uint32_t *pixels = ecore_evas_buffer_pixels_get(window);
   int visible = 0;
   for (int i = 0; i < width * height; i++) if (pixels[i] & 0x00ffffff) visible++;
   if (!visible) return 7;
   ecore_evas_free(window);
   elm_shutdown();
   return 0;
}
