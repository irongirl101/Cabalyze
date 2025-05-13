#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

struct Memory {
    char *response;
    size_t size;
};

static size_t write_callback(void *data, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    char *ptr = realloc(mem->response, mem->size + total_size + 1);
    if (!ptr) return 0;

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, total_size);
    mem->size += total_size;
    mem->response[mem->size] = 0;

    return total_size;
}

void get_current_weather() {
    const char *api_key = "YOUR_API_KEY";  // Replace this with your actual key
    const char *city = "Bangalore";

    CURL *curl;
    CURLcode res;

    struct Memory chunk = {0};

    char url[512];
    snprintf(url, sizeof(url),
             "https://api.weatherapi.com/v1/current.json?key=%s&q=%s",
             api_key, city);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            cJSON *json = cJSON_Parse(chunk.response);
            if (json) {
                cJSON *location = cJSON_GetObjectItem(json, "location");
                cJSON *current = cJSON_GetObjectItem(json, "current");

                const char *city_name = cJSON_GetObjectItem(location, "name")->valuestring;
                const char *time = cJSON_GetObjectItem(location, "localtime")->valuestring;
                double temp_c = cJSON_GetObjectItem(current, "temp_c")->valuedouble;
                const char *condition = cJSON_GetObjectItem(
                    cJSON_GetObjectItem(current, "condition"), "text")->valuestring;

                printf("Current weather in %s at %s:\n", city_name, time);
                printf("Temperature: %.1f°C\n", temp_c);
                printf("Condition: %s\n", condition);

                cJSON_Delete(json);
            } else {
                fprintf(stderr, "JSON parse error.\n");
            }
        } else {
            fprintf(stderr, "CURL error: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    free(chunk.response);
}

int main() {
    get_current_weather();
    return 0;
}
