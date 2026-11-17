#include <stdlib.h>

#include <Edje.h>

#include "eclair_compat.h"
#include "eclair_ui.h"

static void _theme_deleted(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   eina_stringshare_del(data);
}

void eclair_ui_theme_source_set(Evas_Object *widget, const char *path)
{
   if (!widget || !path ||
       !edje_file_group_exists(path, "eclair/editor")) return;
   const char *file = eina_stringshare_add(path);
   evas_object_data_set(widget, "eclair-theme-file", file);
   evas_object_event_callback_add(widget, EVAS_CALLBACK_DEL, _theme_deleted, (void *)file);
}

void eclair_ui_theme_apply(Evas_Object *widget, const char *path)
{
   if (!widget || !path ||
       !edje_file_group_exists(path, "eclair/editor")) return;
   Elm_Theme *theme = elm_theme_new();
   if (!theme) return;
   elm_theme_ref_set(theme, elm_theme_default_get());
   char *absolute = eclair_realpath(path);
   elm_theme_overlay_add(theme, absolute ? absolute : path);
   free(absolute);
   elm_object_theme_set(widget, theme);
   eclair_ui_theme_source_set(widget, path);
   /* Widgets retain their own reference, including their child widgets. */
   elm_theme_free(theme);
}
