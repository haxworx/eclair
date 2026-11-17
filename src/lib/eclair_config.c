#include "eclair.h"
#include <Ecore_File.h>
#include <Eet.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

/* Concatenate only after checking the complete path, including its NUL. */
static Eina_Bool
_eclair_config_path_set(char *out, size_t capacity, const char *base, const char *suffix)
{
   size_t base_len = strlen(base), suffix_len = strlen(suffix);
   if (base_len >= capacity || suffix_len >= capacity - base_len)
      return EINA_FALSE;
   memcpy(out, base, base_len);
   memcpy(out + base_len, suffix, suffix_len + 1);
   return EINA_TRUE;
}

/* Copy legacy data without replacing files already saved in the XDG directory.
 * Do not follow symlinks in the old cache. */
static void _eclair_config_migrate(const char *source, const char *destination)
{
   struct stat st;
#ifndef _WIN32
   if (lstat(source, &st)) return;
#endif
   if (S_ISREG(st.st_mode)) {
      if (!ecore_file_exists(destination) && !ecore_file_cp(source, destination))
         fprintf(stderr, "Config: unable to migrate %s\n", source);
   } else if (S_ISDIR(st.st_mode)) {
      Eina_List *entries;
      char *entry;
      if (!ecore_file_mkpath(destination)) return;
      entries = ecore_file_ls(source);
      EINA_LIST_FREE(entries, entry) {
         char from[PATH_MAX], to[PATH_MAX];
         if (snprintf(from, sizeof(from), "%s/%s", source, entry) < (int)sizeof(from) &&
             snprintf(to, sizeof(to), "%s/%s", destination, entry) < (int)sizeof(to))
            _eclair_config_migrate(from, to);
         free(entry);
      }
   }
}

Eina_Bool eclair_config_init(Eclair_Config *config)
{
   const char *home = getenv("HOME"), *base = getenv("XDG_CONFIG_HOME");
   char fallback[PATH_MAX], legacy[PATH_MAX];
   if (!config) return EINA_FALSE;
   memset(config, 0, sizeof(*config));
   if (!base || base[0] != '/') {
      if (!home || home[0] != '/' ||
          !_eclair_config_path_set(fallback, sizeof(fallback), home, "/.config"))
         return EINA_FALSE;
      base = fallback;
   }
   char lock_path[PATH_MAX];
   if (!_eclair_config_path_set(config->config_dir_path, sizeof(config->config_dir_path), base, "/eclair/") ||
       !_eclair_config_path_set(config->covers_dir_path, sizeof(config->covers_dir_path), config->config_dir_path, "covers/") ||
       !_eclair_config_path_set(config->config_file_path, sizeof(config->config_file_path), config->config_dir_path, "eclair.eet") ||
       !_eclair_config_path_set(config->default_playlist_path, sizeof(config->default_playlist_path), config->config_dir_path, "playlist.m3u") ||
       !_eclair_config_path_set(lock_path, sizeof(lock_path), config->config_dir_path, "instance.lock"))
      return EINA_FALSE;
   if (!ecore_file_mkpath(config->config_dir_path)) return EINA_FALSE;
   /* Keep this file in place: unlinking it would let another process lock a
    * different inode while we still own the original lock. */
   config->lock_fd = open(lock_path, O_CREAT | O_RDWR | O_CLOEXEC, 0600);
   if (config->lock_fd < 0) {
      fprintf(stderr, "Unable to open instance lock: %s\n", strerror(errno));
      return EINA_FALSE;
   }
   if (flock(config->lock_fd, LOCK_EX | LOCK_NB)) {
      if (errno == EWOULDBLOCK || errno == EAGAIN)
         config->already_running = EINA_TRUE;
      else
         fprintf(stderr, "Unable to lock configuration: %s\n", strerror(errno));
      close(config->lock_fd);
      return EINA_FALSE;
   }
   config->lock_held = EINA_TRUE;
   /* Prefer the most recent settings from the briefly used directory name. */
   if (snprintf(legacy, sizeof(legacy), "%s/ecrire", base) < (int)sizeof(legacy))
      _eclair_config_migrate(legacy, config->config_dir_path);
   if (home && snprintf(legacy, sizeof(legacy), "%s/.eclair", home) < (int)sizeof(legacy))
      _eclair_config_migrate(legacy, config->config_dir_path);
   if (!ecore_file_mkpath(config->covers_dir_path)) goto fail;
   if (!eet_init()) goto fail;
   config->eet_initialized = EINA_TRUE;
   eclair_config_load(config);
   return EINA_TRUE;
fail:
   close(config->lock_fd);
   config->lock_held = EINA_FALSE;
   return EINA_FALSE;
}

