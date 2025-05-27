//trying to create a 2D array, in order to easily index it, makes life easier, 
//howver in C, we cannot have heterogenous values, therefore, tokenizing everything as a string, and then changing to int or float using atoi and atof when needed 

//NOTE TO SELF: stricmp RETURNS A 0 WHEN TRUE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#define API_KEY "5b3ce3597851110001cf6248f91e18c396fa4c6c833546932b975c1e"

#define MAX_COL 10 
#define MAX_ROW 100
#define MAX_LEN 1024 

#define MAX_COL_W 3 
#define MAX_ROW_W 100
#define MAX_LEN_W 1024 
               
struct MemoryStruct {
    char *memory;
    size_t size;
};

struct memory {
    char *response;
    size_t size;
};

typedef struct {
    double distance;
    double duration;
    float surge;
} RouteInfo;

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

int get_current_weather(const char *api_key, double lat, double lon, char *condition_text, size_t condition_size);
float surge(CURL *curl, const char *address);

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
    free(chunk.response);
    chunk.response = NULL;
    chunk.size = 0;
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

int dist_dur(RouteInfo *info) {
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
            if (!cJSON_IsArray(features)) {
                fprintf(stderr, "Expected 'features' to be an array\n");
                cJSON_Delete(json);
                return 1;
            }

            cJSON *firstFeature = cJSON_GetArrayItem(features, 0);
            if (!firstFeature) {
                fprintf(stderr, "No route features found\n");
                cJSON_Delete(json);
                return 1;
            }

            cJSON *properties = cJSON_GetObjectItem(firstFeature, "properties");
            if (!properties) {
                fprintf(stderr, "No 'properties' in feature\n");
                cJSON_Delete(json);
                return 1;
            }

            cJSON *segments = cJSON_GetObjectItem(properties, "segments");
            if (!cJSON_IsArray(segments)) {
                fprintf(stderr, "Expected 'segments' to be an array\n");
                cJSON_Delete(json);
                return 1;
            }

            cJSON *firstSegment = cJSON_GetArrayItem(segments, 0);
            if (!firstSegment) {
                fprintf(stderr, "No segments found\n");
                cJSON_Delete(json);
                return 1;
            }

            cJSON *distance = cJSON_GetObjectItem(firstSegment, "distance");
            cJSON *duration = cJSON_GetObjectItem(firstSegment, "duration");

            if (!distance || !duration) {
                fprintf(stderr, "Distance or duration missing in segment\n");
                cJSON_Delete(json);
                return 1;
            }


            if (distance && duration) {
                //float dist = distance->valuedouble*1.5/ 1000.0; 
                //float dur = duration->valuedouble*2.3/ 60.0; 
                info->distance = distance->valuedouble *1.5/ 1000.0;
                info->duration = duration->valuedouble * 2.5 / 60.0;
                //printf("Distance: %.2f km\n", distance->valuedouble*1.5/1000);/*1.5/ 1000.0);*/
                //printf("Duration: %.2f minutes\n", duration->valuedouble*2.5/60.0);  //*2.3 
            }

            cJSON_Delete(json);
        } else {
            fprintf(stderr, "Failed to parse routing JSON\n");
        }
    }

    float surge_start = surge(curl, start_address);
    float surge_end   = surge(curl, end_address);
    float avg_surge   = (surge_start + surge_end) / 2.0f;
    info->surge = avg_surge;

    free(start_coords);
    free(end_coords);
    free(route_chunk.response);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}


float surge(CURL *curl, const char *address) {
    const char *weather_api_key = "f4f88ef718484d7a89e43834251405";
    // 1. Geocode
    char *coords = geocode_address(curl, address);
    if (!coords) {
        fprintf(stderr, "Failed to geocode '%s'\n", address);
        return 1.0f;
    }
    double lon, lat;
    sscanf(coords, "%lf,%lf", &lon, &lat);
    free(coords);

    // 2. Get weather condition text
    char condition[128] = {0};
    if (!get_current_weather(weather_api_key, lat, lon, condition, sizeof(condition))) {
        fprintf(stderr, "Failed to get weather for '%s'\n", address);
        return 1.0f;
    }
    
    FILE *fp = fopen("weathers.csv", "r");
    if (!fp) {
        perror("Cannot open weathers.csv");
        return 1.0f;
    }

    char line[MAX_LEN_W];
    float surge_value = 1.0f;            // default if not found
    while (fgets(line, sizeof(line), fp)) {
        // strip newline
        line[strcspn(line, "\r\n")] = '\0';
        char *csv_cond = strtok(line, ",");
        char *csv_surge = strtok(NULL, ",");
        if (csv_cond && csv_surge && strcasecmp(csv_cond, condition) == 0) {
            surge_value = atof(csv_surge);
            break;
        }
    }
    fclose(fp);
    return surge_value;
}



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


