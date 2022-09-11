#ifndef NET_H
#define NET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <curl/curl.h>
#include <json-c/json.h>

struct MemoryStruct {
    uint8_t* memory;
    size_t size;
};

void net_init(void);
void net_deinit(void);

struct MemoryStruct* downloadToRam(const char* URL);
size_t downloadToFile(const char*, const char*);
void freeDownload(struct MemoryStruct* chunk);
struct json_object* fetchJSON(const char*);

#ifdef __cplusplus
}
#endif

#endif
