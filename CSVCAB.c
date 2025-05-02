#include <stdio.h>
#include<stdlib.h>
#include <string.h> 

#define MAX_LEN 1024 

int main(){
    FILE *fp = fopen("fare_parameters.csv", "r"); //opening file 
    if (!fp){//throwing an error if file not found
        perror("File couldnt not be found"); 
        return EXIT_FAILURE; 

    }

    char line[MAX_LEN]; //creating a line 

    fgets(line,sizeof(line),fp); //not using the first line 

}