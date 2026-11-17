#ifndef ECLAIR_TURNTABLE_H
#define ECLAIR_TURNTABLE_H
#include <Evas.h>
/* Optional theme feature, enabled by the turntable group data item. */
void eclair_turntable_attach(Evas_Object *layout);
void eclair_turntable_cover_set(Evas_Object *layout, const char *path);
#endif
