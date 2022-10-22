#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

void simulateCache(char *fileName, long** cache, int setNumber, int Associativity, int blockSize );
void strip(char *hexAddress);
long hexToDecimal(char* hexAddress);
long** initializeCache(int setNumber, int lineNumber);
void hitOrMiss(long** cache, short** usage, int index, int associativity, int tag, int* hit, int* miss, int* eviction);// I guess they never miss, huh? ¬‿¬

//Ömer Faruk TAL
int main(int argc, char *argv[] ) {

    // Here we will initializie flags from the user command line input 
    int opt = 0;
    int setBit = 0;
    int associativity = 0;
    int blockBit = 0;
    char *traceFile = NULL;


    while ((opt = getopt(argc, argv, "vs:E:b:t:")) != -1) {
        
        if (opt == 's') {
            setBit = atoi(optarg);
        }
        else if (opt == 'E') {
            associativity = atoi(optarg);
        }
        else if (opt == 'b') {
            blockBit = atoi(optarg);
        }
        else if (opt == 't') {
            traceFile = optarg;
        }
        else {
            printf("You have entered a unrecongized option. Porgram will terminate.\n ");
            return 0;
        }
    }
    
    // Cache will be initialized with using flag inputs
    long** cache = initializeCache( (1<<setBit) , associativity );
    // In this function we will read file and print summary
    simulateCache(traceFile, cache, (1 << setBit), associativity, (1<< blockBit) );

    // Free'ing the heap memory 
    for (int i = 0; i < (1 << setBit); i++) {
        free(cache[i]);
    }
    free(cache);
    return 0;
}



void simulateCache(char *fileName, long** cache, int setNumber, int associativity, int blockSize ) {
    
    // We will initialize hit, miss, eviction values and open the file
    FILE *fp = fopen(fileName, "r");

    int hit      = 0;
    int miss     = 0;
    int eviction = 0;

    const unsigned MAX = 20;
    char buffer[MAX];
  
    // We will initialize a usage array for deciding eviction (Least Recently Used array)
    // With this we will be able to identify the 'least recently used' block in the  cache line
    short ** usage = (short **) malloc( sizeof(short*) * setNumber);
  
    for (int i = 0; i < setNumber; i++ ) {
        usage[i] = (short*) malloc(sizeof(short)* associativity);
        for (int j = 0; j < associativity; j++) {
            usage[i][j] = associativity - 1 -j;
        }
    }
    
    // We started to read the lines in trace file
    while (fgets(buffer, MAX, fp)) {
        
        // We are only interested int Modify, Load, Store operations
        if (buffer[1] == 'M' || buffer[1] == 'L' || buffer[1] == 'S') {
            
            // Hex adress is found in the line
            char *hexAddress = buffer + 3;
            // The unrelated information is stripped
            strip(hexAddress);
            // The hex address is turned into a decimal value
            long decimal = hexToDecimal(hexAddress);

            // index and tag values are calculated from the hex address (but in decimal form)
            int index = (decimal / blockSize) % setNumber;
            long tag = (decimal / blockSize) / setNumber;

            // Depending on the operation type, hitOrMiss Function will called ( which increments the hit, miss eviction values) 
            // If Load operation 
            if (buffer[1] == 'L') {
                
                hitOrMiss(cache, usage, index, associativity, tag , &hit, &miss, &eviction);

            }
            // If Strore operation
            else if (buffer[1] == 'S') {

                hitOrMiss(cache, usage, index, associativity, tag , &hit, &miss, &eviction);
            }
            // If Modify operation (basically one Load, and Store operation)
            else {

                hitOrMiss(cache, usage, index, associativity, tag , &hit, &miss, &eviction);
                hitOrMiss(cache, usage, index, associativity, tag , &hit, &miss, &eviction);
            }


       }


    }
    
    // Close file 
    fclose(fp);
    
    // Free the heap memeory
    for (int i = 0; i < setNumber; i++ ) {
        free(usage[i]);
    }
    free(usage);

    // Print related information
    printSummary(hit, miss, eviction);

}

