#include "eclair_menu.h"
#include "eclair_scale.h"
#include "eclair_ui.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void _eclair_menu_resized(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _eclair_menu_column_show(Eclair_Menu *menu, int x, int y);

Eclair_Menu *eclair_menu_new(Evas_Object *parent_window)
{
   if (!parent_window) return NULL;
   Eclair_Menu *menu = calloc(1, sizeof(*menu));
   if (menu) {
      menu->parent_window = parent_window;
      evas_object_event_callback_add(parent_window, EVAS_CALLBACK_RESIZE, _eclair_menu_resized, menu);
   }
   return menu;
}

static Eclair_Menu *_eclair_menu_root(Eclair_Menu *menu)
{
   while (menu && menu->parent) menu = menu->parent->parent;
   return menu;
}

static void _eclair_menu_hide_columns(Eclair_Menu *menu)
{
   Eina_List *l;
   Eclair_Menu_Item *item;
   if (menu->column) evas_object_hide(menu->column);
   EINA_LIST_FOREACH(menu->items, l, item) {
      if (item->view) elm_layout_signal_emit(item->view, "eclair,selected,off", "eclair");
      if (item->child) _eclair_menu_hide_columns(item->child);
   }
   menu->selected = NULL;
}

void eclair_menu_pop_down(Eclair_Menu *menu)
{
   menu = _eclair_menu_root(menu);
   if (!menu) return;
   Eina_Bool was_open = menu->widget && evas_object_visible_get(menu->widget);
   if (menu->keyboard) { evas_object_del(menu->keyboard); menu->keyboard = NULL; }
   if (menu->themed) {
      _eclair_menu_hide_columns(menu);
      if (menu->widget) evas_object_hide(menu->widget);
   } else if (menu->widget) elm_menu_close(menu->widget);
   Evas_Object *player = evas_object_data_get(menu->parent_window, "eclair-player-layout");
   if (player && was_open) {
      edje_object_signal_emit(player, "signal_menu_closed", "eclair");
      evas_object_focus_set(player, EINA_TRUE);
   }
}

static void _eclair_menu_resized(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Eclair_Menu *menu = data;
   if (!menu->parent) eclair_menu_pop_down(menu);
}

static void _eclair_menu_views_clear(Eclair_Menu *menu)
{
   Eina_List *l;
   Eclair_Menu_Item *item;
   EINA_LIST_FOREACH(menu->items, l, item) {
      if (item->child) _eclair_menu_views_clear(item->child);
      item->view = NULL;
      item->widget_item = NULL;
   }
   if (menu->column) evas_object_del(menu->column);
   menu->column = menu->scroller = NULL;
   menu->selected = NULL;
}

/* Measure compact rows at desktop scale. Only horizontal space can reduce the
 * font size; vertical overflow scrolls, like the player's playlist. */
static int _eclair_menu_measure(Eclair_Menu *menu, const char *theme, double scale)
{
   Eclair_Menu *root = _eclair_menu_root(menu);
   Evas_Object *row = edje_object_add(evas_object_evas_get(root->parent_window));
   edje_object_file_set(row, theme, "eclair/menu-row");
   edje_object_scale_set(row, scale);
   int children = 0;
   menu->width = (int)ceil(72 * scale);
   menu->height = 0;
   Eina_List *l;
   Eclair_Menu_Item *item;
   EINA_LIST_FOREACH(menu->items, l, item) {
      if (item->type == ECLAIR_MENU_ITEM_SEPARATOR_ITEM) {
         menu->height += (int)ceil(4 * scale);
         continue;
      }
      edje_object_part_text_set(row, "elm.text", item->label);
      edje_object_calc_force(row);
      int w, h;
      edje_object_size_min_calc(row, &w, &h);
      if (w > menu->width) menu->width = w;
      menu->height += (int)ceil(20 * scale);
      if (item->child) {
         int path = _eclair_menu_measure(item->child, theme, scale);
         if (path > children) children = path;
      }
   }
   evas_object_del(row);
   /* The panel border and the narrow scrollbar have their own space. */
   menu->width += (int)ceil(14 * scale);
   menu->height += (int)ceil(4 * scale);
   return menu->width + (children ? children + (int)ceil(3 * scale) : 0);
}

