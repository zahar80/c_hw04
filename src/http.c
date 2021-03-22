#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <curl/curl.h>

#include "http.h"

struct buffer_t
{
    char* memory;
    size_t size;
};

static size_t request_callback(void* data, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    struct buffer_t* buffer = (struct buffer_t*)userp;

    char* ptr = realloc(buffer->memory, buffer->size + realsize + 1);
    assert(ptr);
    if(!ptr) {
        return 0;
    }

    buffer->memory = ptr;
    memcpy(&buffer->memory[buffer->size], data, realsize);
    buffer->size += realsize;
    buffer->memory[buffer->size] = 0;

    return realsize;
}

char* do_request(const char* url)
{
    assert(url && *url);
    if (!url) {
        return NULL;
    }

    CURL* curl = curl_easy_init();
    assert(curl);
    if(!curl) {
        return NULL;
    }

    struct buffer_t buffer = { NULL, 0 };

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, request_callback);

    CURLcode curl_result = curl_easy_perform(curl);
    assert(curl_result == CURLE_OK);
    if(curl_result != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        return NULL;
    }

    curl_easy_cleanup(curl);

    return buffer.memory;
}
