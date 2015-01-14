#include <string.h>
#include <runtime_reqs.h>

char *strdup(const char *s)
{
  size_t bufsize = strlen(s) + 1;
  char *retval = runtime_libc_malloc(bufsize);
  if(retval) memcpy(retval, s, bufsize);
  return retval;
}