static void _eclair_menu_choose(Eclair_Menu_Item *item)
{
   Eclair_Menu *menu = item->parent;
   Eclair_Menu *root = _eclair_menu_root(menu);
   if (!root->widget || !evas_object_visible_get(root->widget)) return;
   if (item == menu->selected) return;
   if (menu->selected) {
      elm_layout_signal_emit(menu->selected->view, "eclair,selected,off", "eclair");
      if (menu->selected->child) _eclair_menu_hide_columns(menu->selected->child);
   }
   menu->selected = item;
   elm_layout_signal_emit(item->view, "eclair,selected,on", "eclair");
   if (item->child) {
      int x, y, w, row_y;
      evas_object_geometry_get(menu->column, &x, &y, &w, NULL);
      evas_object_geometry_get(item->view, NULL, &row_y, NULL, NULL);
      _eclair_menu_column_show(item->child, x + w + (int)ceil(3 * root->scale), row_y);
   }
}

static void _eclair_menu_clicked(void *data, Evas_Object *obj, void *event)
{
   Eclair_Menu_Item *item = data;
   if (item->child) {
      if (_eclair_menu_root(item->parent)->themed) _eclair_menu_choose(item);
      return;
   }
   eclair_menu_pop_down(item->parent);
   if (item->item_cb) item->item_cb(item, item->cb_data);
}

static void _eclair_menu_hover(void *data, Evas_Object *obj, const char *signal, const char *source)
{
   _eclair_menu_choose(data);
}

static void _eclair_menu_activate(void *data, Evas_Object *obj, const char *signal, const char *source)
{
   _eclair_menu_clicked(data, obj, NULL);
}

static void _eclair_menu_dismiss(void *data, Evas_Object *obj, const char *signal, const char *source)
{
   eclair_menu_pop_down(data);
}

static void _eclair_menu_column_show(Eclair_Menu *menu, int x, int y)
{
   Eclair_Menu *root = _eclair_menu_root(menu);
   int h = menu->height < root->bounds_h ? menu->height : root->bounds_h;
   if (y + h > root->bounds_y + root->bounds_h) y = root->bounds_y + root->bounds_h - h;
   if (y < root->bounds_y) y = root->bounds_y;
   if (!menu->column) {
      const char *theme = evas_object_data_get(root->parent_window, "eclair-theme-file");
      menu->column = elm_layout_add(root->widget);
      elm_layout_file_set(menu->column, theme, "eclair/menu-panel");
      menu->scroller = elm_scroller_add(menu->column);
      elm_object_style_set(menu->scroller, "eclair-menu");
      elm_scroller_policy_set(menu->scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
      elm_scroller_content_min_limit(menu->scroller, EINA_FALSE, EINA_FALSE);
      elm_scroller_bounce_set(menu->scroller, EINA_FALSE, EINA_FALSE);
      elm_object_part_content_set(menu->column, "eclair.content", menu->scroller);
      Evas_Object *box = elm_box_add(menu->scroller);
      evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0);
      evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
      elm_object_content_set(menu->scroller, box);
      Eina_List *l;
      Eclair_Menu_Item *item;
      EINA_LIST_FOREACH(menu->items, l, item) {
         item->view = elm_layout_add(box);
         Eina_Bool separator = item->type == ECLAIR_MENU_ITEM_SEPARATOR_ITEM;
         elm_layout_file_set(item->view, theme, separator ? "eclair/menu-separator" : "eclair/menu-row");
         evas_object_size_hint_weight_set(item->view, EVAS_HINT_EXPAND, 0);
         evas_object_size_hint_align_set(item->view, EVAS_HINT_FILL, EVAS_HINT_FILL);
         evas_object_size_hint_min_set(item->view, 0, (int)ceil((separator ? 4 : 20) * root->scale));
         if (!separator) {
            elm_object_part_text_set(item->view, "elm.text", item->label);
            elm_object_access_info_set(item->view, item->label);
            if (item->child) elm_layout_signal_emit(item->view, "eclair,branch,on", "eclair");
            if (item->icon_path) {
               char group[256];
               snprintf(group, sizeof(group), "eclair/icon/%s", item->icon_path);
               if (edje_file_group_exists(theme, group)) {
                  Evas_Object *icon = elm_layout_add(item->view);
                  elm_layout_file_set(icon, theme, group);
                  elm_object_part_content_set(item->view, "eclair.icon", icon);
               }
            }
            elm_layout_signal_callback_add(item->view, "eclair,menu,hover", "eclair", _eclair_menu_hover, item);
            elm_layout_signal_callback_add(item->view, "eclair,menu,activate", "eclair", _eclair_menu_activate, item);
         }
         elm_box_pack_end(box, item->view);
         evas_object_show(item->view);
      }
      evas_object_show(box);
      evas_object_show(menu->scroller);
   }
   evas_object_move(menu->column, x, y);
   evas_object_resize(menu->column, menu->width, h);
   evas_object_raise(menu->column);
   evas_object_show(menu->column);
}