/*
 * Helper Functions
 */

// This function with using cache and usage in harmony to increment hit, miss and eviction accordingly
void hitOrMiss(long** cache, short** usage, int index, int associativity, int tag, int* hit, int* miss, int* eviction) {
        
        
        int hitFlag = -1; 
        // With using index we try to find tag value in cache 
        for (int i = 0; i < associativity; i++) {
            if (cache[index][i] == tag) {
                (*(hit))++;
                hitFlag = i;
            }

        }
        
        
        // If tag is found in cache
        if (hitFlag >= 0) {
            
            // Update the usage array accoringly (Only increase the usage values that are smaller than the found block, make found block usage 0)    
            int hitLREValue = usage[index][hitFlag];

            for (int i = 0; i < associativity; i++ ) {
                if (usage[index][i] < hitLREValue ) {
                    usage[index][i]++;
                }
                if (i == hitFlag) {
                    usage[index][i] = 0;
                }
            }
            return;
        }
        // In miss case
        else {
            
            // Found which array is the least recently used
            int replaceIndex = 0;

            for (int i = 0; i < associativity; i++) {
                if (usage[index][i] == associativity -1) {
                    replaceIndex = i;
                }
            }
            
            // If the tag in the cache is -1 this means that it is a cold miss,
            // Else we need to evict this 'least recently used block' in cache
            if (cache[index][replaceIndex] != -1) {
                (*(eviction))++;
            }
            
            //Update cache
            cache[index][replaceIndex] = tag;
            
            (*(miss))++;
            // Since it is a miss, update usage array (basically make usage of replaced block 0, increment others)
            for (int i = 0; i < associativity; i++) {
                usage[index][i] = ( usage[index][i] + 1 ) % associativity;
            }

       }

}

// With using setNumber, and lineNumber we initialize our cache
// Our cache will be storing the tag values
long** initializeCache(int setNumber, int lineNumber) {
    
    
    long** cache = (long **) malloc(sizeof(long*) * setNumber);
    for (int i = 0; i < setNumber ; i++) {

        cache[i] = (long *) malloc(sizeof(long) * lineNumber );

    }
    
    // Our cache is 'cold' at the beginning so every element will be -1 to state that
    for (int i = 0; i < setNumber; i++) {
        for (int j = 0; j < lineNumber ; j++) {
            cache[i][j] = -1;
        }
    }

    return cache;

}


// This function takes a hexAdress in string form and turn it into a decimal integer value
long hexToDecimal(char* hexAddress) {

    long decimalValue = 0;
    int length = strlen(hexAddress);


    for (int i = 0; i < length; i++) {
        char* num = hexAddress + length - 1 - i;
        long basamak = 0;
        if      (*num == '0') basamak = 0;
        else if (*num == '1') basamak = 1;
        else if (*num == '2') basamak = 2;
        else if (*num == '3') basamak = 3;
        else if (*num == '4') basamak = 4;
        else if (*num == '5') basamak = 5;
        else if (*num == '6') basamak = 6;
        else if (*num == '7') basamak = 7;
        else if (*num == '8') basamak = 8;
        else if (*num == '9') basamak = 9;
        else if (*num == 'a') basamak = 10;
        else if (*num == 'b') basamak = 11;
        else if (*num == 'c') basamak = 12;
        else if (*num == 'd') basamak = 13;
        else if (*num == 'e') basamak = 14;
        else if (*num == 'f') basamak = 15;
        *(hexAddress + length -1 - i ) = '\0';
        long factor = 1;
        decimalValue += basamak * (factor << (4*i) );
    }
    return decimalValue; 
}


// This function takes the beginning of hexAdress and strip
// the characters after the ',' which is unrelated with our assignment
void strip(char *hexAddress){
    
    int i = 0;

    while (*(hexAddress + i) != ',' && *(hexAddress + i) != '\0') {
        i++;
    }
    
    if (*(hexAddress + i) == ',' ) {
        *(hexAddress + i) = '\0';
    }

}













