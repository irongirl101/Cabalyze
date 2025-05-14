#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#define WEATHER_API_KEY "1aabc40bf90d4a73a9295105251305"
#define GEOCODING_API_KEY "5b3ce3597851110001cf6248f91e18c396fa4c6c833546932b975c1e"

// Structure to hold the response from the API call
struct memory {
    char *response;
    size_t size;
};

// Callback function for storing the response from the API call
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct memory *mem = (struct memory *)userp;
    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (!ptr) return 0;
    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;
    return realsize;
}

// Geocoding function to get latitude and longitude from an address
char* geocode_address(CURL *curl, const char *address) {
    struct memory chunk = {NULL, 0};
    
    char url[1024];
    char *escaped = curl_easy_escape(curl, address, 0);  // URL encode the address
    
    // Geocoding API request
    snprintf(url, sizeof(url), 
             "https://api.openrouteservice.org/geocode/search?api_key=%s&text=%s", 
             GEOCODING_API_KEY, escaped);
    
    curl_free(escaped);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    
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

// Fetch hourly weather data based on latitude and longitude
void fetch_hourly_weather(const char *lat, const char *lon) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "CURL init failed\n");
        return;
    }

    char url[512];
    snprintf(url, sizeof(url),
        "https://api.weatherapi.com/v1/forecast.json?key=%s&q=%s,%s&hours=24", 
        WEATHER_API_KEY, lat, lon);

    struct memory chunk = {malloc(1), 0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

    if (curl_easy_perform(curl) != CURLE_OK) {
        fprintf(stderr, "API request failed\n");
        curl_easy_cleanup(curl);
        free(chunk.response);
        return;
    }

    cJSON *json = cJSON_Parse(chunk.response);
    if (!json) {
        fprintf(stderr, "Failed to parse JSON\n");
        curl_easy_cleanup(curl);
        free(chunk.response);
        return;
    }

    cJSON *forecast = cJSON_GetObjectItem(json, "forecast");
    cJSON *hourly = cJSON_GetObjectItem(forecast, "forecastday");
    cJSON *hours = cJSON_GetArrayItem(hourly, 0); // Get the first forecast day
    cJSON *hour_array = cJSON_GetObjectItem(hours, "hour");

    if (!hour_array || !cJSON_IsArray(hour_array)) {
        fprintf(stderr, "No hourly data found\n");
    } else {
        int n = cJSON_GetArraySize(hour_array);
        for (int i = 0; i < n; ++i) {
            cJSON *hour = cJSON_GetArrayItem(hour_array, i);
            double temp = cJSON_GetObjectItem(hour, "temp_c")->valuedouble;
            const char *time = cJSON_GetObjectItem(hour, "time")->valuestring;
            const char *condition = cJSON_GetObjectItem(cJSON_GetObjectItem(hour, "condition"), "text")->valuestring;
            printf("Hour %d: %s -> %.1f°C, %s\n", i, time, temp, condition);
        }
    }

    cJSON_Delete(json);
    free(chunk.response);
    curl_easy_cleanup(curl);
}

int main() {
    const char *address = "Bangalore"; // Replace with any address

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "CURL initialization failed!\n");
        return 1;
    }

    // Step 1: Get the coordinates for the address
    char *coords = geocode_address(curl, address);
    if (coords == NULL) {
        curl_easy_cleanup(curl);
        return 1;
    }

    // Step 2: Get the weather for the location (latitude, longitude)
    char *longitude = strtok(coords, ",");
    char *latitude = strtok(NULL, ",");
    fetch_hourly_weather(latitude, longitude);
     

    // Clean up
    free(coords);
    curl_easy_cleanup(curl);
    return 0;
}