static Eclair_Menu_Item *_eclair_menu_selected(Eclair_Menu *menu)
{
   if (menu->themed || _eclair_menu_root(menu)->themed) {
      if (!menu->selected) return NULL;
      Eclair_Menu *child = menu->selected->child;
      Eclair_Menu_Item *item = child && child->column && evas_object_visible_get(child->column) ?
         _eclair_menu_selected(child) : NULL;
      return item ? item : menu->selected;
   }
   Eina_List *l;
   Eclair_Menu_Item *item;
   EINA_LIST_FOREACH(menu->items, l, item) {
      if (item->widget_item && elm_menu_item_selected_get(item->widget_item)) {
         Eclair_Menu_Item *child = item->child ? _eclair_menu_selected(item->child) : NULL;
         return child ? child : item;
      }
   }
   return NULL;
}

static void _eclair_menu_select(Eclair_Menu_Item *item)
{
   if (!_eclair_menu_root(item->parent)->themed) {
      elm_menu_item_selected_set(item->widget_item, EINA_TRUE);
      return;
   }
   _eclair_menu_choose(item);
   int x, y, w, h, bx, by;
   evas_object_geometry_get(item->view, &x, &y, &w, &h);
   evas_object_geometry_get(elm_object_content_get(item->parent->scroller), &bx, &by, NULL, NULL);
   elm_scroller_region_show(item->parent->scroller, x - bx, y - by, w, h);
}

static void _eclair_menu_key(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Eclair_Menu *root = data;
   if (!evas_object_visible_get(root->widget)) return;
   Evas_Event_Key_Down *key = event;
   Eclair_Menu_Item *selected = _eclair_menu_selected(root);
   Eclair_Menu *menu = selected ? selected->parent : root;
   if (!strcmp(key->keyname, "Escape")) eclair_menu_pop_down(root);
   else if (!strcmp(key->keyname, "Down") || !strcmp(key->keyname, "Up")) {
      Eina_Bool reverse = !strcmp(key->keyname, "Up");
      Eina_List *node = selected ? eina_list_data_find_list(menu->items, selected) : NULL;
      unsigned count = eina_list_count(menu->items);
      while (count--) {
         node = node ? (reverse ? node->prev : node->next) : NULL;
         if (!node) node = reverse ? eina_list_last(menu->items) : menu->items;
         Eclair_Menu_Item *item = node->data;
         if (item->type == ECLAIR_MENU_ITEM_SEPARATOR_ITEM) continue;
         _eclair_menu_select(item);
         break;
      }
   } else if (selected && (!strcmp(key->keyname, "Right") ||
                          !strcmp(key->keyname, "Return") || !strcmp(key->keyname, "space"))) {
      if (selected->child) {
         if (root->themed) {
            int x, y, w;
            evas_object_geometry_get(menu->column, &x, &y, &w, NULL);
            _eclair_menu_column_show(selected->child, x + w + (int)ceil(3 * root->scale), y);
         } else elm_object_item_signal_emit(selected->widget_item, "elm,action,open", "elm");
         Eina_List *l;
         Eclair_Menu_Item *item;
         EINA_LIST_FOREACH(selected->child->items, l, item) {
            if (item->type == ECLAIR_MENU_ITEM_SEPARATOR_ITEM) continue;
            _eclair_menu_select(item);
            break;
         }
      } else if (strcmp(key->keyname, "Right")) _eclair_menu_clicked(selected, obj, NULL);
   } else if (selected && !strcmp(key->keyname, "Left") && menu->parent) {
      if (root->themed) _eclair_menu_hide_columns(menu);
      else {
         elm_menu_item_selected_set(selected->widget_item, EINA_FALSE);
         elm_menu_item_selected_set(menu->parent->widget_item, EINA_FALSE);
         elm_menu_item_selected_set(menu->parent->widget_item, EINA_TRUE);
      }
   } else return;
   key->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
}

