#ifndef ECLAIR_UI_H
#define ECLAIR_UI_H

#include <Elementary.h>

/* A private Elementary overlay: custom player themes without UI groups retain
 * the desktop widgets, and no global Elementary settings are changed. */
void eclair_ui_theme_apply(Evas_Object *widget, const char *path);
void eclair_ui_theme_source_set(Evas_Object *widget, const char *path);

#endif
