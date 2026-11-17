#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT
#include <Ecore_Evas.h>
#include <Edje.h>
#include <Edje_Edit.h>
#include <assert.h>
#include <math.h>
#include <string.h>

typedef struct { int x, y, w, h; } Bounds;
static Bounds bounds(Evas_Object *layout, const char *part)
{
   Bounds b;
   assert(edje_object_part_exists(layout, part));
   edje_object_part_geometry_get(layout, part, &b.x, &b.y, &b.w, &b.h);
   assert(b.w > 0 && b.h > 0);
   return b;
}
static void separate(Bounds a, Bounds b)
{
   assert(a.x + a.w <= b.x || b.x + b.w <= a.x ||
          a.y + a.h <= b.y || b.y + b.h <= a.y);
}
static void received(void *data, Evas_Object *obj, const char *signal, const char *source)
{
   (*(int *)data)++;
}
static void flush(Evas_Object *layout)
{
   for (int i = 0; i < 5; i++) edje_object_message_signal_process(layout);
}
static void click(Ecore_Evas *canvas, Evas_Object *layout, const char *part)
{
   Bounds b = bounds(layout, part);
   Evas *evas = ecore_evas_get(canvas);
   evas_event_feed_mouse_in(evas, 0, NULL);
   evas_event_feed_mouse_move(evas, b.x + b.w/2, b.y + b.h/2, 0, NULL);
   evas_event_feed_mouse_down(evas, 1, EVAS_BUTTON_NONE, 0, NULL);
   evas_event_feed_mouse_up(evas, 1, EVAS_BUTTON_NONE, 0, NULL);
   flush(layout);
}
int main(int argc, char **argv)
{
   ecore_evas_init(); edje_init();
   const char *parts[] = {"button_play", "button_stop", "button_prev", "button_next", "button_open", "button_repeat", "button_shuffle", "button_close", "button_minimize"};
   const char *signals[] = {"eclair_play", "eclair_stop", "eclair_prev", "eclair_next", "eclair_open", "eclair_repeat_toggle", "eclair_shuffle_toggle", "eclair_main_close", "eclair_main_minimize"};
   for (int theme = 1; theme < argc; theme++) {
      Ecore_Evas *canvas = ecore_evas_buffer_new(1100, 1100);
      assert(canvas);
      Evas_Object *layout = edje_edit_object_add(ecore_evas_get(canvas));
      assert(edje_object_file_set(layout, argv[theme], "eclair_main_body"));
      Eina_List *list = edje_edit_parts_list_get(layout), *l;
      char *name;
      int vectors = 0, shadows = 0;
      EINA_LIST_FOREACH(list, l, name) {
         Edje_Part_Type type = edje_edit_part_type_get(layout, name);
         if (type == EDJE_PART_TYPE_IMAGE) {
            /* PNGs are allowed only for the three soft silhouette shadows. */
            assert(!strcmp(name, "body_drop") || !strcmp(name, "artwork_drop") ||
                   !strcmp(name, "drawer_drop"));
            assert(!edje_edit_part_mouse_events_get(layout, name));
            shadows++;
         }
         if (type == EDJE_PART_TYPE_VECTOR) vectors++;
      }
      assert(vectors >= 20);
      assert(shadows == 1);
      edje_edit_string_list_free(list);
      for (int factor = 2; factor <= 4; factor++) {
         double scale = factor / 2.0;
         edje_object_scale_set(layout, scale);
         int width, height;
         edje_object_size_min_get(layout, &width, &height);
         evas_object_resize(layout, width * scale, height * scale);
         evas_object_show(layout);
         flush(layout);
         edje_object_calc_force(layout);
         ecore_evas_manual_render(canvas);
         Bounds surface = bounds(layout, "player_surface");
         Bounds shadow = bounds(layout, "body_drop");
         assert(shadow.x >= 0 && shadow.y >= 0);
         assert(shadow.x + shadow.w <= width * scale + 1);
         assert(shadow.y + shadow.h <= height * scale + 1);
         assert(!edje_object_part_exists(layout, "playlist_button"));
         assert(!edje_object_part_exists(layout, "left_border"));
         Bounds queue = bounds(layout, "playlist_container");
         assert(queue.x >= surface.x && queue.x + queue.w <= surface.x + surface.w);
         assert(queue.y + queue.h <= surface.y + surface.h);
         separate(bounds(layout, "body"), queue);
         Bounds artwork = bounds(layout, "artwork_pane");
         Bounds cover = bounds(layout, "cover");
         separate(bounds(layout, "body"), artwork);
         separate(artwork, queue);
         assert(artwork.y < queue.y && cover.y >= artwork.y);
         assert(cover.y + cover.h <= artwork.y + artwork.h);
         assert(abs(cover.w - 156 * scale) <= 1 && cover.w == cover.h);
         Bounds repeat = bounds(layout, "button_repeat");
         Bounds shuffle = bounds(layout, "button_shuffle");
         assert(abs(shuffle.w - 24*scale) <= 1 && abs(shuffle.h - 20*scale) <= 1);
         assert(shuffle.y == repeat.y && shuffle.h == repeat.h);
         assert(abs(shuffle.x - repeat.x - repeat.w - 4*scale) <= 1);
         separate(repeat, shuffle);
         separate(shuffle, bounds(layout, "volume_bar_track"));
         assert(!edje_object_part_exists(layout, "button_repeat_label"));
         assert(!edje_object_part_exists(layout, "button_shuffle_label"));
         Bounds volume = bounds(layout, "volume_bar_confine");
         assert(abs((2*repeat.y + repeat.h) - (2*volume.y + volume.h)) <= 2);
         assert(abs(repeat.w - 24*scale) <= 1 && abs(repeat.h - 20*scale) <= 1);
         separate(repeat, bounds(layout, "volume_bar_track"));
         separate(bounds(layout, "info_bar"), bounds(layout, "transport"));
         separate(bounds(layout, "info_bar"), bounds(layout, "button_close"));
         for (int i = 0; i < 9; i++) {
            int count = 0;
            edje_object_signal_callback_add(layout, signals[i], "*", received, &count);
            click(canvas, layout, parts[i]);
            assert(count == 1);
            edje_object_signal_callback_del_full(layout, signals[i], "*", received, &count);
         }
         /* Playing replaces the center glyph and routes its click to pause. */
         int pause = 0, play = 0;
         edje_object_signal_callback_add(layout, "eclair_pause", "*", received, &pause);
         edje_object_signal_callback_add(layout, "eclair_play", "*", received, &play);
         edje_object_signal_emit(layout, "signal_play", "eclair_bin"); flush(layout);
         assert(!strcmp(edje_object_part_state_get(layout, "button_pause", NULL), "play"));
         click(canvas, layout, "button_play");
         assert(pause == 1 && play == 0);
         edje_object_signal_emit(layout, "signal_pause", "eclair_bin"); flush(layout);
         click(canvas, layout, "button_play");
         assert(pause == 1 && play == 1);
         edje_object_signal_callback_del_full(layout, "eclair_pause", "*", received, &pause);
         edje_object_signal_callback_del_full(layout, "eclair_play", "*", received, &play);
      }
      evas_object_del(layout);
      ecore_evas_free(canvas);
   }
   edje_shutdown(); ecore_evas_shutdown();
}
