#ifndef _EEPROM_H
#define _EEPROM_H
 
typedef struct __attribute__((aligned(4), packed)) {
  uint16_t mail_time;
  uint16_t crc;
} E_config;

extern E_config e_config;

#ifdef USE_EEPROM
class SpiRamAllocator {
  public:
    void* allocate(size_t size) {
      return my_malloc(size);
    }
    void deallocate(void* pointer) {
      heap_caps_free(pointer);
    }
};

typedef ArduinoJson::Internals::DynamicJsonBufferBase<SpiRamAllocator> SpiRamJsonBuffer;
#endif
#endif
