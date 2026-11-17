#ifndef _ECLAIR_COVER_H_
#define _ECLAIR_COVER_H_

#include <Evas.h>
#include "eclair_types.h"

struct _Eclair_Cover_Manager
{
   Eclair *eclair;
   struct _Eclair_Cover_Network *network;
};

void eclair_cover_init(Eclair_Cover_Manager *cover_manager, Eclair *eclair);
Eina_Bool eclair_cover_file_valid(Eclair_Cover_Manager *manager, const char *path);
void eclair_cover_network_init(Eclair_Cover_Manager *manager);
void eclair_cover_shutdown(Eclair_Cover_Manager *manager);
char *eclair_cover_network_get(Eclair_Cover_Manager *manager, Eclair_Media_File *file);
char *eclair_cover_file_get(Eclair_Cover_Manager *cover_manager, Eclair_Media_File *file);
char *eclair_cover_file_get_from_local(Eclair_Cover_Manager *cover_manager, const char *artist, const char *album, char *file_path);
Eina_Bool eclair_cover_current_set(Eclair_Cover_Manager *cover_manager, const char *uri);

#endif
