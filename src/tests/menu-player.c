#include "eclair.h"
#include "eclair_window.h"
#include "eclair_scale.h"
#include <assert.h>

static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static Eina_Bool render(void *data) { elm_win_render(data); return ECORE_CALLBACK_RENEW; }
static void settle(Evas_Object *win)
{
   Ecore_Timer *frames = ecore_timer_add(1.0 / 60, render, win);
   ecore_timer_add(0.08, finish, NULL); ecore_main_loop_begin();
   ecore_timer_del(frames);
   elm_win_render(win);
}
static void key(Evas_Object *win, const char *name)
{
   Evas *evas = evas_object_evas_get(win);
   evas_focus_in(evas);
   evas_event_feed_key_down(evas, name, name, NULL, NULL, 0, NULL);
   evas_event_feed_key_up(evas, name, name, NULL, NULL, 0, NULL);
   settle(win);
}
static void point(Evas_Object *win, Evas_Object *obj, Eina_Bool click)
{
   int x, y, w, h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   assert(w > 0 && h > 0);
   Evas *evas = evas_object_evas_get(win);
   evas_event_feed_mouse_in(evas, 0, NULL);
   evas_event_feed_mouse_move(evas, x + w / 2, y + h / 2, 0, NULL);
   if (click) {
      evas_event_feed_mouse_down(evas, 1, EVAS_BUTTON_NONE, 0, NULL);
      evas_event_feed_mouse_up(evas, 1, EVAS_BUTTON_NONE, 0, NULL);
   }
   settle(win);
}
static void bounds(Eclair_Menu *root, Eclair_Menu *column)
{
   int x, y, w, h, ww, wh;
   evas_object_geometry_get(column->column, &x, &y, &w, &h);
   evas_object_geometry_get(root->parent_window, NULL, NULL, &ww, &wh);
   assert(x >= 0 && y >= 0 && x + w <= ww && y + h <= wh);
   assert(x >= root->bounds_x && y >= root->bounds_y);
   assert(x + w <= root->bounds_x + root->bounds_w);
   assert(y + h <= root->bounds_y + root->bounds_h);
}
static unsigned canvases(void)
{
   Eina_List *list = ecore_evas_ecore_evas_list_get();
   unsigned count = eina_list_count(list);
   eina_list_free(list);
   return count;
}
static void chosen(Eclair_Menu_Item *item, void *data) { (*(int *)data)++; }
static void preview(Evas_Object *win, const char *variable)
{
   const char *path = getenv(variable);
   if (!path) return;
   /* Capture final panel geometry, rather than a partially opened drawer. */
   Ecore_Timer *frames = ecore_timer_add(1.0 / 60, render, win);
   ecore_timer_add(1.2, finish, NULL);
   ecore_main_loop_begin();
   ecore_timer_del(frames);
   Evas *evas = evas_object_evas_get(win);
   Ecore_Evas *canvas = ecore_evas_ecore_evas_get(evas);
   int w, h;
   ecore_evas_geometry_get(canvas, NULL, NULL, &w, &h);
   elm_win_render(win);
   Evas_Object *image = evas_object_image_add(evas);
   evas_object_image_alpha_set(image, EINA_TRUE);
   evas_object_image_size_set(image, w, h);
   evas_object_image_data_copy_set(image, (void *)ecore_evas_buffer_pixels_get(canvas));
   assert(evas_object_image_save(image, path, NULL, NULL));
   evas_object_del(image);
}
int main(int argc, char **argv)
{
   elm_init(argc, argv); eclair_scale_init();
   for (int i = 1; i < argc; i++) {
      Eclair app = {0};
      app.gui_theme_file = argv[i];
      app.config.properties = eina_hash_string_superfast_new(free);
      app.gui_window = eclair_window_create(argv[i], "main", "Menu integration", ECLAIR_SOFTWARE, &app, EINA_TRUE);
      assert(app.gui_window);
      eclair_window_add_default_callbacks(app.gui_window, &app);
      eclair_dialogs_init(&app.dialogs_manager, &app);
      Evas_Object *win = app.gui_window->window, *layout = app.gui_window->edje_object;
      settle(win);
      Evas_Object *burger = (Evas_Object *)edje_object_part_object_get(layout, "button_menu");
      assert(burger && evas_object_visible_get(burger));
      unsigned windows = canvases();
      Eclair_Menu *menu = app.dialogs_manager.menu;
      assert(eina_list_count(menu->items) == 3);
      point(win, burger, EINA_FALSE);
      evas_event_feed_mouse_down(app.gui_window->evas, 3, EVAS_BUTTON_NONE, 0, NULL);
      evas_event_feed_mouse_up(app.gui_window->evas, 3, EVAS_BUTTON_NONE, 0, NULL);
      settle(win);
      assert(!menu->widget || !evas_object_visible_get(menu->widget));
      point(win, burger, EINA_TRUE);
      assert(menu->themed && evas_object_visible_get(menu->widget));
      assert(!strcmp(edje_object_part_state_get(layout, "button_menu", NULL), "open"));
      assert(canvases() == windows);
      assert(menu->scale >= eclair_scale_get() * 0.75);
      bounds(menu, menu);
      int burger_y, burger_h, column_y;
      evas_object_geometry_get(burger, NULL, &burger_y, NULL, &burger_h);
      evas_object_geometry_get(menu->column, NULL, &column_y, NULL, NULL);
      assert(column_y >= burger_y + burger_h);
      Eina_List *l;
      Eclair_Menu_Item *item;
      EINA_LIST_FOREACH(menu->items, l, item) {
         if (!item->child) continue;
         point(win, item->view, EINA_FALSE);
         assert(menu->selected == item);
         assert(item->child && evas_object_visible_get(item->child->column));
         bounds(menu, item->child);
         int x, w, child_x;
         evas_object_geometry_get(menu->column, &x, NULL, &w, NULL);
         evas_object_geometry_get(item->child->column, &child_x, NULL, NULL, NULL);
         assert(child_x >= x + w);
         if (item == app.dialogs_manager.menu_playlist_item) preview(win, "ECLAIR_MENU_PREVIEW");
         Eina_List *row_node;
         Eclair_Menu_Item *row;
         EINA_LIST_FOREACH(item->child->items, row_node, row) {
            if (row->type == ECLAIR_MENU_ITEM_SEPARATOR_ITEM) continue;
            Evas_Object *text = (Evas_Object *)edje_object_part_object_get(elm_layout_edje_get(row->view), "elm.text");
            int tw;
            edje_object_part_geometry_get(elm_layout_edje_get(row->view), "elm.text", NULL, NULL, &tw, NULL);
            assert(evas_object_text_horiz_advance_get(text) <= tw + 1);
         }
      }
      /* Last theme choice is below the short players' viewport. Keyboard
       * navigation scrolls it into view, and activating it closes the menu. */
      key(win, "Right");
      Eclair_Menu *choices = menu->selected->child;
      unsigned count = eina_list_count(choices->items);
      for (unsigned j = 1; j < count; j++) key(win, "Down");
      Eclair_Menu_Item *last = eina_list_last(choices->items)->data;
      assert(choices->selected == last);
      int calls = 0;
      eclair_menu_item_callback_set(last, chosen, &calls);
      key(win, "Return");
      assert(calls == 1 && !evas_object_visible_get(menu->widget));
      assert(evas_focus_get(app.gui_window->evas) == layout);
      point(win, burger, EINA_TRUE);
      key(win, "Escape");
      assert(!evas_object_visible_get(menu->widget));
      assert(!strcmp(edje_object_part_state_get(layout, "button_menu", NULL), "default"));
      /* About is a direct action, uses the current skin, and is single-instance. */
      point(win, burger, EINA_TRUE);
      point(win, app.dialogs_manager.menu_about_item->view, EINA_TRUE);
      Evas_Object *about = app.dialogs_manager.about_window;
      assert(about && !evas_object_visible_get(menu->widget));
      settle(about);
      Evas_Object *frame = elm_object_name_find(about, "about-frame", -1);
      assert(frame);
      const char *about_group;
      edje_object_file_get(elm_layout_edje_get(frame), NULL, &about_group);
      assert(!strcmp(about_group, "eclair/about"));
      Evas_Object *credits = elm_object_name_find(about, "about-credits", -1);
      const char *text = elm_object_text_get(credits);
      assert(strstr(text, "Simon Treny") && strstr(text, "Alastair Poole") && strstr(text, "vko"));
      assert(strstr(text, "2005") && strstr(text, "COPYING"));
      eclair_dialogs_about_window(&app.dialogs_manager);
      assert(app.dialogs_manager.about_window == about);
      preview(about, "ECLAIR_ABOUT_PREVIEW");
      evas_object_resize(about, eclair_scale_pixels(400), eclair_scale_pixels(420));
      settle(about);
      Evas_Object *close = elm_object_name_find(about, "about-close", -1);
      assert(close);
      int close_y, close_h, about_h;
      evas_object_geometry_get(close, NULL, &close_y, NULL, &close_h);
      evas_object_geometry_get(about, NULL, NULL, NULL, &about_h);
      assert(close_h > 0 && close_y >= 0 && close_y + close_h <= about_h);
      evas_object_smart_callback_call(close, "clicked", NULL);
      assert(!app.dialogs_manager.about_window);
      eclair_dialogs_shutdown(&app.dialogs_manager);
      eclair_window_del(app.gui_window);
      eina_hash_free(app.config.properties);
   }
   elm_shutdown();
   return 0;
}
