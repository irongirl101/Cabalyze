#include <stdio.h>
#include<stdlib.h>
#include <string.h> 

#define MAX_LEN 1024 

int main(){

    char provider[] = "Ola"; 
    char AC[] = "Yes"; 
    FILE *fp = fopen("fares.csv", "r"); //opening file 
    if (!fp){//throwing an error if file not found
        perror("File couldnt not be found"); 
        return EXIT_FAILURE; 

    }

    char line[MAX_LEN]; //creating a line 

    fgets(line,sizeof(line),fp); //not using the first line 

    while (fgets(line,sizeof(line),fp))
    {
        line[strcspn(line,"\n")] = '\0';  

        char *token = strtok(line,","); //seperating at delimiter 

        if(strcmp(token, provider)!=0){
            continue; 
        }
        token = strtok(NULL, ","); 

        
        if (!token) continue;
        char *vehicle = token;

        if(strcmp(token,AC)!=0){
            continue;
        }
        token = strtok(NULL,","); 

        //token = strtok(NULL, ",");
        //int base = token ? atoi(token) : 0;

       printf("%s\n", vehicle); 


    }
    fclose(fp); 
    return 0; 

}