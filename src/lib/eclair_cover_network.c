#include "eclair.h"
#include "eclair_media_file.h"
#include <Ecore_Con.h>
#include <Ecore_File.h>
#include <json-c/json.h>

/* One asynchronous search/download at a time; no playlist pointers survive it. */
struct _Eclair_Cover_Network
{
   Eclair_Cover_Manager *manager;
   Ecore_Con_Url *fetch;
   Ecore_Event_Handler *data_handler, *complete_handler;
   Eina_Strbuf *response;
   Eina_Hash *attempted;
   char *path, *artist, *album;
   Eina_Bool image, oversized;
};

static void _request_clear(struct _Eclair_Cover_Network *net)
{
   if (net->fetch) ecore_con_url_free(net->fetch);
   net->fetch = NULL;
   if (net->response) eina_strbuf_reset(net->response);
   free(net->path); net->path = NULL;
   free(net->artist); net->artist = NULL;
   free(net->album); net->album = NULL;
   net->image = net->oversized = EINA_FALSE;
}

static Eina_Bool _fetch(struct _Eclair_Cover_Network *net, const char *url)
{
   net->fetch = ecore_con_url_new(url);
   if (!net->fetch) return EINA_FALSE;
   ecore_con_url_timeout_set(net->fetch, 15);
   ecore_con_url_additional_header_add(net->fetch, "User-Agent", "Eclair/0.0.1");
   return ecore_con_url_get(net->fetch);
}

static Eina_Bool _data(void *data, int type EINA_UNUSED, void *event)
{
   struct _Eclair_Cover_Network *net = data;
   Ecore_Con_Event_Url_Data *ev = event;
   if (ev->url_con != net->fetch) return ECORE_CALLBACK_PASS_ON;
   size_t limit = net->image ? 8 * 1024 * 1024 : 1024 * 1024;
   if (eina_strbuf_length_get(net->response) + ev->size > limit)
      net->oversized = EINA_TRUE;
   if (!net->oversized)
      eina_strbuf_append_length(net->response, (const char *)ev->data, ev->size);
   return ECORE_CALLBACK_PASS_ON;
}

static const char *_string(json_object *obj, const char *key)
{
   json_object *value;
   if (!json_object_object_get_ex(obj, key, &value) ||
       !json_object_is_type(value, json_type_string)) return NULL;
   return json_object_get_string(value);
}

static char *_image_url(struct _Eclair_Cover_Network *net)
{
   json_object *root = json_tokener_parse(eina_strbuf_string_get(net->response));
   json_object *results;
   char *url = NULL;
   if (!root) return NULL;
   if (json_object_object_get_ex(root, "results", &results) &&
       json_object_is_type(results, json_type_array)) {
      for (size_t i = 0; i < json_object_array_length(results); i++) {
         json_object *item = json_object_array_get_idx(results, i);
         const char *artist = _string(item, "artistName");
         const char *album = _string(item, "collectionName");
         const char *art = _string(item, "artworkUrl100");
         if (!artist || !album || !art || strcasecmp(artist, net->artist) ||
             strcasecmp(album, net->album) || strncmp(art, "https://", 8)) continue;
         Eina_Strbuf *buf = eina_strbuf_new();
         eina_strbuf_append(buf, art);
         eina_strbuf_replace_all(buf, "100x100bb", "600x600bb");
         url = eina_strbuf_string_steal(buf);
         eina_strbuf_free(buf);
         break;
      }
   }
   json_object_put(root);
   return url;
}

