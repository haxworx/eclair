#include "eclair_config.h"
#include <Ecore_File.h>
#include <Eet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

int main(void)
{
   ecore_file_init();
   char directory[] = "/tmp/eclair-config-XXXXXX";
   assert(mkdtemp(directory));
   setenv("XDG_CONFIG_HOME", directory, 1);
   /* Reject overlong paths before creating directories or taking a lock. */
   char oversized[PATH_MAX + 32];
   memset(oversized, 'x', sizeof(oversized) - 1);
   oversized[0] = '/';
   oversized[sizeof(oversized) - 1] = '\0';
   setenv("XDG_CONFIG_HOME", oversized, 1);
   Eclair_Config rejected;
   assert(!eclair_config_init(&rejected));
   assert(!rejected.lock_held && !rejected.eet_initialized);
   setenv("XDG_CONFIG_HOME", directory, 1);
   Eclair_Config config;
   assert(eclair_config_init(&config));
   assert(strstr(config.config_file_path, "eclair.eet"));
   /* A separate process cannot open the same configuration while it is held. */
   pid_t child = fork();
   assert(child >= 0);
   if (!child) {
      close(config.lock_fd);
      Eclair_Config second;
      _exit(!eclair_config_init(&second) && second.already_running ? 0 : 1);
   }
   int status;
   assert(waitpid(child, &status, 0) == child);
   assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
   const char *theme = "/tmp/Music & \"themes\"/日本語 <skin>.edj";
   eclair_config_set_prop_string(&config, "appearance", "theme", "old");
   eclair_config_set_prop_string(&config, "appearance", "theme", theme);
   eclair_config_set_prop_int(&config, "main_window", "x", -42);
   eclair_config_set_prop_float(&config, "volume", "level", .75f);
   eclair_config_set_prop_string(&config, "test", "empty", "");
   eclair_config_shutdown(&config);
   assert(eclair_config_init(&config));
   char *value = NULL;
   assert(eclair_config_get_prop_string(&config, "appearance", "theme", &value));
   assert(!strcmp(value, theme)); free(value);
   assert(eclair_config_get_prop_string(&config, "test", "empty", &value));
   assert(!strcmp(value, "")); free(value);
   int x;
   float volume;
   assert(eclair_config_get_prop_int(&config, "main_window", "x", &x) && x == -42);
   assert(eclair_config_get_prop_float(&config, "volume", "level", &volume) && volume == .75f);
   assert(!eclair_config_get_prop_string(&config, "missing", "value", &value));
   Eet_File *file = eet_open(config.config_file_path, EET_FILE_MODE_READ);
   assert(file); eet_close(file);
   eclair_config_shutdown(&config);
   /* Even an unclean exit must release the lock without removing the file. */
   int ready[2];
   assert(pipe(ready) == 0);
   child = fork();
   assert(child >= 0);
   if (!child) {
      close(ready[0]);
      Eclair_Config owner;
      if (!eclair_config_init(&owner)) _exit(1);
      assert(write(ready[1], "1", 1) == 1);
      for (;;) pause();
   }
   close(ready[1]);
   char byte;
   assert(read(ready[0], &byte, 1) == 1);
   close(ready[0]);
   Eclair_Config second;
   assert(!eclair_config_init(&second));
   assert(second.already_running);
   assert(kill(child, SIGKILL) == 0);
   assert(waitpid(child, &status, 0) == child && WIFSIGNALED(status));
   assert(eclair_config_init(&config));
   eclair_config_shutdown(&config);
   ecore_file_recursive_rm(directory);
   ecore_file_shutdown();
}
