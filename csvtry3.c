#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN 1024 // Define a maximum length for the line

int main() {
    FILE *fp = fopen("data1.csv", "r"); 
    if (!fp) {
        perror("File could not be opened"); 
        return EXIT_FAILURE;
    }

    char line[MAX_LEN]; // Create a character array for each line

    fgets(line, sizeof(line), fp); // Skip the first line (headers)

    while (fgets(line, sizeof(line), fp)) { // Read each line from the file
        line[strcspn(line, "\n")] = '\0'; // Remove the newline at the end

        char *token = strtok(line, ","); // Split the line by commas

        // Process the first token (Age)
        if (token != NULL) {
            int age = atoi(token); // Convert age to integer

            // Only process the row if the age is greater than 23
            if (age <= 23) {
                continue; // Skip the current row if age is <= 23
            }

            // Now process the other columns (Math, Science, English)
            printf("Age: %d\n", age); // Print the Age
            printf("Scores: ");

            // Get the Math score
            token = strtok(NULL, ",");
            int math = (token != NULL) ? atoi(token) : 0; // Convert Math score

            // Get the Science score
            token = strtok(NULL, ",");
            int science = (token != NULL) ? atoi(token) : 0; // Convert Science score

            // Get the English score
            token = strtok(NULL, ",");
            int english = (token != NULL) ? atoi(token) : 0; // Convert English score

            // Calculate the formula: math + (science * english)
            int result = math + (science * english);

            // Print the result of the formula
            printf("Math: %d, Science: %d, English: %d\n", math, science, english);
            printf("Formula Result: %d\n", result);
            printf("----\n"); // Separator between records
        }
    }

    fclose(fp); // Close the file after reading
    return 0; 
}
