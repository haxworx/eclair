#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <assert.h>
#include <string.h>

static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static Eina_Bool render(void *data) { ecore_evas_manual_render(data); return ECORE_CALLBACK_RENEW; }
static void settle(Ecore_Evas *canvas, double seconds)
{
   Ecore_Timer *frames = ecore_timer_add(1.0 / 60, render, canvas);
   ecore_timer_add(seconds, finish, NULL);
   ecore_main_loop_begin();
   ecore_timer_del(frames);
}
static int title_x(Evas_Object *row)
{
   int x;
   edje_object_calc_force(row);
   evas_object_geometry_get(edje_object_part_object_get(row, "playlist_entry_name"), &x, NULL, NULL, NULL);
   return x;
}
int main(int argc, char **argv)
{
   ecore_evas_init(); edje_init();
   Ecore_Evas *canvas = ecore_evas_buffer_new(500, 300);
   assert(canvas);
   Evas *evas = ecore_evas_get(canvas);
   Evas_Object *row = edje_object_add(evas);
   for (int i = 1; i < argc; i++) {
      assert(edje_object_file_set(row, argv[i], "eclair_playlist_entry"));
      edje_object_scale_set(row, 1.5);
      evas_object_move(row, 20, 20);
      evas_object_resize(row, 300, 24);
      evas_object_show(row);
      edje_object_part_text_set(row, "playlist_entry_name",
         "A very long artist and track name that must scroll when the pointer enters this playlist row");
      edje_object_part_text_set(row, "playlist_entry_length", "1:23:45");
      settle(canvas, .1);
      int start = title_x(row), dx, dy, dw, dh;
      edje_object_part_geometry_get(row, "playlist_entry_length", &dx, &dy, &dw, &dh);
      evas_event_feed_mouse_in(evas, 0, NULL);
      evas_event_feed_mouse_move(evas, 40, 30, 0, NULL);
      settle(canvas, 2.0);
      assert(title_x(row) < start);
      int x, y, w, h;
      edje_object_part_geometry_get(row, "playlist_entry_length", &x, &y, &w, &h);
      assert(x == dx && y == dy && w == dw && h == dh);
      Evas_Object *clip = evas_object_clip_get(edje_object_part_object_get(row, "playlist_entry_name"));
      evas_object_geometry_get(clip, &x, NULL, &w, NULL);
      assert(x + w < 20 + dx);
      evas_event_feed_mouse_move(evas, 450, 200, 0, NULL);
      settle(canvas, .1);
      assert(title_x(row) == start);
      settle(canvas, 1.0);
      assert(title_x(row) == start);
      /* Leaving before the initial pause expires cancels the queued scroll. */
      evas_event_feed_mouse_move(evas, 40, 30, 0, NULL);
      settle(canvas, .1);
      evas_event_feed_mouse_move(evas, 450, 200, 0, NULL);
      settle(canvas, 1.0);
      assert(title_x(row) == start);
      edje_object_part_text_set(row, "playlist_entry_name", "Short");
      settle(canvas, .1);
      start = title_x(row);
      evas_event_feed_mouse_move(evas, 40, 30, 0, NULL);
      settle(canvas, 2.0);
      assert(title_x(row) == start);
      evas_event_feed_mouse_move(evas, 450, 200, 0, NULL);
   }
   ecore_evas_free(canvas);
   edje_shutdown(); ecore_evas_shutdown();
}
