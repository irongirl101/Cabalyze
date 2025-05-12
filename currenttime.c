#include <stdio.h> 
#include <time.h>

int main(){
    time_t current_time; //time_t is a data type used for time 
    struct tm * local_time; //local_time acts as a pointer towards the struct 
    char t[6]; 

    current_time = time(NULL);  //time(NULL) -> gets the current time 
    local_time = localtime (&current_time);  //gets address of where this is stored 

    strftime(t,sizeof(t), "%H:%M", local_time); //formatting 

    printf("%s \n", t); 

    return 0; 
}
