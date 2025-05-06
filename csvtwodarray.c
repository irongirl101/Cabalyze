//trying to create a 2D array, in order to easily index it, makes life easier, 
//howver in C, we cannot have heterogenous values, therefore, tokenizing everything as a string, and then changing to int or float using atoi and atof when needed 

//NOTE TO SELF: strcmp RETURNS A 0 WHEN TRUE 
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 

#define MAX_COL 10 
#define MAX_ROW 100
#define MAX_LEN 1024 

int main(){
   /*//char provider_choice[10]; //considering the user wants to primarily look at uber/ola/namma yatri 
    char provider[100]; 
    //need to ask user for whether they have a choice or not, number of preferences 
    int provider_choice; 

    printf("Do you have any preference towards provider? If yes, enter the number of providers, else enter 0.\n"); 
    scanf("%d",&provider_choice); */ 
    char provider[100]; //have to figure out a way of getting preferences-> for the time being, take one at a time 
    printf("Enter preference if any, else type No \n"); 
    //scanf("%s", provider); -> does not work, as namma aytri has a whitespace, and scanff reads till the first whitespace is found 
    //therefore use fgets 
   /* while(getchar()!='\n' && getchar() != EOF); 
    fgets(provider, sizeof(provider), stdin);//stdin helps in accepting phrases with spaves in between 
    provider[strcspn(provider, "\n")] = '\0'; */
    fgets(provider, sizeof(provider), stdin);
    provider[strcspn(provider, "\n")] = '\0'; // remove newline

    if (
        strcmp(provider, "Uber") != 0 &&
        strcmp(provider, "Ola") != 0 &&
        strcmp(provider, "Namma Yatri") != 0 &&
        strcmp(provider, "No") != 0 &&
        strcmp(provider, "no") != 0
    ) {
        printf("Please enter an acceptable value (Uber, Ola, Namma Yatri, No)\n"); 
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
    printf("Premium? (Yes/No) \n"); 
    scanf("%s", premimum_choice); 

    float surge = 0.75; // for the time being 
    float distance = 18 ; // in km, for the time being 
    float time = 40; //will be in minutes, for the time being 

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

   for(int i = 1; i<row; i++){
        if(strcmp(provider, "No")!=0){
            if(strcmp(table[i][0],provider)==0 && atoi(table[i][4])>=passengers && strcmp(table[i][2], ac_choice)==0 && strcmp(table[i][3],premimum_choice)==0){
                float total = atoi(table[i][5]) + (distance*atoi(table[i][6])) + (time*atoi(table[i][7])) + atoi(table[i][8]) ; 
                float final = total * surge; 
                printf( "Type: %s | Number of Passengers: %s | Estimated Price:%.2f\t", table[i][1],  table[i][4], final); 
                //for(int j =0; j<MAX_COL && table[i][j]; j++){
                    //printf("%s\t", table[i][j]); 
                //}
                printf("\n"); 
            }
        }
        else if(atoi(table[i][4])>=passengers && strcmp(table[i][2], ac_choice)==0 && strcmp(table[i][3],premimum_choice)==0){
            float total = atoi(table[i][5]) + (distance*atoi(table[i][6])) + (time*atoi(table[i][7])) + atoi(table[i][8]) ; 
                float final = total * surge; 
                printf("Provider: %s | Type: %s | Number of Passengers: %s | Price: %.2f\t", table[i][0], table[i][1], table[i][4], final); 
                //for(int j =0; j<MAX_COL && table[i][j]; j++){
                    //printf("%s\t", table[i][j]); 
                //}
                printf("\n");
        }

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

