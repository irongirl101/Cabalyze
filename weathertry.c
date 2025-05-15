#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#define BUFFER_SIZE 100000
#define ORS_API_KEY "5b3ce3597851110001cf6248f91e18c396fa4c6c833546932b975c1e"
#define WEATHER_API_KEY "f4f88ef718484d7a89e43834251405"

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    strcat((char *)userp, (char *)contents);
    return size * nmemb;
}

// Step 1: Geocode using OpenRouteService
int geocode_with_ors(const char *address, char *lon, char *lat) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    char url[512];
    snprintf(url, sizeof(url),
             "https://api.openrouteservice.org/geocode/search?api_key=%s&text=%s",
             ORS_API_KEY, address);

    char response[BUFFER_SIZE] = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return 0;

    cJSON *root = cJSON_Parse(response);
    if (!root) return 0;

    cJSON *features = cJSON_GetObjectItem(root, "features");
    if (!cJSON_IsArray(features) || cJSON_GetArraySize(features) == 0) {
        cJSON_Delete(root);
        return 0;
    }

    cJSON *geometry = cJSON_GetObjectItem(
        cJSON_GetObjectItem(cJSON_GetArrayItem(features, 0), "geometry"), "coordinates");

    // ORS returns [lon, lat]
    snprintf(lat, 32, "%.6f", cJSON_GetArrayItem(geometry, 0)->valuedouble);
    snprintf(lon, 32, "%.6f", cJSON_GetArrayItem(geometry, 1)->valuedouble);

    cJSON_Delete(root);
    return 1;
}

// Step 2: Fetch WeatherAPI forecast with lat/lon
void fetch_weatherapi_forecast(const char *lat, const char *lon) {
    
    CURL *curl = curl_easy_init();
    if (!curl) return;

    char url[512];
    snprintf(url, sizeof(url),
             "http://api.weatherapi.com/v1/forecast.json?key=%s&q=%s,%s&days=1&aqi=no&alerts=no",
             WEATHER_API_KEY, lat, lon);
   
    char response[BUFFER_SIZE] = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to fetch weather data\n");
        return;
    }
    printf("RAW RESPONSE:\n%s\n", response);

        cJSON *root = cJSON_Parse(response);
    if (!root) {
        printf("Error parsing JSON\n");
        return;
    }

    // Extract the hourly data array
    cJSON *hourly = cJSON_GetObjectItemCaseSensitive(root, "hourly");
    if (!cJSON_IsArray(hourly)) {
        printf("Hourly data is not an array\n");
        cJSON_Delete(root);
        return;
    }

    // Loop through the array of hourly data
    int hourly_count = cJSON_GetArraySize(hourly);
    for (int i = 0; i < hourly_count; i++) {
        cJSON *hour_data = cJSON_GetArrayItem(hourly, i);

        // Extract individual fields
        cJSON *time = cJSON_GetObjectItemCaseSensitive(hour_data, "time");
        cJSON *temp_c = cJSON_GetObjectItemCaseSensitive(hour_data, "temp_c");
        cJSON *condition = cJSON_GetObjectItemCaseSensitive(hour_data, "condition");
        cJSON *text = cJSON_GetObjectItemCaseSensitive(condition, "text");
        cJSON *wind_mph = cJSON_GetObjectItemCaseSensitive(hour_data, "wind_mph");

        // Check if fields exist and print them
        if (cJSON_IsString(time) && cJSON_IsNumber(temp_c) && cJSON_IsString(text) && cJSON_IsNumber(wind_mph)) {
            printf("Time: %s, Temp: %.1f°C, Condition: %s, Wind Speed: %.1f mph\n",
                   time->valuestring, temp_c->valuedouble, text->valuestring, wind_mph->valuedouble);
        } else {
            printf("Error extracting data for hour %d\n", i);
        }
    }

    // Clean up
    cJSON_Delete(root);

}

int main() {
    char address[256];
    char lat[32], lon[32];

    printf("Enter location: ");
    fgets(address, sizeof(address), stdin);
    address[strcspn(address, "\n")] = 0; // remove newline

    if (!geocode_with_ors(address, lon, lat)) {
        fprintf(stderr, "Geocoding failed.\n");
        return 1;
    }

    printf("Latitude: %s | Longitude: %s\n", lon, lat);
    fetch_weatherapi_forecast(lon, lat);

    return 0;
}