static Eina_Bool _complete(void *data, int type EINA_UNUSED, void *event)
{
   struct _Eclair_Cover_Network *net = data;
   Ecore_Con_Event_Url_Complete *ev = event;
   if (ev->url_con != net->fetch) return ECORE_CALLBACK_PASS_ON;
   Eclair *app = net->manager->eclair;
   if (!net->oversized && ev->status == 200) {
      if (!net->image) {
         char *url = _image_url(net);
         if (url) {
            ecore_con_url_free(net->fetch); net->fetch = NULL;
            eina_strbuf_reset(net->response);
            net->image = EINA_TRUE;
            Eina_Bool started = _fetch(net, url);
            free(url);
            if (started) return ECORE_CALLBACK_PASS_ON;
         }
      } else if (app->cover || app->video.video_object) {
         Evas_Object *canvas_object = app->cover ? app->cover : app->video.video_object;
         Evas_Object *image = evas_object_image_add(evas_object_evas_get(canvas_object));
         evas_object_image_memfile_set(image, (void *)eina_strbuf_string_get(net->response),
                                      eina_strbuf_length_get(net->response), NULL, NULL);
         int w, h;
         evas_object_image_size_get(image, &w, &h);
         if (evas_object_image_load_error_get(image) == EVAS_LOAD_ERROR_NONE && w > 0 && h > 0) {
            char temporary[PATH_MAX];
            if (snprintf(temporary, sizeof(temporary), "%s.tmp.png", net->path) < (int)sizeof(temporary)) {
               if (evas_object_image_save(image, temporary, NULL, NULL))
                  ecore_file_mv(temporary, net->path);
               ecore_file_unlink(temporary);
            }
         }
         evas_object_del(image);
      }
   }
   _request_clear(net);
   /* Refresh the current selection, which may have changed during the fetch. */
   eclair_update_current_file_info(app, EINA_FALSE);
   return ECORE_CALLBACK_PASS_ON;
}

void eclair_cover_network_init(Eclair_Cover_Manager *manager)
{
   if (!ecore_con_url_init()) return;
   struct _Eclair_Cover_Network *net = calloc(1, sizeof(*net));
   if (!net) { ecore_con_url_shutdown(); return; }
   manager->network = net;
   net->manager = manager;
   net->response = eina_strbuf_new();
   net->attempted = eina_hash_string_superfast_new(NULL);
   net->data_handler = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, _data, net);
   net->complete_handler = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, _complete, net);
   if (!net->response || !net->attempted || !net->data_handler || !net->complete_handler)
      eclair_cover_shutdown(manager);
}

void eclair_cover_shutdown(Eclair_Cover_Manager *manager)
{
   struct _Eclair_Cover_Network *net = manager->network;
   if (!net) return;
   if (net->data_handler) ecore_event_handler_del(net->data_handler);
   if (net->complete_handler) ecore_event_handler_del(net->complete_handler);
   _request_clear(net);
   eina_strbuf_free(net->response);
   if (net->attempted) eina_hash_free(net->attempted);
   free(net);
   manager->network = NULL;
   ecore_con_url_shutdown();
}

char *eclair_cover_network_get(Eclair_Cover_Manager *manager, Eclair_Media_File *file)
{
   struct _Eclair_Cover_Network *net = manager->network;
   if (!net || !file->artist || !file->artist[0] || !file->album || !file->album[0]) return NULL;
   Eina_Strbuf *query = eina_strbuf_new();
   eina_strbuf_append(query, file->artist);
   eina_strbuf_append_char(query, '\n');
   eina_strbuf_append(query, file->album);
   unsigned char digest[20];
   char key[41], path[PATH_MAX];
   eina_sha1((const unsigned char *)eina_strbuf_string_get(query), eina_strbuf_length_get(query), digest);
   for (int i = 0; i < 20; i++) sprintf(key + 2 * i, "%02x", digest[i]);
   if (snprintf(path, sizeof(path), "%snetwork-%s.png", manager->eclair->config.covers_dir_path, key) >= (int)sizeof(path)) {
      eina_strbuf_free(query); return NULL;
   }
   if (ecore_file_exists(path)) {
      if (eclair_cover_file_valid(manager, path)) { eina_strbuf_free(query); return strdup(path); }
      ecore_file_unlink(path);
   }
   if (net->fetch || eina_hash_find(net->attempted, key)) { eina_strbuf_free(query); return NULL; }
   eina_hash_add(net->attempted, key, (void *)1);
   net->path = strdup(path);
   net->artist = strdup(file->artist);
   net->album = strdup(file->album);
   Eina_Strbuf *url = eina_strbuf_new();
   eina_strbuf_append(url, "https://itunes.apple.com/search?entity=album&media=music&limit=10&term=");
   const unsigned char *p = (const unsigned char *)eina_strbuf_string_get(query);
   for (; *p; p++) {
      if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))
         eina_strbuf_append_char(url, *p);
      else eina_strbuf_append_printf(url, "%%%02X", *p);
   }
   if (!_fetch(net, eina_strbuf_string_get(url))) _request_clear(net);
   eina_strbuf_free(query);
   eina_strbuf_free(url);
   return NULL;
}
