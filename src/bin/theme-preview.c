#include "eclair_turntable.h"
/* Render the real Edje layout without a display server.
 * Usage: theme-preview THEME.edj OUTPUT.png SCALE
 */
#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <Edje_Edit.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static Eina_Bool finish_preview(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static Eina_Bool render_preview(void *data) { ecore_evas_manual_render(data); return ECORE_CALLBACK_RENEW; }

int main(int argc, char **argv)
{
   if (argc != 4) {
      fprintf(stderr, "Usage: %s THEME.edj OUTPUT.png SCALE\n", argv[0]);
      return 2;
   }
   double scale = atof(argv[3]);
   if (scale < .5 || scale > 4) return 2;
   int w = lround(438 * scale), h = lround(260 * scale);
   ecore_evas_init(); edje_init();
   Ecore_Evas *canvas = ecore_evas_buffer_new(w, h);
   if (!canvas) return 1;
   ecore_evas_alpha_set(canvas, EINA_TRUE);
   Evas *evas = ecore_evas_get(canvas);
   Evas_Object *layout = edje_edit_object_add(evas);
   if (!edje_object_file_set(layout, argv[1], "eclair_main_body")) return 1;
   edje_object_size_min_get(layout, &w, &h);
   w = lround(w * scale); h = lround(h * scale);
   ecore_evas_resize(canvas, w, h);
   edje_object_scale_set(layout, scale);
   evas_object_resize(layout, w, h);
   evas_object_show(layout);
   edje_object_message_signal_process(layout);
   Evas_Object *cover = evas_object_image_filled_add(evas);
   evas_object_image_file_set(cover, "data/themes/folio/images/cover.svg", NULL);
   evas_object_size_hint_aspect_set(cover, EVAS_ASPECT_CONTROL_BOTH, 88, 87);
   edje_object_part_swallow(layout, "cover", cover);
   evas_object_show(cover);
   edje_object_signal_emit(layout, "signal_cover_set", "eclair_bin");
   eclair_turntable_attach(layout);
   eclair_turntable_cover_set(layout, "data/themes/folio/images/cover.svg");
   edje_object_signal_emit(layout, "signal_repeat_enabled", "eclair_bin");
   edje_object_signal_emit(layout, "signal_play", "eclair_bin");
   edje_object_message_signal_process(layout);
   edje_object_part_drag_value_set(layout, "volume_bar_drag", .6, 0);
   edje_object_part_drag_value_set(layout, "progress_bar_drag", .35, 0);
   edje_object_part_text_set(layout, "current_media_name", "Morning Light — Eclair");
   edje_object_part_text_set(layout, "time_elapsed", "1:24");
   edje_object_calc_force(layout);
   {
      int x, y, pw, ph;
      edje_object_part_geometry_get(layout, "playlist_container", &x, &y, &pw, &ph);
      const char *titles[] = {"Morning Light", "A Quiet Place", "Golden Hour", "Home Again"};
      const char *times[] = {"4:00", "3:42", "5:16", "3:08"};
      for (int i = 0; i < 4; i++) {
         Evas_Object *row = edje_object_add(evas);
         edje_object_file_set(row, argv[1], "eclair_playlist_entry");
         edje_object_scale_set(row, scale);
         edje_object_part_text_set(row, "playlist_entry_name", titles[i]);
         edje_object_part_text_set(row, "playlist_entry_length", times[i]);
         evas_object_move(row, x, y + lround(i * 16 * scale));
         evas_object_resize(row, pw, lround(16 * scale));
         evas_object_show(row);
      }
   }
   /* Let playback-driven parts reach their pose before rendering. */
   Ecore_Timer *frames = ecore_timer_add(1.0 / 60, render_preview, canvas);
   ecore_timer_add(.75, finish_preview, NULL);
   ecore_main_loop_begin();
   ecore_timer_del(frames);
   ecore_evas_manual_render(canvas);
   Evas_Object *image = evas_object_image_add(evas);
   evas_object_image_alpha_set(image, EINA_TRUE);
   evas_object_image_size_set(image, w, h);
   evas_object_image_data_copy_set(image, (void *)ecore_evas_buffer_pixels_get(canvas));
   Eina_Bool saved = evas_object_image_save(image, argv[2], NULL, NULL);
   ecore_evas_free(canvas); edje_shutdown(); ecore_evas_shutdown();
   return saved ? 0 : 1;
}
