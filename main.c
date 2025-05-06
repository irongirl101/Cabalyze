#include <stdio.h> 


/*
HERES THE PLAN 

- FRONTEND 

GET DATA FROM USER 
- LOCATION START 
- LOCATION END 
- TIME (IF NOW, GET COMPUTER TIME)
- NUMBER OF PASSENGERS 
- MULTIPOINT? 
- APP OF INTEREST? 
- AC OR NON AC 
- PREMIUM OR NOT PREMIUM 


SPIT OUT THE BEST OPTION AS PRIMARY, BUT ALSO MENTION THE OTHER OPTIONS 

-BACKEND 

FORMULA IS PRESENT FOR EACH APP -> UBER, OLA, RAPIDO ECT ETC 
AND DATA IS KNOWN FOR EACH PART OF THE OF THE FORMULA  
except for surge multiplier, which will be gathered from the surge table 

Fare=Base Fare+(Distance×Per Kilometer Rate)+(Time×Per Minute Rate)+Booking Fee

Final Fare= Fare * Surge Multiplier

Explaination for each part of the formula, where to get what from 

base Fare -> fare data table 
diatance -> distance got from distance matrix api (or anything)
per km rate -> from fare data table 
time -> time taken for the journey 
per minute -> from data set 
booking fee -> from data set 
surge multiplier -> based off current weather using api, surge multiplier can be taken 

compare everything based on the number of passengers, and spit out the final amount 


note to future self, look at how everything is calculated ones, before finishing it up  
*/

int main(){


    return 0; 
}