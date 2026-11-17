#include "eclair_cover.h"
#include <Emotion.h>
#include <Ecore_File.h>
#include "eclair.h"
#include "eclair_playlist.h"
#include "eclair_media_file.h"
#include "eclair_utils.h"

#define NUM_EXTENSIONS 3
#define NUM_COVER_NAMES 2

static const char *cover_extensions[NUM_EXTENSIONS] = { "jpg", "jpeg", "png" };
static const char *cover_names[NUM_COVER_NAMES] = { "cover", "folder" };

static char *_eclair_cover_build_path_from_filepath(Eclair_Cover_Manager *cover_manager, char *file_path, const char *cover_extension);
static char *_eclair_cover_build_path_from_artist_album(Eclair_Cover_Manager *cover_manager, const char *artist, const char *album, const char *cover_extension);
static char *_eclair_cover_build_cover_filename(const char *cover_name, const char *cover_extension);

void eclair_cover_init(Eclair_Cover_Manager *cover_manager, Eclair *eclair)
{
   if (!cover_manager) return;
   cover_manager->eclair = eclair;
   cover_manager->network = NULL;
   eclair_cover_network_init(cover_manager);
}

Eina_Bool eclair_cover_file_valid(Eclair_Cover_Manager *manager, const char *path)
{
   Eclair *app = manager->eclair;
   Evas_Object *canvas_object = app->cover ? app->cover : app->video.video_object;
   if (!canvas_object) return EINA_FALSE;
   Evas_Object *image = evas_object_image_add(evas_object_evas_get(canvas_object));
   evas_object_image_file_set(image, path, NULL);
   int w, h;
   evas_object_image_size_get(image, &w, &h);
   Eina_Bool valid = evas_object_image_load_error_get(image) == EVAS_LOAD_ERROR_NONE && w > 0 && h > 0;
   evas_object_del(image);
   return valid;
}

/* Called on the main loop: Emotion returns an Evas image on its canvas. */
char *eclair_cover_file_get(Eclair_Cover_Manager *cover_manager, Eclair_Media_File *file)
{
   Evas_Object *artwork, *video;
   unsigned char digest[20];
   char key[41], *path;
   int i;

   if (!cover_manager || !cover_manager->eclair || !file || !file->path)
      return NULL;

   path = eclair_cover_file_get_from_local(cover_manager, file->artist, file->album, file->path);
   if (path) {
      if (eclair_cover_file_valid(cover_manager, path)) return path;
      free(path);
   }

   video = cover_manager->eclair->video.video_object;
   if (!video || file->artwork_checked || eclair_utils_uri_is_mrl(file->path))
      return eclair_cover_network_get(cover_manager, file);
   file->artwork_checked = EINA_TRUE;

   /* Use the full path so equal track names in different albums cannot collide. */
   eina_sha1((const unsigned char *)file->path, strlen(file->path), digest);
   for (i = 0; i < 20; i++) sprintf(key + 2 * i, "%02x", digest[i]);
   path = malloc(strlen(cover_manager->eclair->config.covers_dir_path) + sizeof("embedded-.png") + 40);
   if (!path) return NULL;
   sprintf(path, "%sembedded-%s.png", cover_manager->eclair->config.covers_dir_path, key);
   if (ecore_file_exists(path)) {
      if (eclair_cover_file_valid(cover_manager, path)) return path;
      ecore_file_unlink(path);
   }

   artwork = emotion_file_meta_artwork_get(video, file->path, EMOTION_ARTWORK_PREVIEW_IMAGE);
   if (!artwork)
      artwork = emotion_file_meta_artwork_get(video, file->path, EMOTION_ARTWORK_IMAGE);
   if (artwork)
   {
      Eina_Bool saved = evas_object_image_save(artwork, path, NULL, NULL);
      evas_object_del(artwork);
      if (saved) return path;
   }
   free(path);
   return eclair_cover_network_get(cover_manager, file);
}

