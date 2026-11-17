#include "eclair.h"
#include "eclair_window.h"
#include "eclair_scale.h"
#include <assert.h>

static Eina_Bool check_geometry;
static int initial_x, initial_y, initial_w, initial_h;
static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static Eina_Bool render(void *data)
{
   Eclair_Window *window = data;
   if (check_geometry) {
      int x, y, w, h;
      ecore_evas_request_geometry_get(ecore_evas_ecore_evas_get(window->evas), &x, &y, &w, &h);
      assert(x == initial_x && y == initial_y && w == initial_w && h == initial_h);
      evas_object_geometry_get(window->edje_object, &x, &y, &w, &h);
      assert(x == 0 && y == 0 && w == initial_w && h == initial_h);
   }
   elm_win_render(window->window);
   return ECORE_CALLBACK_RENEW;
}
static void settle(Eclair_Window *window)
{
   Ecore_Timer *frames = ecore_timer_add(1.0 / 60, render, window);
   ecore_timer_add(1.15, finish, NULL);
   ecore_main_loop_begin();
   ecore_timer_del(frames);
}
static void toggle(Eclair_Window *window, const char *part)
{
   edje_object_signal_emit(window->edje_object, "mouse,clicked,1", part);
   edje_object_message_signal_process(window->edje_object);
   settle(window);
}
int main(int argc, char **argv)
{
   elm_init(argc, argv);
   eclair_scale_init();
   for (int i = 1; i < argc; i++) {
      Eclair app = {0};
      app.config.properties = eina_hash_string_superfast_new(free);
      Eclair_Window *window = eclair_window_create(argv[i], "main", "Stable window test", ECLAIR_SOFTWARE, &app, EINA_TRUE);
      assert(window);
      app.gui_window = window;
      eclair_window_add_default_callbacks(window, &app);
      settle(window);
      ecore_evas_request_geometry_get(ecore_evas_ecore_evas_get(window->evas),
                                     &initial_x, &initial_y, &initial_w, &initial_h);
      assert(initial_w == eclair_scale_pixels(window->layout_width));
      assert(initial_h == eclair_scale_pixels(window->layout_height));
      check_geometry = EINA_TRUE;
      assert(!edje_object_part_exists(window->edje_object, "playlist_button"));
      assert(!edje_object_part_exists(window->edje_object, "left_border"));
      int qx, qy, qw, qh;
      edje_object_part_geometry_get(window->edje_object, "playlist_container", &qx, &qy, &qw, &qh);
      assert(qw > 0 && qh > eclair_scale_pixels(100));
      /* Legacy clicks and cover updates cannot move or hide the queue. */
      toggle(window, "playlist_button_close");
      toggle(window, "playlist_button");
      edje_object_signal_emit(window->edje_object, "signal_cover_set", "eclair_bin");
      edje_object_signal_emit(window->edje_object, "signal_cover_unset", "eclair_bin");
      settle(window);
      int x, y, pw, ph;
      edje_object_part_geometry_get(window->edje_object, "playlist_container", &x, &y, &pw, &ph);
      assert(x == qx && y == qy && pw == qw && ph == qh);
      int layout_w = window->layout_width, layout_h = window->layout_height, w, h;
      check_geometry = EINA_FALSE;
      eclair_window_del(window);
      assert(eclair_config_get_prop_int(&app.config, "main_window", "w", &w) && w == layout_w);
      assert(eclair_config_get_prop_int(&app.config, "main_window", "h", &h) && h == layout_h);
      window = eclair_window_create(argv[i], "main", "Reopen stable window", ECLAIR_SOFTWARE, &app, EINA_TRUE);
      assert(window);
      app.gui_window = window;
      settle(window);
      evas_object_geometry_get(window->window, NULL, NULL, &w, &h);
      assert(w == initial_w && h == initial_h);
      edje_object_signal_emit(window->edje_object, "mouse,clicked,1", "playlist_button");
      edje_object_message_signal_process(window->edje_object);
      eclair_window_del(window);
      eina_hash_free(app.config.properties);
   }
   elm_shutdown();
   return 0;
}
