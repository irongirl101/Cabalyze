#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#define API_KEY ""

struct memory {
    char *response;
    size_t size;
};

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct memory *mem = (struct memory *)userp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (ptr == NULL) return 0;

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->response[mem->size] = '\0';

    return realsize;
}

// Function to geocode address -> returns coordinates as "lon,lat"
char* geocode_address(CURL *curl, const char *address) {
    struct memory chunk = {NULL, 0};

    char url[1024];
    char *escaped = curl_easy_escape(curl, address, 0);

    snprintf(url, sizeof(url),
             "https://api.openrouteservice.org/geocode/search?api_key=%s&text=%s",
             API_KEY, escaped);

    curl_free(escaped);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Geocoding failed: %s\n", curl_easy_strerror(res));
        free(chunk.response);
        return NULL;
    }

    cJSON *json = cJSON_Parse(chunk.response);
    if (!json) {
        fprintf(stderr, "Failed to parse geocoding JSON\n");
        free(chunk.response);
        return NULL;
    }

    cJSON *features = cJSON_GetObjectItem(json, "features");
    cJSON *first = cJSON_GetArrayItem(features, 0);
    if (!first) {
        fprintf(stderr, "No results found for geocoding\n");
        cJSON_Delete(json);
        free(chunk.response);
        return NULL;
    }

    cJSON *geometry = cJSON_GetObjectItem(first, "geometry");
    cJSON *coordinates = cJSON_GetObjectItem(geometry, "coordinates");

    double lon = cJSON_GetArrayItem(coordinates, 0)->valuedouble;
    double lat = cJSON_GetArrayItem(coordinates, 1)->valuedouble;

    char *coords = malloc(50);
    snprintf(coords, 50, "%.6f,%.6f", lon, lat);

    cJSON_Delete(json);
    free(chunk.response);
    return coords;
}

int main() {
    CURL *curl;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) {
        fprintf(stderr, "Failed to init curl\n");
        return 1;
    }

    char start_address[256], end_address[256];
    printf("Enter start location: ");
    fgets(start_address, sizeof(start_address), stdin);
    start_address[strcspn(start_address, "\n")] = '\0';  // remove newline

    printf("Enter end location: ");
    fgets(end_address, sizeof(end_address), stdin);
    end_address[strcspn(end_address, "\n")] = '\0';

    char *start_coords = geocode_address(curl, start_address);
    char *end_coords = geocode_address(curl, end_address);

    if (!start_coords || !end_coords) {
        fprintf(stderr, "Failed to geocode start or end location.\n");
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }

    // Now use routing API
    struct memory route_chunk = {NULL, 0};

    char url[1024];
    snprintf(url, sizeof(url),
             "https://api.openrouteservice.org/v2/directions/driving-car?api_key=%s&start=%s&end=%s",
             API_KEY, start_coords, end_coords);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&route_chunk);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Route request failed: %s\n", curl_easy_strerror(res));
    } else {
        cJSON *json = cJSON_Parse(route_chunk.response);
        if (json) {
            cJSON *features = cJSON_GetObjectItem(json, "features");
            cJSON *firstFeature = cJSON_GetArrayItem(features, 0);
            cJSON *properties = cJSON_GetObjectItem(firstFeature, "properties");
            cJSON *segments = cJSON_GetObjectItem(properties, "segments");
            cJSON *firstSegment = cJSON_GetArrayItem(segments, 0);
            cJSON *distance = cJSON_GetObjectItem(firstSegment, "distance");
            cJSON *duration = cJSON_GetObjectItem(firstSegment, "duration");

            if (distance && duration) {
                printf("Distance: %.2f km\n", distance->valuedouble*1.7/ 1000.0);
                printf("Duration: %.2f minutes\n", duration->valuedouble*2.3 / 60.0);
            }

            cJSON_Delete(json);
        } else {
            fprintf(stderr, "Failed to parse routing JSON\n");
        }
    }

    free(start_coords);
    free(end_coords);
    free(route_chunk.response);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
