#ifndef ECLAIR_COMPAT_H
#define ECLAIR_COMPAT_H

#include <stdlib.h>

static inline char *eclair_realpath(const char *path)
{
#ifdef _WIN32
  return _fullpath(NULL, path, 0);
#else
   return realpath(path, NULL);
#endif
}

#endif