//Try to open the cover file on the disk if it exists
//NULL if failed. The returned path will have to be freed if success
char *eclair_cover_file_get_from_local(Eclair_Cover_Manager *cover_manager, const char *artist, const char *album, char *file_path)
{
   char *cover_filename, *cover_path, *directory;
   int i, j;

   for (i = 0; i < NUM_EXTENSIONS; i++)
   {
      if ((cover_path = _eclair_cover_build_path_from_filepath(cover_manager, file_path, cover_extensions[i]))
         && ecore_file_exists(cover_path))
         return cover_path;
      free(cover_path);
   }

   for (i = 0; i < NUM_EXTENSIONS; i++)
   {
      if ((cover_path = _eclair_cover_build_path_from_artist_album(cover_manager, artist, album, cover_extensions[i]))
         && ecore_file_exists(cover_path))
         return cover_path;
      free(cover_path);
   }

   directory = file_path ? ecore_file_dir_get(file_path) : NULL;
   if (!directory) return NULL;
   for (i = 0; i < NUM_COVER_NAMES; i++)
   {
      for (j = 0; j < NUM_EXTENSIONS; j++)
      {
         if ((cover_filename = _eclair_cover_build_cover_filename(cover_names[i], cover_extensions[j]))
            && (cover_path = eclair_utils_search_file(cover_filename, directory)))
         {
            free(cover_filename);
            free(directory);
            return cover_path;
         }
         free(cover_filename);
      }
   }

   free(directory);
   return NULL;
}

//Build the cover file path from file_path and cover_extension
//The returned path has to be freed
static char *_eclair_cover_build_path_from_filepath(Eclair_Cover_Manager *cover_manager, char *file_path, const char *cover_extension)
{
   char *cover_path, *filename_without_ext;

   if (!cover_manager || !file_path || !cover_extension || !cover_manager->eclair || strlen(file_path) <= 0)
      return NULL;

   filename_without_ext = eclair_utils_file_get_filename_without_ext(file_path);
   cover_path = malloc(strlen(cover_manager->eclair->config.covers_dir_path) + strlen(filename_without_ext) + strlen(cover_extension) + 2);
   sprintf(cover_path, "%s%s.%s", cover_manager->eclair->config.covers_dir_path, filename_without_ext, cover_extension);
   free(filename_without_ext);

   return cover_path;
}

//Build the cover file path from artist and album
//The returned path has to be freed
static char *_eclair_cover_build_path_from_artist_album(Eclair_Cover_Manager *cover_manager, const char *artist, const char *album, const char *cover_extension)
{
   char *filename, *path, *c;

   if (!cover_manager || !artist || !album || !cover_extension || !cover_manager->eclair
      || strlen(artist) <= 0 || strlen(album) <= 0)
      return NULL; 

   filename = malloc(strlen(artist) + strlen(album) + strlen(cover_extension) + 3);
   sprintf(filename, "%s_%s.%s", artist, album, cover_extension);
   //Remove forbidden '/' in the filename
   for (c = strchr(filename, '/'); c; c = strchr(c, '/'))
      *c = '_';

   path = malloc(strlen(cover_manager->eclair->config.covers_dir_path) + strlen(filename) + 1);
   sprintf(path, "%s%s", cover_manager->eclair->config.covers_dir_path, filename);
   free(filename);

   return path;
}

//Build the cover filename from the cover name ("cover", "folder", ...) and its extension
//The returned filename has to be freed
static char *_eclair_cover_build_cover_filename(const char *cover_name, const char *cover_extension)
{
   char *cover_filename;

   if (!cover_name || !cover_extension)
      return NULL;

   cover_filename = malloc(strlen(cover_name) + strlen(cover_extension) + 2);
   sprintf(cover_filename, "%s.%s", cover_name, cover_extension);

   return cover_filename;
}

//Set the cover of the file
Eina_Bool eclair_cover_current_set(Eclair_Cover_Manager *cover_manager, const char *uri)
{
   char *cover_path, *new_path;
   Eclair_Media_File *current_file;

   if (!cover_manager || !cover_manager->eclair || !uri
      || !(current_file = eclair_playlist_current_media_file(&cover_manager->eclair->playlist)))
      return EINA_FALSE;

   cover_path = eclair_utils_uri_to_path(uri);
   if (!cover_path) return EINA_FALSE;
   if (eclair_utils_uri_is_mrl(cover_path) || !eclair_cover_file_valid(cover_manager, cover_path)) {
      free(cover_path);
      return EINA_FALSE;
   }

   if (!(new_path = _eclair_cover_build_path_from_artist_album(cover_manager, current_file->artist, current_file->album, eclair_utils_file_get_extension(cover_path)))
      && !(new_path = _eclair_cover_build_path_from_filepath(cover_manager, current_file->path, eclair_utils_file_get_extension(cover_path))))
   {
      free(cover_path);
      return EINA_FALSE;
   }
   if (!ecore_file_cp(cover_path, new_path))
   {
      free(cover_path);
      free(new_path);
      return EINA_FALSE;
   }

   free(cover_path);
   free(current_file->cover_path);
   current_file->cover_path = new_path;
   eclair_update_current_file_info(cover_manager->eclair, 1);
   return EINA_TRUE;
}
