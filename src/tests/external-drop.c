/* Exercise the external Xdnd URI-list/MOVE transfer used by EFM. */
#include "eclair.h"
#include "eclair_window.h"
#include "eclair_scale.h"
#include "eclair_callbacks.h"
#include "eclair_media_file.h"
#include <X11/Xlib.h>
#include <assert.h>
#include <Ecore_File.h>
static Display *display;
static Window source, target;
static Eclair app;
static char payload[PATH_MAX + 16];
static int phase, accepted, requested, dropped;
static Atom atom(const char *name) { return XInternAtom(display, name, False); }
static void message(const char *name, long b, long c, long d, long e)
{
   XEvent ev = {0};
   ev.xclient.type = ClientMessage;
   ev.xclient.window = target;
   ev.xclient.message_type = atom(name);
   ev.xclient.format = 32;
   ev.xclient.data.l[0] = source;
   ev.xclient.data.l[1] = b; ev.xclient.data.l[2] = c;
   ev.xclient.data.l[3] = d; ev.xclient.data.l[4] = e;
   XSendEvent(display, target, False, NoEventMask, &ev); XFlush(display);
}
static Eina_Bool events(void *data EINA_UNUSED, Ecore_Fd_Handler *handler EINA_UNUSED)
{
   while (XPending(display)) {
      XEvent ev;
      XNextEvent(display, &ev);
      if (ev.type == ClientMessage && ev.xclient.message_type == atom("XdndStatus")) {
         accepted = ev.xclient.data.l[1] & 1;
         printf("XdndStatus accepted=%d\n", accepted); fflush(stdout);
         if (accepted && !dropped) { dropped = 1; message("XdndDrop", 0, CurrentTime, 0, 0); }
      } else if (ev.type == SelectionRequest) {
         requested++;
         XSelectionRequestEvent *req = &ev.xselectionrequest;
         char *name = XGetAtomName(display, req->target);
         printf("SelectionRequest target=%s\n", name); fflush(stdout); XFree(name);
         Atom property = req->property ? req->property : req->target;
         XChangeProperty(display, req->requestor, property, req->target, 8,
                         PropModeReplace, (unsigned char *)payload, strlen(payload));
         XEvent reply = {0};
         reply.xselection.type = SelectionNotify;
         reply.xselection.display = display; reply.xselection.requestor = req->requestor;
         reply.xselection.selection = req->selection; reply.xselection.target = req->target;
         reply.xselection.property = property; reply.xselection.time = req->time;
         XSendEvent(display, req->requestor, False, 0, &reply); XFlush(display);
      }
   }
   return ECORE_CALLBACK_RENEW;
}
static Eina_Bool start(void *data EINA_UNUSED)
{
   elm_win_render(app.gui_window->window);
   int x, y, bx, by;
   Ecore_Evas *ee = ecore_evas_ecore_evas_get(app.gui_window->evas);
   target = ecore_evas_window_get(ee);
   ecore_evas_geometry_get(ee, &x, &y, NULL, NULL);
   edje_object_part_geometry_get(app.gui_window->edje_object, "body", &bx, &by, NULL, NULL);
   int ex, ey, ew, eh;
   evas_object_geometry_get(app.gui_window->edje_object, &ex, &ey, &ew, &eh);
   printf("window=%d,%d layout=%d,%d,%d,%d body=%d,%d target=%lx\n", x,y,ex,ey,ew,eh,bx,by,target); fflush(stdout);
   message("XdndEnter", 5L << 24, atom("text/uri-list"), 0, 0);
   message("XdndPosition", 0, ((x + bx + 8) << 16) | (y + by + 8), CurrentTime, atom("XdndActionMove"));
   return ECORE_CALLBACK_CANCEL;
}
static Eina_Bool check(void *data EINA_UNUSED)
{
   if (app.playlist.playlist) { phase = 1; ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
   return ECORE_CALLBACK_RENEW;
}
static Eina_Bool timeout(void *data EINA_UNUSED) { ecore_main_loop_quit(); return ECORE_CALLBACK_CANCEL; }
int main(int argc, char **argv)
{
   elm_init(argc, argv); eclair_scale_init();
   app.meta_tag_manager.meta_tag_delete_thread = 1;
   app.config.properties = eina_hash_string_superfast_new(free);
   eclair_playlist_init(&app.playlist, &app);
   app.gui_window = eclair_window_create(argv[1], "main", "XDND receiver", ECLAIR_SOFTWARE, &app, 1);
   assert(app.gui_window);
   eclair_window_add_default_callbacks(app.gui_window, &app);
   app.cover_manager.eclair = &app;
   char directory[] = "/tmp/eclair-efm-XXXXXX";
   assert(mkdtemp(directory));
   char path[PATH_MAX];
   snprintf(path, sizeof(path), "%s/song.mp3", directory);
   FILE *file = fopen(path, "w"); assert(file); fclose(file);
   snprintf(payload, sizeof(payload), "file://%s\r\n",
            argc > 2 && !strcmp(argv[2], "folder") ? directory : path);
   display = XOpenDisplay(NULL); assert(display);
   source = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 16, 16, 0, 0, 0);
   XSetSelectionOwner(display, atom("XdndSelection"), source, CurrentTime);
   ecore_main_fd_handler_add(ConnectionNumber(display), ECORE_FD_READ, events, NULL, NULL, NULL);
   ecore_timer_add(.3, start, NULL);
   ecore_timer_add(.05, check, NULL);
   ecore_timer_add(4, timeout, NULL);
   ecore_main_loop_begin();
   printf("result tracks=%u accepted=%d requested=%d\n", eina_list_count(app.playlist.playlist), accepted, requested);
   unlink(path);
   rmdir(directory);
   return !(phase && accepted && requested);
}
