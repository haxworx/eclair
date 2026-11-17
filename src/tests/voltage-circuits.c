#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

typedef struct { const char *name; double xy[8]; } Route;
static const Route routes[] = {
   {"upper", {24,28, 108,28, 116,22, 185,22}},
   {"lower", {24,122, 160,122, 167,115, 233,115}},
   {"right", {324,64, 324,84, 324,103, 316,112}},
   {"pink", {328,160, 328,185, 317,196, 278,196}}
};
static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static Eina_Bool render(void *data) { ecore_evas_manual_render(data); return ECORE_CALLBACK_RENEW; }
static void settle(Ecore_Evas *canvas, double seconds)
{
   Ecore_Timer *frames = ecore_timer_add(1.0 / 60, render, canvas);
   ecore_timer_add(seconds, finish, NULL); ecore_main_loop_begin(); ecore_timer_del(frames);
}
static void send(Evas_Object **layouts, const char *signal)
{
   for (int i = 0; i < 3; i++) {
      edje_object_signal_emit(layouts[i], signal, "eclair_bin");
      for (int j = 0; j < 5; j++) edje_object_message_signal_process(layouts[i]);
   }
}
static double distance(double x, double y, const double *a, const double *b)
{
   double dx = b[0]-a[0], dy = b[1]-a[1];
   double t = ((x-a[0])*dx + (y-a[1])*dy) / (dx*dx + dy*dy);
   if (t < 0) t = 0;
   if (t > 1) t = 1;
   return hypot(x-a[0]-t*dx, y-a[1]-t*dy);
}
static int check(Evas_Object **layouts, Eina_Bool idle)
{
   int visible = 0;
   for (int i = 0; i < 3; i++) {
      double scale = 1 + .5*i;
      int bx, by;
      edje_object_part_geometry_get(layouts[i], "body", &bx, &by, NULL, NULL);
      for (int r = 0; r < 4; r++) {
         for (int dot = 0; dot < 7; dot++) {
            char name[64];
            snprintf(name, sizeof(name), "circuit_%s_%d", routes[r].name, dot);
            const Evas_Object *part = edje_object_part_object_get(layouts[i], name);
            assert(part && evas_object_pass_events_get(part));
            int alpha;
            evas_object_color_get(part, NULL, NULL, NULL, &alpha);
            if (idle) assert(alpha == 0);
            if (!alpha) continue;
            visible++;
            int x, y, w, h;
            edje_object_part_geometry_get(layouts[i], name, &x, &y, &w, &h);
            double px = (x + .5*w - bx)/scale, py = (y + .5*h - by)/scale;
            double nearest = 1e9;
            for (int segment = 0; segment < 3; segment++) {
               double d = distance(px, py, &routes[r].xy[2*segment], &routes[r].xy[2*segment+2]);
               if (d < nearest) nearest = d;
            }
            /* Every visible light must remain on the existing SVG copper,
             * including while rounding a corner and fading at an endpoint. */
            assert(nearest < 1.5);
         }
      }
   }
   return visible;
}
int main(int argc, char **argv)
{
   ecore_evas_init(); edje_init(); assert(argc == 2);
   Ecore_Evas *canvas = ecore_evas_buffer_new(1400, 1400); assert(canvas);
   Evas_Object *layouts[3];
   for (int i = 0; i < 3; i++) {
      layouts[i] = edje_object_add(ecore_evas_get(canvas));
      assert(edje_object_file_set(layouts[i], argv[1], "eclair_main_body"));
      int w, h;
      edje_object_size_min_get(layouts[i], &w, &h);
      edje_object_scale_set(layouts[i], 1+.5*i);
      evas_object_resize(layouts[i], lround(w*(1+.5*i)), lround(h*(1+.5*i)));
      evas_object_show(layouts[i]);
   }
   settle(canvas, .1); check(layouts, EINA_TRUE);
   send(layouts, "signal_play");
   /* Sample multiple routes, bends and loop restarts at three desktop scales. */
   for (int sample = 0; sample < 12; sample++) {
      settle(canvas, .25);
      check(layouts, EINA_FALSE);
      if (!sample) assert(check(layouts, EINA_FALSE) > 60);
   }
   send(layouts, "signal_pause"); settle(canvas, .1); check(layouts, EINA_TRUE);
   settle(canvas, 1.0); check(layouts, EINA_TRUE);
   send(layouts, "signal_play"); settle(canvas, .1);
   send(layouts, "signal_play"); settle(canvas, .3);
   assert(check(layouts, EINA_FALSE) > 60);
   send(layouts, "signal_stop"); settle(canvas, .1); check(layouts, EINA_TRUE);
   settle(canvas, 1.0); check(layouts, EINA_TRUE);
   send(layouts, "signal_play"); settle(canvas, .1);
   for (int i = 0; i < 3; i++) evas_object_del(layouts[i]);
   settle(canvas, .1);
   ecore_evas_free(canvas); edje_shutdown(); ecore_evas_shutdown();
   return 0;
}
