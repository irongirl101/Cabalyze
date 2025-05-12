#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>//used to parse the json data 

#define API_KEY "5b3ce3597851110001cf6248f91e18c396fa4c6c833546932b975c1e"

struct memory {
    char *response;// pointer to the response data from the json data as a string 
    size_t size; // the size of the json data in bytes 
};


// Callback to write data to memory
// size_t is a datatype used to take in size of memory 
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {//content: the recieved data ; size and nmemb for the size to be allocated in memory where size->size of each unit, and nmemb is the number of units ; userp is a pointer to a user made structure, in this case memory
//why size_t write_callback ... -> this shows that these are the number of bytes the function is to return back. (this is part of syntax of libcurl)
    size_t realsize = size * nmemb; //calculation of size of data 
    struct memory *mem = (struct memory *)userp;//creating a pointer called userp towards the struct 

    char *ptr = realloc(mem->response, mem->size + realsize + 1);// resizes the memory size to the response size, +1 is there in order to account for whitespace character
    if (ptr == NULL) {//in case memory allocation fails 
        // Out of memory
        return 0;
    }

    mem->response = ptr;// assigns newly resized memory from realloc to  
    memcpy(&(mem->response[mem->size]), contents, realsize);// copies the data into the reallocated memory size  -> start copying and end of current data 
    mem->size += realsize;  //increase the size each time 
    mem->response[mem->size] = '\0'; // add a null terminator at the end 

    return realsize; // sending back the size 
}

// used to change words to coordinates 
char* geocode_address(CURL *curl, const char *address) {
    struct memory chunk = {NULL, 0}; //creates a chunk in memory for this address 

    char url[1024]; //creating a character array to store the url 
    char *escaped = curl_easy_escape(curl, address, 0); //converts words and special characters into url readable format

    // api call for geocoding 
    snprintf(url, sizeof(url),
             "https://api.openrouteservice.org/geocode/search?api_key=%s&text=%s", 
             API_KEY, escaped);

    curl_free(escaped);//frees the memory used for escaped string  

    curl_easy_setopt(curl, CURLOPT_URL, url); //sets up curl session with url , callback function and memory 
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);// performs https request 
    if (res != CURLE_OK) { // error hamdling 
        fprintf(stderr, "Geocoding failed: %s\n", curl_easy_strerror(res));
        free(chunk.response);
        return NULL;
    }

    cJSON *json = cJSON_Parse(chunk.response); // passes JSON string from api 
    if (!json) { //error handling for geocoding 
        fprintf(stderr, "Failed to parse geocoding JSON\n");
        free(chunk.response);
        return NULL;
    }

    cJSON *features = cJSON_GetObjectItem(json, "features"); //checks the data  
    cJSON *first = cJSON_GetArrayItem(features, 0); //gets first result 
    if (!first) { //error handling 
        fprintf(stderr, "No results found for geocoding\n");
        cJSON_Delete(json);
        free(chunk.response);
        return NULL;
    }

    cJSON *geometry = cJSON_GetObjectItem(first, "geometry"); // checks for geometry 
    cJSON *coordinates = cJSON_GetObjectItem(geometry, "coordinates"); //checks for coordinates 

    //extracts longitude and latitude values 
    double lon = cJSON_GetArrayItem(coordinates, 0)->valuedouble; 
    double lat = cJSON_GetArrayItem(coordinates, 1)->valuedouble;
    //actual coordinate values 
    char *coords = malloc(50);
    snprintf(coords, 50, "%.6f,%.6f", lon, lat);

    cJSON_Delete(json);
    free(chunk.response);
    return coords;
}

int main() {
    // initializing useing libcurl -> standard 
    CURL *curl;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) {// error handling 
        fprintf(stderr, "Failed to init curl\n");
        return 1;
    }

    //asks for start and end location, while removing the newline character
    char start_address[256], end_address[256];
    printf("Enter start location: ");
    fgets(start_address, sizeof(start_address), stdin);
    start_address[strcspn(start_address, "\n")] = '\0';  // remove newline

    printf("Enter end location: ");
    fgets(end_address, sizeof(end_address), stdin);
    end_address[strcspn(end_address, "\n")] = '\0';

    //geocodes the start and end using geocode function 
    char *start_coords = geocode_address(curl, start_address);
    char *end_coords = geocode_address(curl, end_address);

    //error handling 
    if (!start_coords || !end_coords) {
        fprintf(stderr, "Failed to geocode start or end location.\n");
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }

    //creating a new space in memory for the routing 
    struct memory route_chunk = {NULL, 0};

    //building api url using the newly found start and end coords 
    char url[1024];
    snprintf(url, sizeof(url),
             "https://api.openrouteservice.org/v2/directions/driving-car?api_key=%s&start=%s&end=%s",
             API_KEY, start_coords, end_coords);
    //curl request for output -> standard 
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&route_chunk);
    //performs the requests 
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) { //error handling 
        fprintf(stderr, "Route request failed: %s\n", curl_easy_strerror(res));
    } else {
        cJSON *json = cJSON_Parse(route_chunk.response);
        if (json) {
            //gets features[0] = first feature; first feature[0] = properties; properties[0] = segments; segement[0] -> distance and duration 
            cJSON *features = cJSON_GetObjectItem(json, "features");
            cJSON *firstFeature = cJSON_GetArrayItem(features, 0);
            cJSON *properties = cJSON_GetObjectItem(firstFeature, "properties");
            cJSON *segments = cJSON_GetObjectItem(properties, "segments");
            cJSON *firstSegment = cJSON_GetArrayItem(segments, 0);
            cJSON *distance = cJSON_GetObjectItem(firstSegment, "distance");
            cJSON *duration = cJSON_GetObjectItem(firstSegment, "duration");

            if (distance && duration) {
                printf("Distance: %.2f km\n", distance->valuedouble*1.7/1000.0 ); // getting distance in km, 1.7 -> for displacement discrepancy 
                printf("Duration: %.2f\n", duration->valuedouble*2.3/60); // getting duration in min, 2.3 -> time discrepancy due to displacement > distance 
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