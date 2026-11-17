#include "eclair_turntable.h"
#include <math.h>
#include <stdlib.h>
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
   ecore_timer_add(seconds, finish, NULL); ecore_main_loop_begin(); ecore_timer_del(frames);
}
static void send_signal(Evas_Object *layout, const char *name)
{
   edje_object_signal_emit(layout, name, "eclair_bin");
   for (int i = 0; i < 5; i++) edje_object_message_signal_process(layout);
}
static int artwork_alpha(Evas_Object *label)
{
   int alpha;
   evas_object_color_get(label, NULL, NULL, NULL, &alpha);
   return evas_object_visible_get(label) ? alpha : 0;
}
static Eina_Bool artwork_visible(Evas_Object *label) { return artwork_alpha(label) > 0; }
static double angle(Evas_Object *layout, const char *part)
{
   const Evas_Object *vector = edje_object_part_object_get(layout, part);
   Evas_Vg_Node *root = evas_object_vg_root_node_get(vector);
   assert(root && !evas_object_map_enable_get(vector));
   const Eina_Matrix3 *matrix = evas_vg_node_transformation_get(root);
   assert(matrix);
   return atan2(matrix->yx, matrix->xx) * 180 / M_PI;
}
int main(int argc, char **argv)
{
   ecore_evas_init(); edje_init();
   double scale = atof(argv[3]);
   Ecore_Evas *canvas = ecore_evas_buffer_new(1100, 1100);
   assert(canvas);
   Evas_Object *layout = edje_object_add(ecore_evas_get(canvas));
   assert(edje_object_file_set(layout, argv[1], "eclair_main_body"));
   edje_object_scale_set(layout, scale);
   evas_object_resize(layout, 438*scale, 480*scale); evas_object_show(layout);
   edje_object_part_text_set(layout, "current_media_name", "Morning Light — Eclair");
   edje_object_part_text_set(layout, "time_elapsed", "1:24");
   eclair_turntable_attach(layout);
   settle(canvas, .1);
   assert(fabs(angle(layout, "tonearm") + 16) < .1);
   const Evas_Object *platter = edje_object_part_object_get(layout, "platter");
   const Eina_Matrix3 *transform = evas_vg_node_transformation_get(evas_object_vg_root_node_get(platter));
   assert(fabs(transform->xx - scale) < .01);
   eclair_turntable_cover_set(layout, argv[2]);
   Evas_Object *label = edje_object_part_swallow_get(layout, "record_cover");
   assert(label && artwork_visible(label));
   /* The label is cropped to a circle, with no opaque square corners. */
   int w, h;
   evas_object_image_size_get(label, &w, &h);
   unsigned int *pixels = evas_object_image_data_get(label, EINA_FALSE);
   assert(pixels && w == 256 && h == 256);
   assert(!(pixels[0] >> 24) && (pixels[(h/2)*w+w/2] >> 24));
   send_signal(layout, "signal_play");
   assert(!artwork_visible(label));
   settle(canvas, .7);
   double playing = angle(layout, "platter");
   assert(playing > 15 && playing < 35);
   assert(fabs(angle(layout, "tonearm")) < .1);
   assert(!artwork_visible(label));
   assert(!evas_object_map_enable_get(label));
   /* Art and sleeve transitions may change while the record is turning. */
   eclair_turntable_cover_set(layout, NULL);
   assert(!artwork_visible(label));
   eclair_turntable_cover_set(layout, argv[2]);
   send_signal(layout, "signal_cover_set"); settle(canvas, .2);
   assert(!artwork_visible(label));
   assert(angle(layout, "platter") > playing + 3);
   send_signal(layout, "signal_play"); send_signal(layout, "signal_play");
   send_signal(layout, "signal_pause"); settle(canvas, .2);
   double paused = angle(layout, "platter");
   assert(!artwork_visible(label)); /* Still lifting the arm. */
   settle(canvas, .4);
   assert(fabs(angle(layout, "tonearm") + 16) < .1);
   assert(artwork_alpha(label) > 0 && artwork_alpha(label) < 255);
   settle(canvas, .4);
   assert(artwork_alpha(label) == 255);
   assert(fabs(angle(layout, "platter") - paused) < .01);
   send_signal(layout, "signal_play");
   assert(!artwork_visible(label)); /* Resume immediately reveals the SVG label. */
   settle(canvas, .6);
   assert(angle(layout, "platter") > paused + 3);
   send_signal(layout, "signal_stop");
   assert(!artwork_visible(label)); /* Wait for the arm to finish lifting. */
   settle(canvas, .2);
   assert(!artwork_visible(label));
   settle(canvas, .4);
   assert(fabs(angle(layout, "tonearm") + 16) < .1);
   assert(artwork_alpha(label) > 0 && artwork_alpha(label) < 255);
   settle(canvas, .4);
   assert(artwork_alpha(label) == 255);
   assert(fabs(angle(layout, "platter")) < .01);
   assert(fabs(angle(layout, "tonearm") + 16) < .1);
   eclair_turntable_cover_set(layout, NULL);
   assert(!artwork_visible(label));
   eclair_turntable_cover_set(layout, "/missing-cover.png");
   assert(!artwork_visible(label));
   eclair_turntable_cover_set(layout, argv[2]);
   assert(artwork_visible(label));
   /* Other instances must not inherit mutations to a cached SVG drawing. */
   Evas_Object *other = edje_object_add(ecore_evas_get(canvas));
   assert(edje_object_file_set(other, argv[1], "eclair_main_body"));
   evas_object_resize(other, 438, 480);
   eclair_turntable_attach(other);
   send_signal(layout, "signal_play"); settle(canvas, .1);
   assert(fabs(angle(other, "platter")) < .01);
   evas_object_del(other);
   /* Theme switches may delete a layout while both animations are active. */
   evas_object_del(layout); settle(canvas, .2);
   ecore_evas_free(canvas); edje_shutdown(); ecore_evas_shutdown();
}
