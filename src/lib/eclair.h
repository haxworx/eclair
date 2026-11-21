#ifndef _ECLAIR_H_
#define _ECLAIR_H_

#include <Evas.h>
#include "eclair_video.h"
#include "eclair_playlist.h"
#include "eclair_subtitles.h"
#include "eclair_dialogs.h"
#include "eclair_meta_tag.h"
#include "eclair_cover.h"
#include "eclair_config.h"
#include "eclair_types.h"
#include "eclair_menu.h"
#include "eclair_utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum _Eclair_State
{
   ECLAIR_PLAYING = 0,
   ECLAIR_PAUSE,
   ECLAIR_STOP
};

struct _Eclair
{
   //Video related vars
   Eclair_Video video;
   Eclair_Video_Module video_module;

   //Gui related vars
   Eclair_Window *gui_window;
   Eclair_Window *playlist_window;
   Eclair_Window *equalizer_window;
   Eclair_Window *cover_window;
   Evas_Object *playlist_container;
   Eclair_Window *playlist_container_owner;
   Evas_Object *previous_cover;
   Evas_Object *cover;
   Eclair_Window *cover_owner;

   Eina_Bool use_progress_bar_drag_for_time;
   Eina_Bool dont_update_progressbar;
   char *gui_theme_file;
   Eina_Bool theme_from_args;
   int playlist_entry_height;
   Eclair_Engine gui_engine;

   Ecore_Pipe *media_updates_pipe;
   pthread_mutex_t media_updates_mutex;
   Eclair_Media_Update *media_updates;

   //Core vars
   Eclair_State state;
   Eclair_Playlist playlist;
   Eclair_Subtitles subtitles;
   Eclair_Dialogs_Manager dialogs_manager;
   Eclair_Meta_Tag_Manager meta_tag_manager;
   Eclair_Cover_Manager cover_manager;
   Eclair_Config config;
   Eina_Bool start_playing;
   double seek_to_pos;

   Eina_Bool help_requested;

   //Arguments
   int *argc;
   char ***argv;
};

//core functions
Eina_Bool eclair_init(Eclair *eclair, int *argc, char ***argv);
void eclair_shutdown(Eclair *eclair);
void eclair_update(Eclair *eclair);
void eclair_update_current_file_info(Eclair *eclair, Eina_Bool force_cover_update);
void eclair_gui_cover_set(Eclair *eclair, const char *cover_path, Eina_Bool force_cover_update);
void eclair_send_signal_to_all_windows(Eclair *eclair, const char *signal);
void eclair_all_windows_text_set(Eclair *eclair, const char *field_name, const char *text);

Eina_Bool eclair_theme_valid(const char *path);
Eina_Bool eclair_theme_set(Eclair *eclair, const char *path);

//navigation functions
void eclair_play_file(Eclair *eclair, const char *path);
void eclair_play_current(Eclair *eclair);
void eclair_play_prev(Eclair *eclair);
void eclair_play_next(Eclair *eclair);
void eclair_pause(Eclair *eclair);
void eclair_play(Eclair *eclair);
void eclair_stop(Eclair *eclair);
double eclair_progress_rate_get(Eclair *eclair);
void eclair_progress_rate_set(Eclair *eclair, double progress_rate);
double eclair_position_get(Eclair *eclair);
void eclair_position_set(Eclair *eclair, double position);
void eclair_audio_level_set(Eclair *eclair, double audio_level);

#endif
