#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <assert.h>
#include <string.h>

typedef struct {
   Evas_Object *layout;
   int queue[4], artwork[4];
} Player;

static Eina_Bool finish(void *data) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
static Eina_Bool render(void *data) { ecore_evas_manual_render(data); return ECORE_CALLBACK_RENEW; }
static void settle(Ecore_Evas *canvas, double seconds)
{
   Ecore_Timer *frames = ecore_timer_add(1.0 / 60, render, canvas);
   ecore_timer_add(seconds, finish, NULL);
   ecore_main_loop_begin();
   ecore_timer_del(frames);
}
static void send(Player *players, int count, const char *signal)
{
   for (int i = 0; i < count; i++) {
      edje_object_signal_emit(players[i].layout, signal, "eclair_bin");
      for (int j = 0; j < 5; j++) edje_object_message_signal_process(players[i].layout);
   }
}
static void geometry(Evas_Object *layout, const char *part, int *b)
{
   edje_object_part_geometry_get(layout, part, &b[0], &b[1], &b[2], &b[3]);
}
static int alpha(Evas_Object *layout, const char *part)
{
   int a;
   evas_object_color_get(edje_object_part_object_get(layout, part), NULL, NULL, NULL, &a);
   return a;
}
static void idle_check(Player *players, int count)
{
   const char *parts[] = {"playback_accent_0", "playback_accent_1", "playback_accent_2"};
   for (int i = 0; i < count; i++) {
      for (int j = 0; j < 3; j++) {
         assert(alpha(players[i].layout, parts[j]) == 40);
         assert(!strcmp(edje_object_part_state_get(players[i].layout, parts[j], NULL), "default"));
      }
      int queue[4], artwork[4];
      geometry(players[i].layout, "playlist_container", queue);
      geometry(players[i].layout, "artwork_pane", artwork);
      assert(!memcmp(queue, players[i].queue, sizeof(queue)));
      assert(!memcmp(artwork, players[i].artwork, sizeof(artwork)));
   }
}
int main(int argc, char **argv)
{
   ecore_evas_init(); edje_init();
   Ecore_Evas *canvas = ecore_evas_buffer_new(1000, 1000);
   assert(canvas && argc > 1);
   Player players[argc - 1];
   for (int i = 0; i < argc - 1; i++) {
      players[i].layout = edje_object_add(ecore_evas_get(canvas));
      assert(edje_object_file_set(players[i].layout, argv[i+1], "eclair_main_body"));
      int w, h;
      edje_object_size_min_get(players[i].layout, &w, &h);
      evas_object_resize(players[i].layout, w, h);
      evas_object_show(players[i].layout);
      edje_object_calc_force(players[i].layout);
      geometry(players[i].layout, "playlist_container", players[i].queue);
      geometry(players[i].layout, "artwork_pane", players[i].artwork);
   }
   settle(canvas, .1); idle_check(players, argc-1);
   send(players, argc-1, "signal_play"); settle(canvas, 1.0);
   for (int i = 0; i < argc-1; i++)
      assert(alpha(players[i].layout, "playback_accent_0") > 60);
   send(players, argc-1, "signal_pause"); settle(canvas, .6); idle_check(players, argc-1);
   /* Wait beyond a full cycle: no delayed animation may restart while idle. */
   settle(canvas, 3.5); idle_check(players, argc-1);
   send(players, argc-1, "signal_play"); settle(canvas, .2);
   send(players, argc-1, "signal_play"); settle(canvas, 1.0);
   for (int i = 0; i < argc-1; i++)
      assert(alpha(players[i].layout, "playback_accent_0") > 60);
   send(players, argc-1, "signal_stop"); settle(canvas, .6); idle_check(players, argc-1);
   /* Deleting an actively animated player must cancel its Edje timelines. */
   send(players, argc-1, "signal_play"); settle(canvas, .1);
   for (int i = 0; i < argc-1; i++) evas_object_del(players[i].layout);
   settle(canvas, .1);
   ecore_evas_free(canvas); edje_shutdown(); ecore_evas_shutdown();
   return 0;
}