static void _eclair_menu_build_native(Eclair_Menu *menu, Evas_Object *widget, Elm_Object_Item *parent)
{
   Eina_List *l;
   Eclair_Menu_Item *item;
   EINA_LIST_FOREACH(menu->items, l, item) {
      if (item->type == ECLAIR_MENU_ITEM_SEPARATOR_ITEM)
         item->widget_item = elm_menu_item_separator_add(widget, parent);
      else item->widget_item = elm_menu_item_add(widget, parent, item->icon_path,
                                                item->label, _eclair_menu_clicked, item);
      if (item->child) _eclair_menu_build_native(item->child, widget, item->widget_item);
   }
}

void eclair_menu_popup_at_xy(Eclair_Menu *menu, int x, int y)
{
   menu = _eclair_menu_root(menu);
   if (!menu) return;
   eclair_menu_pop_down(menu);
   _eclair_menu_views_clear(menu);
   if (menu->widget) evas_object_del(menu->widget);
   menu->widget = NULL;
   const char *theme = evas_object_data_get(menu->parent_window, "eclair-theme-file");
   menu->themed = theme && edje_file_group_exists(theme, "eclair/menu-overlay");
   if (menu->themed) {
      int ww, wh;
      evas_object_geometry_get(menu->parent_window, NULL, NULL, &ww, &wh);
      menu->bounds_x = menu->bounds_y = 0;
      menu->bounds_w = ww; menu->bounds_h = wh;
      Evas_Object *player = evas_object_data_get(menu->parent_window, "eclair-player-layout");
      if (player) {
         int px, py;
         evas_object_geometry_get(player, &px, &py, NULL, NULL);
         edje_object_part_geometry_get(player, "body", &menu->bounds_x, &menu->bounds_y,
                                        &menu->bounds_w, &menu->bounds_h);
         menu->bounds_x += px; menu->bounds_y += py;
         /* Leave the title-bar controls visible, including the menu toggle. */
         if (edje_object_part_exists(player, "button_menu")) {
            int button_y, button_h;
            edje_object_part_geometry_get(player, "button_menu", NULL, &button_y, NULL, &button_h);
            int header = py + button_y + button_h + eclair_scale_pixels(2) - menu->bounds_y;
            if (header > 0 && header < menu->bounds_h) {
               menu->bounds_y += header;
               menu->bounds_h -= header;
            }
         }
      }
      int pad = eclair_scale_pixels(3);
      menu->bounds_x += pad; menu->bounds_y += pad;
      menu->bounds_w -= pad * 2; menu->bounds_h -= pad * 2;
      if (menu->bounds_w < 1 || menu->bounds_h < 1) return;
      menu->scale = eclair_scale_get();
      int path = _eclair_menu_measure(menu, theme, menu->scale);
      if (path > menu->bounds_w) {
         menu->scale *= (double)menu->bounds_w / (path + eclair_scale_pixels(2));
         path = _eclair_menu_measure(menu, theme, menu->scale);
      }
      if (x + path > menu->bounds_x + menu->bounds_w) x = menu->bounds_x + menu->bounds_w - path;
      if (x < menu->bounds_x) x = menu->bounds_x;
      menu->widget = elm_layout_add(menu->parent_window);
      eclair_ui_theme_apply(menu->widget, theme);
      double profile = elm_config_scale_get();
      elm_object_scale_set(menu->widget, menu->scale / (profile > 0 ? profile : 1));
      elm_layout_file_set(menu->widget, theme, "eclair/menu-overlay");
      evas_object_move(menu->widget, 0, 0);
      evas_object_resize(menu->widget, ww, wh);
      elm_layout_signal_callback_add(menu->widget, "eclair,menu,dismiss", "eclair", _eclair_menu_dismiss, menu);
      evas_object_show(menu->widget);
      _eclair_menu_column_show(menu, x, y);
      if (player) edje_object_signal_emit(player, "signal_menu_open", "eclair");
   } else {
      menu->widget = elm_menu_add(menu->parent_window);
      elm_menu_parent_set(menu->widget, menu->parent_window);
      _eclair_menu_build_native(menu, menu->widget, NULL);
      elm_menu_move(menu->widget, x, y);
      evas_object_show(menu->widget);
   }
   menu->keyboard = evas_object_rectangle_add(evas_object_evas_get(menu->widget));
   evas_object_color_set(menu->keyboard, 0, 0, 0, 0);
   evas_object_pass_events_set(menu->keyboard, EINA_TRUE);
   evas_object_show(menu->keyboard);
   evas_object_focus_set(menu->keyboard, EINA_TRUE);
   evas_object_event_callback_add(menu->keyboard, EVAS_CALLBACK_KEY_DOWN, _eclair_menu_key, menu);
   const char *keys[] = {"Escape", "Up", "Down", "Left", "Right", "Return", "space", NULL};
   for (int i = 0; keys[i]; i++)
      if (!evas_object_key_grab(menu->keyboard, keys[i], 0, 0, EINA_TRUE))
         EINA_LOG_WARN("Unable to grab menu key %s", keys[i]);
}

