#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

struct MemoryStruct {
    char *response;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

void get_weather(const char *location, const char *api_key) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.response = malloc(1);
    chunk.size = 0;

    char url[512];
    snprintf(url, sizeof(url),
             "http://api.weatherapi.com/v1/current.json?key=%s&q=%s",
             api_key, location);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Uncomment for debugging
            // printf("Raw API response: %s\n", chunk.response);

            cJSON *json = cJSON_Parse(chunk.response);
            if (!json) {
                fprintf(stderr, "Failed to parse weather JSON\n");
            } else {
                cJSON *error_obj = cJSON_GetObjectItem(json, "error");
                if (error_obj) {
                    const char *message = cJSON_GetObjectItem(error_obj, "message")->valuestring;
                    fprintf(stderr, "API Error: %s\n", message);
                } else {
                    cJSON *current = cJSON_GetObjectItem(json, "current");
                    if (current) {
                        double temp_c = cJSON_GetObjectItem(current, "temp_c")->valuedouble;
                        const char *condition = cJSON_GetObjectItem(
                            cJSON_GetObjectItem(current, "condition"), "text")->valuestring;
                        printf("Current temperature: %.1f°C\n", temp_c);
                        printf("Condition: %s\n", condition);
                    } else {
                        fprintf(stderr, "No 'current' weather data found\n");
                    }
                }
                cJSON_Delete(json);
            }
        }

        curl_easy_cleanup(curl);
    }

    free(chunk.response);
    curl_global_cleanup();
}

int main() {
    const char *api_key = "YOUR_NEW_API_KEY";  // Replace with your new working API key
    const char *location = "London";
    get_weather(location, api_key);
    return 0;
}