int main(){
    printf("\n");
    printf("\n"); 
    printf("\n"); 
printf("  /$$$$$$            /$$                 /$$                              \n"
" /$$__  $$          | $$                | $$                              \n"
"| $$  \\__/  /$$$$$$ | $$$$$$$   /$$$$$$ | $$ /$$   /$$ /$$$$$$$$  /$$$$$$ \n"
"| $$       |____  $$| $$__  $$ |____  $$| $$| $$  | $$|____ /$$/ /$$__  $$\n"
"| $$        /$$$$$$$| $$  \\ $$  /$$$$$$$| $$| $$  | $$   /$$$$/ | $$$$$$$$\n"
"| $$    $$ /$$__  $$| $$  | $$ /$$__  $$| $$| $$  | $$  /$$__/  | $$_____/\n"
"|  $$$$$$/|  $$$$$$$| $$$$$$$/|  $$$$$$$| $$|  $$$$$$$ /$$$$$$$$|  $$$$$$$\n"
" \\______/  \\_______/|_______/  \\_______/|__/ \\____  $$|________/ \\_______/\n"
"                                            /$$  | $$                    \n"
"                                           |  $$$$$$/                    \n"
"                                            \\______/                     \n");
printf("\n"); 

    RouteInfo route;
    int result = dist_dur(&route);
    if (result != 0) {
        fprintf(stderr, "Failed to get distance and duration.\n");
        return 1;
    }

    //printf("Distance: %.2f km\n", route.distance);
    //printf("Duration: %.2f min\n", route.duration);
    //printf("Surge: %0.2f\n", route.surge); 
    
   /*//char provider_choice[10]; //considering the user wants to primarily look at uber/ola/namma yatri 
    char provider[100]; 
    //need to ask user for whether they have a choice or not, number of preferences 
    int provider_choice; 

    printf("Do you have any preference towards provider? If yes, enter the number of providers, else enter 0.\n"); 
    scanf("%d",&provider_choice); */ 
    char provider[100]; //have to figure out a way of getting preferences-> for the time being, take one at a time 
    printf("Enter preference if any(Uber,Ola,Namma Yatri), else type No. \n"); 
   //printf("If you want to comapre against 2 select ones, please enter the one you do not want to include with a minus(-Uber,-Ola,-Namma Yatri)\n");
    //scanf("%s", provider); -> does not work, as namma aytri has a whitespace, and scanf reads till the first whitespace is found 
    //therefore use fgets 
   /* while(getchar()!='\n' && getchar() != EOF); 
    fgets(provider, sizeof(provider), stdin);//stdin helps in accepting phrases with spaves in between 
    provider[strcspn(provider, "\n")] = '\0'; */
    fgets(provider, sizeof(provider), stdin);
    provider[strcspn(provider, "\n")] = '\0'; // remove newline
    //what to do in order to get 2 at a time? -> ask if user wants to compare bw 2, only check one, or compare all 
    //if 2, ask for 2 providers, and give the other. 

    if (
        strcasecmp(provider, "Uber") != 0 &&
        strcasecmp(provider, "Ola") != 0 &&
        strcasecmp(provider, "Namma Yatri") != 0 &&
        strcasecmp(provider, "No") != 0 

    ) {
        printf("Please enter an acceptable value (Uber, Ola, Namma Yatri, No, Yes)\n"); 
        return 1;
    }

    else{
    //printf("%s", provider); 
    
    //provider can have multiple choices, one preference, 2 preferences, or no preferences. so we could have provider choice, and then ask which providers 
    int passengers; 
    printf("How many passengers are travelling?\n"); 
    scanf("%d",&passengers); 

    char ac_choice[100];
    printf("AC? (Yes/No) \n"); 
    scanf("%s", ac_choice); 
    char premimum_choice[100];
    printf("Premium? (Yes/No)(Please choose Yes if AC is Yes) \n"); 
    scanf("%s", premimum_choice); 

    //float surge_value = surge(); // for the time being 
    float distance = route.distance ; // in km, for the time being 
    float time = route.duration; //will be in minutes, for the time being 
    float surge_value = route.surge; 

    //opening a file
    FILE *fp = fopen("fares.csv", "r"); 
    if(!fp){
        perror("File not found"); 
        return EXIT_FAILURE; 
    } 
    char line[MAX_LEN]; //will be used to tokenize values from 
    char *table[MAX_ROW][MAX_COL]; 
    memset(table, 0, sizeof(table));//essentially clearing up a space in memory(maybe containing junk values), where all the value will be allocating to. as a starter, everything is kept as 0, and pointeres are made null. 
    int row = 0;
    

    while (fgets(line,sizeof(line),fp) && row<MAX_ROW){
        line[strcspn(line,"\n")] = '\0'; //ending each line with a line terminator than a newline 
        int col = 0; 
        char *tok = strtok(line,","); //delimiter ,; tokenizing each line ->this one splits the string 

        while (tok && col<MAX_COL)
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
    /*for (int i = 0; i < row; i++) {
        for (int j = 0; j < MAX_COL && table[i][j]; j++) {
            printf("%s\t", table[i][j]);
        }
        printf("\n");
    }*/ 

   //my info {0:Provider,1:Vehicle Type,2:AC,3:Premium,4:Passengers,5:Base Fare (₹),6:Per Km (₹),7:Per Min (₹),8:Booking Fee (₹)}

    
    float price_flag=0; 
    char type_flag[100] = ""; 
    int passengers_flag; 
    //char provider_flag[] = provider; 
    char provider_flag[100] = ""; 


   for(int i = 1; i<row; i++){
        if(strcasecmp(provider, "No")!=0){
            if(strcasecmp(table[i][0],provider)==0 && atoi(table[i][4])>=passengers && strcasecmp(table[i][2], ac_choice)==0 && strcasecmp(table[i][3],premimum_choice)==0){
                float total = atoi(table[i][5]) + (distance*atoi(table[i][6])) + (time*atoi(table[i][7])) + atoi(table[i][8]) ; 
                float final = total * surge_value; 
                printf( "Type: %s | Number of Passengers: %s | Estimated Price:%.2f\t", table[i][1],  table[i][4], final); 
                if (price_flag==0)
                {
                    price_flag = final; 
                    strcpy(type_flag, table[i][1]); 
                    passengers_flag = atoi(table[i][4]); 
                    strcpy(provider_flag, provider); 
                }
                else if(price_flag>final){
                    price_flag = final; 
                    strcpy(type_flag, table[i][1]); 
                    passengers_flag = atoi(table[i][4]);
                    strcpy(provider_flag, provider);
                }
                
                //for(int j =0; j<MAX_COL && table[i][j]; j++){
                    //printf("%s\t", table[i][j]); 
                //}
                printf("\n"); 
            }

        }
        else if(atoi(table[i][4])>=passengers && strcasecmp(table[i][2], ac_choice)==0 && strcasecmp(table[i][3],premimum_choice)==0){
            float total = atoi(table[i][5]) + (distance*atoi(table[i][6])) + (time*atoi(table[i][7])) + atoi(table[i][8]) ; 
                float final = total * surge_value; 
                printf("Provider: %s | Type: %s | Number of Passengers: %s | Price: %.2f\t", table[i][0], table[i][1], table[i][4], final); 
                if (price_flag==0)
                {
                    price_flag = final; 
                    strcpy(type_flag, table[i][1]); 
                    passengers_flag = atoi(table[i][4]); 
                    strcpy(provider_flag, table[i][0]);
                }
                else if(price_flag>final){
                    price_flag = final; 
                    strcpy(type_flag, table[i][1]); 
                    passengers_flag = atoi(table[i][4]);
                    strcpy(provider_flag, table[i][0]);
                }

                //for(int j =0; j<MAX_COL && table[i][j]; j++){
                    //printf("%s\t", table[i][j]); 
                //}
                printf("\n");
        }
        

   }
    if (price_flag==0.00)
    {
        printf("Please enter proper values.\n"); 
        
    }
    else{
    printf("The best value: \n"); 
    printf("Provider: %s | Type: %s | Number of Passengers: %d | Price: %.2f\t \n", provider_flag, type_flag, passengers_flag, price_flag); 
    }
    // free allocated memory
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < MAX_COL && table[i][j]; j++) {
            free(table[i][j]);
        }
    }
    }
    
    return 0; 
}