#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX_COL 10 
#define MAX_ROW 1024 
#define MAX_LEN 1024

int main(){
    time_t current_time; 
    struct tm * local_time; 
    
    char h[3]; 
    char m[3]; 


    current_time = time(NULL);
    local_time = localtime(&current_time); 
    int current = local_time->tm_hour; 

   
    strftime(h,sizeof(h), "%H", local_time); //formatting 
    
    /*//strftime(m,sizeof(m), "%M", local_time); 


    //printf("%s \n", h); 
    //printf("%s \n", m); */

    FILE *fp = fopen("times.csv", "r"); 
    if(!fp){
        printf("file not found"); 
        return EXIT_FAILURE; 
    }

    char line[100];
    char hour_str[3];  // To store just the hour part like "00", "01", ..., "13"
    
     while (fgets(line, sizeof(line), fp)) {
        // Remove newline character if present
        line[strcspn(line, "\n")] = '\0';

        // Extract the hour part from line
        strncpy(hour_str, line, 2);
        hour_str[2] = '\0';  // Null-terminate

        int csv_hour = atoi(hour_str); // Convert string to int

        if (csv_hour == current) {
            printf("Match found in CSV: %s\n", line);
            break;
        }
    }

    fclose(fp);

    return 0; 
}