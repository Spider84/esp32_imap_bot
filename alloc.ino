#include <Arduino.h>

void * my_malloc(size_t size)
{
  void *mem;
#if defined(BOARD_HAS_PSRAM)
  mem = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  if (!mem)
#endif  
    mem = heap_caps_malloc(size, MALLOC_CAP_DEFAULT);
  return mem;
}
