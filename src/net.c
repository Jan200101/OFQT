#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <curl/curl.h>
#include <json.h>

#include "net.h"
#include "fs.h"

// official servers only whitelist some UAs
#define USER_AGENT "murse/0.1 (" NAME "/" VERSION ")"

void net_init()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

void net_deinit()
{
    curl_global_cleanup();
}

static inline size_t memoryCallback(const void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;

    uint8_t* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr)
        return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

struct MemoryStruct* downloadToRam(const char* url)
{
    CURL* curl_handle;
    CURLcode res = CURLE_OK;

    struct MemoryStruct* chunk = malloc(sizeof(struct MemoryStruct));

    if (chunk)
    {
        chunk->memory = malloc(1);
        if (!chunk->memory)
        {
            free(chunk);
            return NULL;
        }
        chunk->memory[0] = 0;
        chunk->size = 0;

        curl_handle = curl_easy_init();

        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, memoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)chunk);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, USER_AGENT);
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

        res = curl_easy_perform(curl_handle);

        long http_code = 0;
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);

        if (res != CURLE_OK || http_code != 200)
        {
            freeDownload(chunk);
            chunk = NULL;
        }

        curl_easy_cleanup(curl_handle);
    }

    return chunk;
}

size_t downloadToFile(const char* url, const char* path)
{
    size_t out_write = 0;
    struct MemoryStruct* chunk = downloadToRam(url);

    if (chunk)
    {
        FILE* fp = fopen(path, "wb");

        if (fp)
        {
            out_write = fwrite(chunk->memory, sizeof(uint8_t), chunk->size, fp);
            fclose(fp);
        }

        freeDownload(chunk);
    }

    return out_write;
}

void freeDownload(struct MemoryStruct* chunk)
{
    if (chunk)
    {
        if (chunk->memory)
            free(chunk->memory);
        free(chunk);
    }
}

struct json_object* fetchJSON(const char* url)
{
    struct json_object* json = NULL;
    struct MemoryStruct* chunk = downloadToRam(url);

    if (chunk)
    {
        json = json_tokener_parse((char*)chunk->memory);
        freeDownload(chunk);
    }

    return json;
}