/* Each setting is a compressed Eet entry named section/property. */
void eclair_config_load(Eclair_Config *config)
{
   if (!config) return;
   if (config->properties) eina_hash_free(config->properties);
   config->properties = eina_hash_string_superfast_new(free);
   Eet_File *file = eet_open(config->config_file_path, EET_FILE_MODE_READ);
   if (!file) {
      eclair_config_set_default(config);
      return;
   }
   int count = 0;
   char **keys = eet_list(file, "*", &count);
   for (int i = 0; i < count; i++) {
      int size = 0;
      char *value = eet_read(file, keys[i], &size);
      if (value && size > 0 && value[size - 1] == '\0' &&
          strlen(value) == (size_t)size - 1)
         eina_hash_add(config->properties, keys[i], value);
      else free(value);
   }
   free(keys);
   eet_close(file);
}

void eclair_config_save(Eclair_Config *config)
{
   if (!config || !config->properties || !config->config_file_path[0]) return;
   char path[PATH_MAX];
   if (snprintf(path, sizeof(path), "%s.tmp", config->config_file_path) >= (int)sizeof(path)) return;
   Eet_File *file = eet_open(path, EET_FILE_MODE_WRITE);
   if (!file) return;
   Eina_Bool ok = EINA_TRUE;
   Eina_Iterator *iterator = eina_hash_iterator_tuple_new(config->properties);
   Eina_Hash_Tuple *entry;
   EINA_ITERATOR_FOREACH(iterator, entry)
      if (eet_write(file, entry->key, entry->data, strlen(entry->data) + 1, 1) <= 0)
         ok = EINA_FALSE;
   eina_iterator_free(iterator);
   if (eet_close(file) != EET_ERROR_NONE) ok = EINA_FALSE;
   if (!ok || rename(path, config->config_file_path)) {
      unlink(path);
      fprintf(stderr, "Config: unable to save %s\n", config->config_file_path);
   }
}

void eclair_config_set_default(Eclair_Config *config)
{
   eclair_config_set_prop_int(config, "gui_window", "x", 0);
   eclair_config_set_prop_int(config, "gui_window", "y", 0);
}

void eclair_config_set_prop_string(Eclair_Config *config, const char *node_name, const char *prop_name, const char *prop_value)
{
   if (!config || !node_name || !prop_name || !prop_value) return;
   if (!config->properties) config->properties = eina_hash_string_superfast_new(free);
   Eina_Strbuf *key = eina_strbuf_new();
   eina_strbuf_append_printf(key, "%s/%s", node_name, prop_name);
   free(eina_hash_set(config->properties, eina_strbuf_string_get(key), strdup(prop_value)));
   eina_strbuf_free(key);
}

//Set prop_name in node_name to the integer prop_value
void eclair_config_set_prop_int(Eclair_Config *config, const char *node_name, const char *prop_name, int prop_value)
{
   char string[40];
   if (snprintf(string, 40, "%d", prop_value) > 0)
      eclair_config_set_prop_string(config, node_name, prop_name, string);
}

//Set prop_name in node_name to the float prop_value
void eclair_config_set_prop_float(Eclair_Config *config, const char *node_name, const char *prop_name, float prop_value)
{
   char string[40];
   if (snprintf(string, 40, "%f", prop_value) > 0)
      eclair_config_set_prop_string(config, node_name, prop_name, string);
}

//Get the string prop_name in node_name
//Return 0 if failed
//The string will have to be freed if success
int eclair_config_get_prop_string(Eclair_Config *config, const char *node_name, const char *prop_name, char **prop_value)
{
   if (!config || !config->properties || !node_name || !prop_name || !prop_value) return 0;
   Eina_Strbuf *key = eina_strbuf_new();
   eina_strbuf_append_printf(key, "%s/%s", node_name, prop_name);
   const char *value = eina_hash_find(config->properties, eina_strbuf_string_get(key));
   *prop_value = value ? strdup(value) : NULL;
   eina_strbuf_free(key);
   return *prop_value != NULL;
}

//Get the integer prop_name in node_name
//Return 0 if failed
int eclair_config_get_prop_int(Eclair_Config *config, const char *node_name, const char *prop_name, int *prop_value)
{
   char *value_string;

   if (!eclair_config_get_prop_string(config, node_name, prop_name, &value_string))
      return 0;

   if (sscanf(value_string, "%d", prop_value) != 1)
   {
      free(value_string);      
      return 0;
   }
   free(value_string);
   return 1;
}

//Get the float prop_name in node_name
//Return 0 if failed
int eclair_config_get_prop_float(Eclair_Config *config, const char *node_name, const char *prop_name, float *prop_value)
{
   char *value_string;

   if (!eclair_config_get_prop_string(config, node_name, prop_name, &value_string))
      return 0;

   if (sscanf(value_string, "%f", prop_value) != 1)
   {
      free(value_string);      
      return 0;
   }
   free(value_string);
   return 1;
}

//Shutdown the config manager
void eclair_config_shutdown(Eclair_Config *config)
{
   if (!config)
      return;

   eclair_config_save(config);
   if (config->properties) eina_hash_free(config->properties);
   config->properties = NULL;
   if (config->eet_initialized) eet_shutdown();
   config->eet_initialized = EINA_FALSE;
   if (config->lock_held) {
      close(config->lock_fd);
      config->lock_held = EINA_FALSE;
   }
}
