#ifndef _ECLAIR_DIALOGS_H_
#define _ECLAIR_DIALOGS_H_

#include <Evas.h>
#include <Ecore.h>
#include "eclair_types.h"

struct _Eclair_Dialogs_Manager
{
   //Menu widget vars
   Eclair_Menu *menu;

   Eclair_Menu_Item *menu_playlist_item;
   Eclair_Menu *menu_playlist_menu;
   Eclair_Menu_Item *menu_about_item;
   Evas_Object *about_window;

   Ecore_Job *theme_job;
   char *pending_theme;
   Evas_Object *file_window;
   Evas_Object *file_selector;
   Evas_Object *file_error;
   int file_action;
   Evas_Object *playlist_window;
   Evas_Object *playlist_rows;
   Evas_Object *playlist_search;
   Evas_Object *playlist_status;
   Ecore_Timer *playlist_refresh;
   unsigned long playlist_signature;

   Eclair *eclair;
};

void eclair_dialogs_init(Eclair_Dialogs_Manager *dialogs_manager, Eclair *eclair);
void eclair_dialogs_shutdown(Eclair_Dialogs_Manager *dialogs_manager);
void eclair_dialogs_open_fc_add_files(Eclair_Dialogs_Manager *dialogs_manager);
void eclair_dialogs_open_fc_load_playlist(Eclair_Dialogs_Manager *dialogs_manager);
void eclair_dialogs_open_fc_save_playlist(Eclair_Dialogs_Manager *dialogs_manager);
void eclair_dialogs_popup_menu(Eclair_Dialogs_Manager *dialogs_manager);
void eclair_dialogs_about_window(Eclair_Dialogs_Manager *dialogs_manager);
void eclair_dialogs_playlist_window(Eclair_Dialogs_Manager *dialogs_manager);
void eclair_dialogs_search_window(Eclair_Dialogs_Manager *dialogs_manager);

#endif
