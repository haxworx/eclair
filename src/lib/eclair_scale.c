#include "eclair_scale.h"
#include <Elementary.h>
#include <Edje.h>
#include <math.h>
#include <stdlib.h>

static double scale = 1.0;

void eclair_scale_init(void)
{
   /* Elementary owns desktop DPI/profile/backend scaling on both displays. */
   scale = elm_config_scale_get();
   if (!isfinite(scale) || scale <= 0.0) scale = 1.0;
}

double eclair_scale_get(void)
{
   return scale;
}

int eclair_scale_pixels(int logical)
{
   return (int)lround(logical * scale);
}

int eclair_scale_logical(int pixels)
{
   return (int)lround(pixels / scale);
}

/* Text and theme geometry are rendered directly at the window's pixel size. */
Evas_Object *eclair_scale_edje_add(Evas *evas)
{
   Evas_Object *object = edje_object_add(evas);
   edje_object_scale_set(object, scale);
   return object;
}
