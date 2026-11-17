#ifndef ECLAIR_SCALE_H
#define ECLAIR_SCALE_H

#include <Ecore_Evas.h>

/* Theme and saved dimensions are logical; window dimensions are physical. */
void eclair_scale_init(void);
double eclair_scale_get(void);
int eclair_scale_pixels(int logical);
int eclair_scale_logical(int pixels);
Evas_Object *eclair_scale_edje_add(Evas *evas);

#endif
