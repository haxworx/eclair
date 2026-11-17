#include <Ecore_Evas.h>
#include <Edje.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

static void signal_send(Evas_Object *layout, const char *name)
{
   edje_object_signal_emit(layout, name, "eclair_bin");
   for (int i = 0; i < 5; i++) edje_object_message_signal_process(layout);
}
int main(int argc, char **argv)
{
   ecore_evas_init(); edje_init();
   Ecore_Evas *canvas = ecore_evas_buffer_new(1100, 1100);
   assert(canvas);
   for (int factor = 2; factor <= 4; factor++) {
      double scale = factor / 2.;
      Evas_Object *layout = edje_object_add(ecore_evas_get(canvas));
      assert(edje_object_file_set(layout, argv[1], "eclair_main_body"));
      edje_object_scale_set(layout, scale);
      evas_object_resize(layout, 518*scale, 362*scale);
      evas_object_show(layout);
      signal_send(layout, "signal_play");
      int first = 0;
      for (int step = 0; step < 3; step++) {
         double progress = step / 2.;
         edje_object_part_drag_value_set(layout, "progress_bar_drag", progress, 0);
         edje_object_calc_force(layout);
         int x, y, w, h, cx, cy, cw, ch;
         edje_object_part_geometry_get(layout, "progress_bar_drag", &x, &y, &w, &h);
         edje_object_part_geometry_get(layout, "progress_bar_confine", &cx, &cy, &cw, &ch);
         assert(abs(w - 14*scale) <= 1 && abs(h - 64*scale) <= 1);
         assert(abs(x - (cx + progress*(cw-w))) <= 1);
         assert(y == cy);
         if (!step) first = x;
         else assert(x > first);
      }
      signal_send(layout, "signal_pause");
      double value;
      edje_object_part_drag_value_get(layout, "progress_bar_drag", &value, NULL);
      assert(fabs(value - 1.) < .001);
      signal_send(layout, "signal_stop");
      edje_object_part_drag_value_get(layout, "progress_bar_drag", &value, NULL);
      assert(value == 0.);
      assert(evas_object_visible_get(edje_object_part_object_get(layout, "progress_bar_drag")));
      evas_object_del(layout);
   }
   ecore_evas_free(canvas); edje_shutdown(); ecore_evas_shutdown();
}
