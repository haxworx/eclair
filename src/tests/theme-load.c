#include <Ecore_Evas.h>
#include <Edje.h>
#include <stdio.h>
int main(int argc, char **argv)
{
   Ecore_Evas *ee;
   int i, failed = 0;
   ecore_evas_init();
   edje_init();
   ee = ecore_evas_buffer_new(640, 480);
   if (!ee) return 2;
   for (i = 1; i < argc; i++) {
      Eina_List *groups = edje_file_collection_list(argv[i]), *l;
      char *group;
      if (!groups) failed++;
      EINA_LIST_FOREACH(groups, l, group) {
         Evas_Object *obj = edje_object_add(ecore_evas_get(ee));
         if (!edje_object_file_set(obj, argv[i], group)) {
            fprintf(stderr, "%s:%s failed\n", argv[i], group);
            failed++;
         }
         evas_object_resize(obj, 640, 480);
         edje_object_calc_force(obj);
         evas_object_del(obj);
      }
      edje_file_collection_list_free(groups);
   }
   ecore_evas_free(ee);
   edje_shutdown();
   ecore_evas_shutdown();
   return failed;
}
