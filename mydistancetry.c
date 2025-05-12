#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#define api_key "5b3ce3597851110001cf6248f91e18c396fa4c6c833546932b975c1e"

struct memory{
    char *responses; 
    size_t size; 
}; 

//calback function to write content to memory 
size_t write_l(void *contents, size_t nmemb, size_t size, void *userp){
    size_t realsize = size * nmemb; 
    struct memory *mem = (struct memory*)userp; 

    char *ptr = realloc(mem->responses, mem->size+size+1); 

    if(ptr == NULL){
        return 0; 
    }

    mem->responses = ptr; 
    memcpy(&(mem->responses[mem->size]), contents, realsize); 
    mem->size+= realsize; 
    mem->responses[mem->size] = '\0'; 

    return realsize; 
}

