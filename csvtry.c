#include <stdio.h> 
#include <stdlib.h> //for error handling
#include <string.h> //for functions wrt strings

#define MAX_LEN 1024 //no line is longer than 1023 + '\n' characters 


/*int main(){

    FILE *fp = fopen("Data.csv","r"); 
    //in order to check whether the file exists/opens 
    if(!fp){
        perror("File could not be opened"); 
        return EXIT_FAILURE; 

    }

    char line[MAX_LEN]; //creating a character array for each line 

    fgets(line,sizeof(line),fp); //skips the first line 

    while (fgets(line,sizeof(line),fp))//fgets gets a line at a time, and stores it in line 
    {
        line[strcspn(line,"\n")] = '\0'; //removing newline; \0-> shows the end of the string. essentially replacing newline character to end of string character 

        char *token = strtok(line,","); //splits the lines at wherever a comma is present 

        //lets say i want to print out only those whose ages>23 
        if(token!=NULL){
            printf("%s\n", token); 
            
        }
        token = strtok(NULL, ",");
        if(token!=NULL){
            int age = atoi(token); // converts str to int 
            if(age<=23){
               continue;
                
            }
            printf("%d\n", age); 
        }
        token = strtok(NULL, ",");
        if (token != NULL) {
            float score = atof(token); // Convert string to float
            printf("%f\n", score);
        }
    }

    fclose(fp); 
    return 0; 

    
}*/ 

int main() {
    FILE *fp = fopen("Data.csv", "r"); 
    if (!fp) {
        perror("File could not be opened"); 
        return EXIT_FAILURE;
    }

    char line[MAX_LEN]; // Create a character array for each line

    fgets(line, sizeof(line), fp); // Skip the first line (headers)

    while (fgets(line, sizeof(line), fp)) { // Read each line from the file
        line[strcspn(line, "\n")] = '\0'; // Remove the newline at the end

        char *token = strtok(line, ","); // Split the line by commas

        // Process the first token (Name)
        if (token != NULL) {
            char *name = token; // Store the name token
            token = strtok(NULL, ","); // Move to the second token (Age)

            // Now we process the age and check if it is greater than 23
            if (token != NULL) {
                int age = atoi(token); // Convert age to integer

                if (age <= 23) {
                    continue; // Skip this record if age is <= 23
                }

                printf("Name: %s\n", name); // Print Name
                printf("Age: %d\n", age);   // Print Age
            }

            token = strtok(NULL, ","); // Move to the third token (Score)
            if (token != NULL) {
                float score = atof(token); // Convert score to float
                printf("Score: %.2f\n", score); // Print Score with 2 decimal places
            }

            printf("----\n"); // Separator between records
        }
    }

    fclose(fp); // Close the file after reading
    return 0; 
}
