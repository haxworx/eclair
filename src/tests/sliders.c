#include "eclair.h"
#include "eclair_window.h"
#include "eclair_scale.h"
#include "eclair_playlist_container.h"
#include <assert.h>
#include <math.h>
static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static void settle(void) { ecore_timer_add(1.1, finish, NULL); ecore_main_loop_begin(); }
static void value_check(Evas_Object *edje, const char *thumb, Eina_Bool horizontal, double expected)
{
   double x, y;
   edje_object_part_drag_value_get(edje, thumb, &x, &y);
   double actual = horizontal ? x : y;
   if (fabs(actual - expected) >= .04)
      fprintf(stderr, "%s expected %.2f got %.2f\n", thumb, expected, actual);
   assert(fabs(actual - expected) < .04);
}
int main(int argc, char **argv)
{
   elm_init(argc, argv); eclair_scale_init();
   for (int theme=1; theme<argc; theme++) {
      Eclair app = {0};
      app.config.properties = eina_hash_string_superfast_new(free);
      app.meta_tag_manager.meta_tag_delete_thread = 1;
      app.gui_window = eclair_window_create(argv[theme], "main", "Slider input", ECLAIR_SOFTWARE, &app, 1);
      assert(app.gui_window);
      Eclair_Window *window = app.gui_window;
      app.playlist_container_owner = window;
      app.playlist_container = eclair_playlist_container_object_add(window->evas, &app);
      eclair_playlist_container_set_entry_theme_path(app.playlist_container, argv[theme]);
      edje_object_part_swallow(window->edje_object, "playlist_container", app.playlist_container);
      evas_object_show(app.playlist_container);
      edje_object_signal_emit(window->edje_object, "signal_play", "eclair_bin");
      settle(); elm_win_render(window->window); settle();
      const char *thumbs[] = {"progress_bar_drag", "volume_bar_drag", "playlist_scrollbar_drag"};
      const char *tracks[] = {"progress_bar_track", "volume_bar_track", "playlist_scrollbar_track"};
      const char *confines[] = {"progress_bar_confine", "volume_bar_confine", "playlist_scrollbar_confine"};
      for (int i=0;i<3;i++) {
         int ex,ey,x,y,w,h,tw,th,tx,ty,twidth,theight;
         Eina_Bool horizontal = i<2;
         evas_object_geometry_get(window->edje_object,&ex,&ey,NULL,NULL);
         edje_object_part_geometry_get(window->edje_object,confines[i],&x,&y,&w,&h);
         edje_object_part_geometry_get(window->edje_object,thumbs[i],NULL,NULL,&tw,&th);
         edje_object_part_geometry_get(window->edje_object,tracks[i],&tx,&ty,&twidth,&theight);
         int travel = horizontal ? w-tw : h-th;
         for (int step=0;step<2;step++) {
            double target = step ? .25 : .75;
            int px = horizontal ? ex+x+tw/2.0+target*travel : ex+tx+twidth/2;
            int py = horizontal ? ey+ty+theight/2 : ey+y+th/2.0+target*travel;
            evas_event_feed_mouse_in(window->evas,0,NULL);
            evas_event_feed_mouse_move(window->evas,px,py,0,NULL);
            evas_event_feed_mouse_down(window->evas,1,EVAS_BUTTON_NONE,0,NULL);
            evas_event_feed_mouse_up(window->evas,1,EVAS_BUTTON_NONE,0,NULL);
            edje_object_message_signal_process(window->edje_object);
            value_check(window->edje_object,thumbs[i],horizontal,target);
         }
         /* A track click can continue as a drag, including past either end. */
         int track_x = horizontal ? ex+x+tw/2.0+.75*travel : ex+tx+twidth/2;
         int track_y = horizontal ? ey+ty+theight/2 : ey+y+th/2.0+.75*travel;
         evas_event_feed_mouse_move(window->evas,track_x,track_y,0,NULL);
         evas_event_feed_mouse_down(window->evas,1,EVAS_BUTTON_NONE,0,NULL);
         for (int end=0;end<2;end++) {
            int px=horizontal ? ex+x+(end?2*travel:-travel) : track_x;
            int py=horizontal ? track_y : ey+y+(end?2*travel:-travel);
            evas_event_feed_mouse_move(window->evas,px,py,0,NULL);
            value_check(window->edje_object,thumbs[i],horizontal,end);
         }
         evas_event_feed_mouse_up(window->evas,1,EVAS_BUTTON_NONE,0,NULL);
         assert(!app.use_progress_bar_drag_for_time);
         edje_object_part_drag_value_set(window->edje_object,thumbs[i],horizontal?.25:0,horizontal?0:.25);
         /* The thumb still supports its original drag gesture. */
         edje_object_calc_force(window->edje_object);
         Evas_Object *thumb=(Evas_Object *)edje_object_part_object_get(window->edje_object,thumbs[i]);
         evas_object_geometry_get(thumb,&tx,&ty,&twidth,&theight);
         int px=tx+twidth/2, py=ty+theight/2;
         evas_event_feed_mouse_move(window->evas,px,py,0,NULL);
         evas_event_feed_mouse_down(window->evas,1,EVAS_BUTTON_NONE,0,NULL);
         edje_object_message_signal_process(window->edje_object);
         evas_event_feed_mouse_move(window->evas,px+(horizontal?travel/4:0),py+(horizontal?0:travel/4),0,NULL);
         evas_event_feed_mouse_up(window->evas,1,EVAS_BUTTON_NONE,0,NULL);
         edje_object_message_signal_process(window->edje_object);
         value_check(window->edje_object,thumbs[i],horizontal,.5);
      }
      eclair_window_del(window);
      eina_hash_free(app.config.properties);
   }
   elm_shutdown();
}
