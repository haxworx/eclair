#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <assert.h>
static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static Eina_Bool render(void *data) { ecore_evas_manual_render(data); return ECORE_CALLBACK_RENEW; }
static void settle(Ecore_Evas *canvas, double seconds)
{
   Ecore_Timer *frames = ecore_timer_add(1.0 / 60, render, canvas);
   ecore_timer_add(seconds, finish, NULL);
   ecore_main_loop_begin();
   ecore_timer_del(frames);
}
static int light_alpha(Evas_Object *layout)
{
   int alpha;
   evas_object_color_get(edje_object_part_object_get(layout, "status_light"), NULL, NULL, NULL, &alpha);
   return alpha;
}
static void send_signal(Evas_Object *layout, const char *name)
{
   edje_object_signal_emit(layout, name, "eclair_bin");
   for (int i = 0; i < 5; i++) edje_object_message_signal_process(layout);
}
int main(int argc, char **argv)
{
   ecore_evas_init(); edje_init();
   Ecore_Evas *canvas = ecore_evas_buffer_new(1100, 1100);
   assert(canvas);
   Evas_Object *layout = edje_object_add(ecore_evas_get(canvas));
   assert(edje_object_file_set(layout, argv[1], "eclair_main_body"));
   int w, h;
   edje_object_size_min_get(layout, &w, &h);
   evas_object_resize(layout, w, h); evas_object_show(layout);
   settle(canvas, .1);
   int idle = light_alpha(layout);
   send_signal(layout, "signal_play"); settle(canvas, 1.6);
   assert(light_alpha(layout) > idle + 80);
   send_signal(layout, "signal_pause"); settle(canvas, 3.0);
   assert(light_alpha(layout) == idle);
   send_signal(layout, "signal_play"); settle(canvas, 1.6);
   assert(light_alpha(layout) > idle + 80);
   send_signal(layout, "signal_stop"); settle(canvas, 3.0);
   assert(light_alpha(layout) == idle);
   ecore_evas_free(canvas); edje_shutdown(); ecore_evas_shutdown();
}
