#include "eclair.h"
#include "eclair_window.h"
#include "eclair_scale.h"
#include "eclair_ui.h"
#include <assert.h>

static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static void settle(Evas_Object *win)
{
   elm_win_render(win);
   ecore_timer_add(0.06, finish, NULL); ecore_main_loop_begin();
   elm_win_render(win);
}
static void press(Evas_Object *win, const char *key)
{
   Evas *evas = evas_object_evas_get(win);
   evas_focus_in(evas);
   evas_event_feed_key_down(evas, key, key, NULL, NULL, 0, NULL);
   evas_event_feed_key_up(evas, key, key, NULL, NULL, 0, NULL);
   settle(win);
}
static void clicked(Eclair_Menu_Item *item, void *data) { (*(int *)data)++; }
static void check_column(Eclair_Menu *root, Eclair_Menu *column)
{
   int x, y, w, h;
   evas_object_geometry_get(column->column, &x, &y, &w, &h);
   assert(w > 0 && h > 0);
   assert(x >= root->bounds_x && y >= root->bounds_y);
   assert(x + w <= root->bounds_x + root->bounds_w);
   assert(y + h <= root->bounds_y + root->bounds_h);
}
int main(int argc, char **argv)
{
   elm_init(argc, argv);
   eclair_scale_init();
   Evas_Object *win = elm_win_util_standard_add("menu-test", "Menu test");
   if (argc > 1) eclair_ui_theme_source_set(win, argv[1]);
   evas_object_resize(win, eclair_scale_pixels(306), eclair_scale_pixels(120));
   evas_object_show(win);
   settle(win);
   Eclair_Menu *menu = eclair_menu_new(win), *child = eclair_menu_new(win);
   Eclair_Menu_Item *open = eclair_menu_add_item_with_icon(menu, "Windows", "window-new");
   Eclair_Menu_Item *playlist = eclair_menu_add_item_with_icon(child, "Playlist Manager", "view-list-details");
   int calls = 0;
   eclair_menu_item_callback_set(playlist, clicked, &calls);
   if (argc > 1)
      for (int i = 0; i < 12; i++) eclair_menu_add_item(child, "Another playlist action");
   eclair_menu_attach_to_item(child, open);
   eclair_menu_popup_at_xy(menu, 10000, 10000);
   settle(win);
   assert(menu->widget);
   if (argc > 1) {
      assert(menu->themed && open->view);
      assert(evas_object_evas_get(menu->widget) == evas_object_evas_get(win));
      assert(menu->scale >= eclair_scale_get() * 0.7);
      check_column(menu, menu);
      Evas *evas = evas_object_evas_get(win);
      int x, y, w, h;
      evas_object_geometry_get(open->view, &x, &y, &w, &h);
      evas_event_feed_mouse_in(evas, 0, NULL);
      evas_event_feed_mouse_move(evas, x + w / 2, y + h / 2, 0, NULL);
      settle(win);
      assert(menu->selected == open && child->column);
      check_column(menu, child);
      int parent_x, parent_w, child_x;
      evas_object_geometry_get(menu->column, &parent_x, NULL, &parent_w, NULL);
      evas_object_geometry_get(child->column, &child_x, NULL, NULL, NULL);
      assert(child_x >= parent_x + parent_w);
      assert(!strcmp(elm_object_part_text_get(playlist->view, "elm.text"), "Playlist Manager"));
      Evas_Object *icon = elm_object_part_content_get(open->view, "eclair.icon");
      const char *file, *group;
      assert(icon);
      edje_object_file_get(elm_layout_edje_get(icon), &file, &group);
      assert(file && group && !strcmp(group, "eclair/icon/window-new"));
      const char *preview = getenv("ECLAIR_UI_PREVIEW");
      if (preview) {
         Ecore_Evas *canvas = ecore_evas_ecore_evas_get(evas);
         ecore_evas_geometry_get(canvas, NULL, NULL, &w, &h);
         ecore_evas_manual_render(canvas);
         Evas_Object *image = evas_object_image_add(evas);
         evas_object_image_alpha_set(image, EINA_TRUE);
         evas_object_image_size_set(image, w, h);
         evas_object_image_data_copy_set(image, (void *)ecore_evas_buffer_pixels_get(canvas));
         assert(evas_object_image_save(image, preview, NULL, NULL));
         evas_object_del(image);
      }
      press(win, "Right");
      assert(child->selected == playlist);
      for (int i = 0; i < 12; i++) press(win, "Down");
      int scroll_y = 0;
      elm_scroller_region_get(child->scroller, NULL, &scroll_y, NULL, NULL);
      assert(scroll_y > 0);
      press(win, "Left");
      assert(!evas_object_visible_get(child->column));
      press(win, "Right");
      press(win, "Return");
      assert(calls == 1 && !evas_object_visible_get(menu->widget));
      assert(evas_object_visible_get(win));
      eclair_menu_popup_at_xy(menu, -100, -100);
      settle(win);
      check_column(menu, menu);
      press(win, "Escape");
      assert(!evas_object_visible_get(menu->widget));
      eclair_menu_popup_at_xy(menu, 0, 0);
      settle(win);
      /* A resize closes the overlay rather than leaving stale columns. */
      evas_object_resize(win, eclair_scale_pixels(320), eclair_scale_pixels(140));
      settle(win);
      assert(!evas_object_visible_get(menu->widget));
   } else {
      assert(open->widget_item && playlist->widget_item);
      assert(!strcmp(elm_object_item_text_get(open->widget_item), "Windows"));
      assert(elm_menu_item_subitems_get(open->widget_item));
   }
   eclair_menu_item_label_set(playlist, "A considerably longer playlist menu label");
   eclair_menu_pop_down(menu);
   eclair_menu_free(menu);
   evas_object_del(win);
   elm_shutdown();
   return 0;
}
