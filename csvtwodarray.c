//trying to create a 2D array, in order to easily index it, makes life easier, 
//howver in C, we cannot have heterogenous values, therefore, tokenizing everything as a string, and then changing to int or float using atoi and atof when needed 

#include <stdio.h>
#include <string.h>
#include <stdlib.h> 

#define MAX_COL 10 
#define MAX_ROW 100
#define MAX_LEN 1024 

int main(){
    char provider[] = "Namma Yatri"; 
    int passengers = 4; 
    char ac_choice[] = "Yes";
    char premimum_choice[] = "Yes"; 

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

   for(int i = 1; i<row; i++){
        if(strcmp(table[i][0],provider)==0 && atoi(table[i][4])>=passengers && strcmp(table[i][2], ac_choice)==0 && strcmp(table[i][3],premimum_choice)==0){
            printf("%s %s %s %s\t", table[i][0], table[i][1], table[i][2], table[i][4]); 
            //for(int j =0; j<MAX_COL && table[i][j]; j++){
                //printf("%s\t", table[i][j]); 
            //}
            printf("\n"); 
        }

   }

    // Free allocated memory
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < MAX_COL && table[i][j]; j++) {
            free(table[i][j]);
        }
    }
    return 0; 
}

