#define EFL_BETA_API_SUPPORT
#include "eclair_turntable.h"
#include <Edje.h>
#include <Ecore.h>
#include <Efl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
   Evas_Object *layout, *label;
   Ecore_Animator *animator;
   Evas_Vg_Node *record, *arm;
   Eina_Matrix3 record_base, arm_base;
   double angle, arm_angle, last, artwork_fade;
   Eina_Bool playing, has_cover;
} Turntable;

/* Rotate drawing nodes, never mapped VECTOR canvas objects. Each node is a
 * private copy: the SVG loader's cached drawing trees may be shared. */
static Evas_Vg_Node *drawing(Evas_Object *layout, const char *part)
{
   Evas_Object *vector = (Evas_Object *)edje_object_part_object_get(layout, part);
   Evas_Vg_Node *root = vector ? evas_object_vg_root_node_get(vector) : NULL;
   if (!root) return NULL;
   Evas_Vg_Node *copy = efl_duplicate(root);
   if (!copy) return NULL;
   evas_object_vg_root_node_set(vector, copy);
   efl_unref(copy);
   return evas_object_vg_root_node_get(vector);
}
static void rotate(Evas_Vg_Node *node, const Eina_Matrix3 *base, double x, double y, double degrees)
{
   if (!node) return;
   double a = degrees * M_PI / 180.0, c = cos(a), s = sin(a);
   Eina_Matrix3 rotation = {c, -s, x - c*x + s*y,
                            s, c, y - s*x - c*y, 0, 0, 1}, result;
   eina_matrix3_compose(base, &rotation, &result);
   evas_vg_node_transformation_set(node, &result);
}
static void paint(Turntable *t)
{
   if (!t->record) t->record = drawing(t->layout, "platter");
   if (!t->arm) t->arm = drawing(t->layout, "tonearm");
   int rw, rh, aw, ah;
   edje_object_part_geometry_get(t->layout, "platter", NULL, NULL, &rw, &rh);
   edje_object_part_geometry_get(t->layout, "tonearm", NULL, NULL, &aw, &ah);
   eina_matrix3_identity(&t->record_base);
   eina_matrix3_scale(&t->record_base, rw / 184., rh / 184.);
   eina_matrix3_identity(&t->arm_base);
   eina_matrix3_scale(&t->arm_base, aw / 180., ah / 180.);
   rotate(t->record, &t->record_base, 92, 92, t->angle);
   rotate(t->arm, &t->arm_base, 105, 20, t->arm_angle);
   /* Fade static artwork in only after the record and arm are at rest. */
   if (t->has_cover && !t->playing && t->arm_angle == -16 && t->artwork_fade > 0) {
      int alpha = lround(255 * t->artwork_fade);
      evas_object_color_set(t->label, alpha, alpha, alpha, alpha);
      evas_object_show(t->label);
   } else {
      /* Edje can show swallowed objects again when the sleeve recalculates.
       * Transparent colour keeps the artwork hidden through those changes. */
      evas_object_color_set(t->label, 0, 0, 0, 0);
      evas_object_hide(t->label);
   }
}
static Eina_Bool tick(void *data)
{
   Turntable *t = data;
   double now = ecore_loop_time_get(), dt = now - t->last;
   t->last = now;
   if (t->playing) t->angle = fmod(t->angle + dt * 30.0, 360.0);
   Eina_Bool was_at_rest = !t->playing && t->arm_angle == -16;
   double target = t->playing ? 0 : -16, step = dt * 32;
   if (fabs(target - t->arm_angle) <= step) t->arm_angle = target;
   else t->arm_angle += target > t->arm_angle ? step : -step;
   if (was_at_rest && t->has_cover)
      t->artwork_fade = fmin(1., t->artwork_fade + dt / .35);
   paint(t);
   if (!t->playing && t->arm_angle == target &&
       (!t->has_cover || t->artwork_fade >= 1.)) {
      t->animator = NULL;
      return ECORE_CALLBACK_CANCEL;
   }
   return ECORE_CALLBACK_RENEW;
}
static void playback(void *data, Evas_Object *obj, const char *signal, const char *source)
{
   Turntable *t = data;
   t->playing = !strcmp(signal, "signal_play");
   if (t->playing) t->artwork_fade = 0;
   if (!strcmp(signal, "signal_stop")) t->angle = 0;
   t->last = ecore_loop_time_get();
   if (!t->animator) t->animator = ecore_animator_add(tick, t);
   paint(t);
}
static void geometry_changed(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   paint(data);
}
static void destroy(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Turntable *t = data;
   if (t->animator) ecore_animator_del(t->animator);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_RESIZE, geometry_changed, t);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_MOVE, geometry_changed, t);
   evas_object_data_del(obj, "turntable");
   evas_object_del(t->label);
   free(t);
}
void eclair_turntable_attach(Evas_Object *layout)
{
   const char *enabled = edje_object_data_get(layout, "turntable");
   if (!enabled || strcmp(enabled, "1") || evas_object_data_get(layout, "turntable")) return;
   Turntable *t = calloc(1, sizeof(*t));
   if (!t) return;
   t->layout = layout;
   t->arm_angle = -16;
   t->label = evas_object_image_filled_add(evas_object_evas_get(layout));
   evas_object_pass_events_set(t->label, EINA_TRUE);
   edje_object_part_swallow(layout, "record_cover", t->label);
   evas_object_data_set(layout, "turntable", t);
   evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, destroy, t);
   evas_object_event_callback_add(layout, EVAS_CALLBACK_RESIZE, geometry_changed, t);
   evas_object_event_callback_add(layout, EVAS_CALLBACK_MOVE, geometry_changed, t);
   edje_object_signal_callback_add(layout, "signal_play", "*", playback, t);
   edje_object_signal_callback_add(layout, "signal_pause", "*", playback, t);
   edje_object_signal_callback_add(layout, "signal_stop", "*", playback, t);
   paint(t);
}
void eclair_turntable_cover_set(Evas_Object *layout, const char *path)
{
   Turntable *t = evas_object_data_get(layout, "turntable");
   if (!t) return;
   evas_object_hide(t->label);
   evas_object_color_set(t->label, 0, 0, 0, 0);
   t->has_cover = EINA_FALSE;
   if (!path) return;
   Evas_Object *source = evas_object_image_add(evas_object_evas_get(layout));
   evas_object_image_load_size_set(source, 512, 512);
   evas_object_image_file_set(source, path, NULL);
   int w, h;
   evas_object_image_size_get(source, &w, &h);
   const unsigned int *pixels = evas_object_image_data_get(source, EINA_FALSE);
   if (pixels && w > 0 && h > 0) {
      /* Crop to a square and antialias the circular paper label. Album art
       * remains user media; all decorative theme assets are native SVG. */
      const int size = 256;
      unsigned int *label = calloc(size * size, sizeof(*label));
      if (label) {
         int side = w < h ? w : h;
         for (int y = 0; y < size; y++) for (int x = 0; x < size; x++) {
            double dx = x + .5 - size/2., dy = y + .5 - size/2.;
            double alpha = fmin(1., fmax(0., size/2. - hypot(dx, dy)));
            unsigned int p = pixels[((h-side)/2 + y*side/size)*w + (w-side)/2 + x*side/size];
            unsigned int a = (p >> 24) * alpha, r = ((p >> 16)&255) * alpha;
            unsigned int g = ((p >> 8)&255) * alpha, b = (p&255) * alpha;
            label[y*size+x] = (a<<24)|(r<<16)|(g<<8)|b;
         }
         evas_object_image_size_set(t->label, size, size);
         evas_object_image_alpha_set(t->label, EINA_TRUE);
         evas_object_image_data_copy_set(t->label, label);
         free(label);
         t->has_cover = EINA_TRUE;
         t->artwork_fade = !t->playing && t->arm_angle == -16 ? 1. : 0.;
      }
   }
   evas_object_del(source);
   paint(t);
}
