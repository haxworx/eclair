#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <assert.h>
#include <string.h>

static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static Eina_Bool render(void *data) { ecore_evas_manual_render(data); return ECORE_CALLBACK_RENEW; }
static void settle(Ecore_Evas *canvas, double seconds)
{
   Ecore_Timer *frames = ecore_timer_add(1.0/60, render, canvas);
   ecore_timer_add(seconds, finish, NULL); ecore_main_loop_begin(); ecore_timer_del(frames);
}
static void signal_send(Evas_Object *layout, const char *name)
{
   edje_object_signal_emit(layout, name, "eclair_bin");
   for (int i=0; i<5; i++) edje_object_message_signal_process(layout);
}
static int width(Evas_Object *layout)
{
   int w;
   edje_object_part_geometry_get(layout, "status_light", NULL, NULL, &w, NULL);
   return w;
}
static void idle_check(Evas_Object *layout)
{
   assert(!strcmp(edje_object_part_state_get(layout, "status_light", NULL), "default"));
   int alpha;
   const Evas_Object *echo = edje_object_part_object_get(layout, "status_light");
   evas_object_color_get(echo, NULL, NULL, NULL, &alpha);
   assert(alpha == 0 && !evas_object_map_enable_get(echo));
}
int main(int argc, char **argv)
{
   ecore_evas_init(); edje_init();
   Ecore_Evas *canvas = ecore_evas_buffer_new(478,462);
   Evas_Object *layout = edje_object_add(ecore_evas_get(canvas));
   assert(edje_object_file_set(layout,argv[1],"eclair_main_body"));
   evas_object_resize(layout,478,462); evas_object_show(layout); settle(canvas,.1);
   idle_check(layout);
   int rest = width(layout);
   signal_send(layout,"signal_play"); settle(canvas,.6);
   int first = width(layout); assert(first > rest+10);
   settle(canvas,.6); assert(width(layout) > first);
   settle(canvas,2.0); assert(width(layout) < 200); /* A fresh echo has begun. */
   signal_send(layout,"signal_pause"); settle(canvas,.6); idle_check(layout);
   settle(canvas,.6); idle_check(layout);
   signal_send(layout,"signal_play"); settle(canvas,.6); assert(width(layout)>rest+10);
   signal_send(layout,"signal_stop"); settle(canvas,.6); idle_check(layout);
   signal_send(layout,"signal_play"); settle(canvas,.1);
   evas_object_del(layout); settle(canvas,.1);
   ecore_evas_free(canvas); edje_shutdown(); ecore_evas_shutdown();
}
