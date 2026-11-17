#include "eclair.h"
#include "eclair_window.h"
#include <assert.h>
#include <Ecore_Evas.h>

static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static Eina_Bool render(void *data) { ecore_evas_manual_render(data); return ECORE_CALLBACK_RENEW; }
static void settle(Ecore_Evas *canvas, double seconds)
{
   Ecore_Timer *frames = ecore_timer_add(1.0 / 60, render, canvas);
   ecore_timer_add(seconds, finish, NULL);
   ecore_main_loop_begin();
   ecore_timer_del(frames);
}
static int title_x(Evas_Object *layout)
{
   int x;
   edje_object_calc_force(layout);
   evas_object_geometry_get(edje_object_part_object_get(layout, "current_media_name"), &x, NULL, NULL, NULL);
   return x;
}
int main(int argc, char **argv)
{
   ecore_evas_init(); edje_init();
   Ecore_Evas *canvas = ecore_evas_buffer_new(1000, 1000);
   assert(canvas);
   Evas_Object *layout = edje_object_add(ecore_evas_get(canvas));
   Eclair_Window window = {.edje_object = layout};
   Eclair app = {.gui_window = &window};
   for (int i = 1; i < argc; i++) {
      assert(edje_object_file_set(layout, argv[i], "eclair_main_body"));
      edje_object_scale_set(layout, 1.5);
      evas_object_resize(layout, 657, 390);
      evas_object_show(layout);
      eclair_all_windows_text_set(&app, "current_media_name", "Short title");
      settle(canvas, .1);
      int stationary = title_x(layout);
      settle(canvas, 2.5);
      assert(title_x(layout) == stationary);
      eclair_all_windows_text_set(&app, "current_media_name",
         "An artist with a very long name — a track title that is much too wide to fit in the player display");
      settle(canvas, .1);
      int start = title_x(layout);
      settle(canvas, 3.0);
      assert(title_x(layout) < start);
      /* A replacement title resets to the start, then stays still if it fits. */
      eclair_all_windows_text_set(&app, "current_media_name", "New song");
      settle(canvas, .1);
      int reset = title_x(layout);
      settle(canvas, 2.5);
      assert(title_x(layout) == reset);
   }
   ecore_evas_free(canvas);
   edje_shutdown(); ecore_evas_shutdown();
}
