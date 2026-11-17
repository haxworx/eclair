#ifndef _ECLAIR_MENU_H_
#define _ECLAIR_MENU_H_

#include <Elementary.h>
#include "eclair_types.h"

struct _Eclair_Menu
{
   Eclair_Menu_Item *parent;
   Eina_List *items;
   Evas_Object *parent_window;
   Evas_Object *widget;
   Evas_Object *keyboard;
   Evas_Object *column;
   Evas_Object *scroller;
   Eclair_Menu_Item *selected;
   Eina_Bool themed;
   double scale;
   int width, height;
   int bounds_x, bounds_y, bounds_w, bounds_h;
};

enum _Eclair_Menu_Item_Type
{
   ECLAIR_MENU_ITEM_NORMAL_ITEM,
   ECLAIR_MENU_ITEM_ICON_ITEM,
   ECLAIR_MENU_ITEM_CHECK_ITEM,
   ECLAIR_MENU_ITEM_SEPARATOR_ITEM
};

struct _Eclair_Menu_Item
{
   Eclair_Menu *parent;
   Eclair_Menu *child;
   char *label;
   char *icon_path;
   Elm_Object_Item *widget_item; /* Desktop fallback for custom themes. */
   Evas_Object *view;
   Eclair_Menu_Item_Type type;
   void (*item_cb)(Eclair_Menu_Item *item, void *data);
   void *cb_data;
};

Eclair_Menu *eclair_menu_new(Evas_Object *parent_window);
void eclair_menu_free(Eclair_Menu *menu);
int eclair_menu_append_item(Eclair_Menu *menu, Eclair_Menu_Item *item);
void eclair_menu_attach_to_item(Eclair_Menu *menu, Eclair_Menu_Item *item);
void eclair_menu_popup(Eclair_Menu *menu);
void eclair_menu_popup_at_xy(Eclair_Menu *menu, int x, int y);
void eclair_menu_pop_down(Eclair_Menu *menu);
Eclair_Menu_Item *eclair_menu_add_item(Eclair_Menu *menu, const char *label);
Eclair_Menu_Item *eclair_menu_add_item_with_icon(Eclair_Menu *menu, const char *label, const char *icon_path);
Eclair_Menu_Item *eclair_menu_add_seperator(Eclair_Menu *menu);
void eclair_menu_item_label_set(Eclair_Menu_Item *item, const char *label);
void eclair_menu_item_callback_set(Eclair_Menu_Item *item, void (*item_cb)(Eclair_Menu_Item *item, void *data), void *data);

#endif
