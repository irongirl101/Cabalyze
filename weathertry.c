#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h> 

#define MAX_COL_W 3 
#define MAX_ROW_W 100
#define MAX_LEN_W 1024 

struct MemoryStruct {
    char *memory;
    size_t size;
};


static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Function to get latitude and longitude from address
char* geocode_address(CURL *curl, const char *address, const char *API_KEY) {
    struct MemoryStruct chunk = {NULL, 0};

    char url[1024];
    char *escaped = curl_easy_escape(curl, address, 0);

    snprintf(url, sizeof(url),
             "https://api.openrouteservice.org/geocode/search?api_key=%s&text=%s",
             API_KEY, escaped);

    curl_free(escaped);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    free(chunk.memory);
    chunk.memory = NULL;
    chunk.size = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Geocoding failed: %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        return NULL;
    }

    cJSON *json = cJSON_Parse(chunk.memory);
    if (!json) {
        fprintf(stderr, "Failed to parse geocoding JSON\n");
        free(chunk.memory);
        return NULL;
    }

    cJSON *features = cJSON_GetObjectItem(json, "features");
    cJSON *first = cJSON_GetArrayItem(features, 0);
    if (!first) {
        fprintf(stderr, "No results found for geocoding\n");
        cJSON_Delete(json);
        free(chunk.memory);
        return NULL;
    }

    cJSON *geometry = cJSON_GetObjectItem(first, "geometry");
    cJSON *coordinates = cJSON_GetObjectItem(geometry, "coordinates");

    double lon = cJSON_GetArrayItem(coordinates, 0)->valuedouble;
    double lat = cJSON_GetArrayItem(coordinates, 1)->valuedouble;

    char *coords = malloc(50);
    snprintf(coords, 50, "%.6f,%.6f", lon, lat);

    cJSON_Delete(json);
    free(chunk.memory);
    return coords;
}

// Function to get current weather condition from WeatherAPI
int get_current_weather(const char *api_key, double lat, double lon, char *condition_text, size_t condition_size) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;

    // Build URL for WeatherAPI current weather
    char url[512];
    snprintf(url, sizeof(url),
        "http://api.weatherapi.com/v1/current.json?key=%s&q=%f,%f",
        api_key, lat, lon);

    curl = curl_easy_init();
    if(!curl) {
        fprintf(stderr, "Failed to init curl\n");
        free(chunk.memory);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return 0;
    }

    cJSON *root = cJSON_Parse(chunk.memory);
    free(chunk.memory);
    curl_easy_cleanup(curl);

    if(!root) {
        fprintf(stderr, "JSON parse error\n");
        return 0;
    }

    cJSON *current = cJSON_GetObjectItem(root, "current");
    if(!current) {
        cJSON_Delete(root);
        return 0;
    }

    cJSON *condition = cJSON_GetObjectItem(current, "condition");
    if(!condition) {
        cJSON_Delete(root);
        return 0;
    }

    cJSON *text = cJSON_GetObjectItem(condition, "text");
    if(text && cJSON_IsString(text)) {
        strncpy(condition_text, text->valuestring, condition_size - 1);
        condition_text[condition_size - 1] = '\0';  // ensure null terminated
        cJSON_Delete(root);
        return 1;
    }

    cJSON_Delete(root);
    return 0;
}

float surge() {
    
    const char *ors_api_key = "";
    const char *weather_api_key = "";

    const char *address="100 Feet Ring Road, Banashankari Stage III, Dwaraka Nagar, Banashankari, Bengaluru, Karnataka 560085";

    double lat, lon;
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return 1;
    }
    char *coords = geocode_address(curl, address, ors_api_key);
    curl_easy_cleanup(curl);
    if (!coords) {
        fprintf(stderr, "Failed to geocode address\n");
        return 1;
    }
    sscanf(coords, "%lf,%lf", &lon, &lat);
    free(coords);
    printf("Geocoded: %s -> Latitude: %f, Longitude: %f\n", address, lat, lon);

    char condition[128];
    if(!get_current_weather(weather_api_key, lon, lat, condition, sizeof(condition))) {
        fprintf(stderr, "Failed to get weather\n");
        return 1;
    }

    printf("Current weather condition: %s\n", condition);

    FILE *fp = fopen("weathers.csv", "r"); 
    if(!fp){
        perror("File not found"); 
        return EXIT_FAILURE; 
    } 
    char line[MAX_LEN_W]; //will be used to tokenize values from 
    char *table[MAX_ROW_W][MAX_COL_W]; 
    memset(table, 0, sizeof(table));//essentially clearing up a space in memory(maybe containing junk values), where all the value will be allocating to. as a starter, everything is kept as 0, and pointeres are made null. 
    int row = 0;
    

    while (fgets(line,sizeof(line),fp) && row<MAX_ROW_W){
        line[strcspn(line,"\n")] = '\0'; //ending each line with a line terminator than a newline 
        int col = 0; 
        char *tok = strtok(line,","); //delimiter ,; tokenizing each line ->this one splits the string 

        while (tok && col<MAX_COL_W)
        {
            table[row][col] = strdup(tok); //creating memeory and store 

            if (table[row][col] == NULL) {
                perror("Memory allocation failed for strdup");
                fclose(fp);
                return EXIT_FAILURE;
            }
            col++; 
            tok = strtok(NULL,","); //-> gets the next token why null? no clude, take it as syntax 
        }
        row++; 
    }

    fclose(fp); 
    float surge; 
    for(int i = 1; i<row; i++){
        //printf("%s \n", table[i][0]); 
        if(strcasecmp(table[i][0],condition)==0){
            //printf("%s \n", table[i][0]); 
            surge = atof(table[i][1]); 
        }

    }
    return surge; 
    //printf("%0.2f\n", surge); 

    return 0;
}