void eclair_menu_popup(Eclair_Menu *menu)
{
   if (!menu) return;
   int x, y;
   evas_pointer_canvas_xy_get(evas_object_evas_get(menu->parent_window), &x, &y);
   eclair_menu_popup_at_xy(menu, x, y);
}

int eclair_menu_append_item(Eclair_Menu *menu, Eclair_Menu_Item *item)
{
   if (!menu || !item) return 0;
   item->parent = menu;
   menu->items = eina_list_append(menu->items, item);
   return 1;
}

void eclair_menu_attach_to_item(Eclair_Menu *menu, Eclair_Menu_Item *item)
{
   if (!menu || !item) return;
   menu->parent = item;
   item->child = menu;
}

static Eclair_Menu_Item *_eclair_menu_item_new(Eclair_Menu *menu, const char *label,
                                              const char *icon_path, Eclair_Menu_Item_Type type)
{
   if (!menu) return NULL;
   Eclair_Menu_Item *item = calloc(1, sizeof(*item));
   if (!item) return NULL;
   item->label = label ? strdup(label) : NULL;
   item->icon_path = icon_path ? strdup(icon_path) : NULL;
   item->type = type;
   eclair_menu_append_item(menu, item);
   return item;
}

Eclair_Menu_Item *eclair_menu_add_item(Eclair_Menu *menu, const char *label)
{
   return _eclair_menu_item_new(menu, label, NULL, ECLAIR_MENU_ITEM_NORMAL_ITEM);
}
Eclair_Menu_Item *eclair_menu_add_item_with_icon(Eclair_Menu *menu, const char *label, const char *path)
{
   return _eclair_menu_item_new(menu, label, path, ECLAIR_MENU_ITEM_ICON_ITEM);
}
Eclair_Menu_Item *eclair_menu_add_seperator(Eclair_Menu *menu)
{
   return _eclair_menu_item_new(menu, NULL, NULL, ECLAIR_MENU_ITEM_SEPARATOR_ITEM);
}
void eclair_menu_item_label_set(Eclair_Menu_Item *item, const char *label)
{
   if (!item) return;
   free(item->label);
   item->label = label ? strdup(label) : NULL;
   if (item->widget_item) elm_object_item_text_set(item->widget_item, label);
   if (item->view) elm_object_part_text_set(item->view, "elm.text", label);
}
void eclair_menu_item_callback_set(Eclair_Menu_Item *item, void (*callback)(Eclair_Menu_Item *, void *), void *data)
{
   if (!item) return;
   item->item_cb = callback;
   item->cb_data = data;
}
void eclair_menu_free(Eclair_Menu *menu)
{
   if (!menu) return;
   if (!menu->parent) {
      eclair_menu_pop_down(menu);
      _eclair_menu_views_clear(menu);
      if (menu->widget) evas_object_del(menu->widget);
   }
   evas_object_event_callback_del_full(menu->parent_window, EVAS_CALLBACK_RESIZE,
                                      _eclair_menu_resized, menu);
   Eclair_Menu_Item *item;
   EINA_LIST_FREE(menu->items, item) {
      eclair_menu_free(item->child);
      free(item->label);
      free(item->icon_path);
      free(item);
   }
   free(menu);
}
