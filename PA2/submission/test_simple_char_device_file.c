#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define DEVICE "/dev/simple_character_device"
#define BUFFER_SIZE 1024

//int maximum = 1024;
char userInput;
int *readInput = 0, *seekOffset = 0, *seekWhence = 0;
char buff[BUFFER_SIZE];
char writeInput;

int main(){
    int file = open(DEVICE , O_RDWR);
    char *ptrr = 0;
    while(1){
        printf("a.  Press r to read from device \nb.  Press w to write to the device \nc.  Press s to seek into the device \nd.  Press e to exit from the device \ne.  Press anything else to keep reading or writing from the device \nf.  Enter command: \n");
        scanf("%c", &userInput);
        
        switch(userInput){
            case 'r': case 'R':

                printf("Enter the number of bytes you want to read: \n");
                scanf("%d", &readInput);
                //ptr is pointer of cast-type. The malloc() function returns a pointer to an area of memory with size of byte size. If the space is insufficient, allocation fails and returns NULL pointer.
                //memory allocated using malloc

                //ptrr = malloc(sizeof(readInput));
                ptrr = (char *) malloc(sizeof(readInput));

                if(ptrr == NULL)
                {
                    printf("Error! memory not allocated.!");
                    exit(0);
                }
            
                //read(file, ptrr, readInput); //print the same thing
                read(file, ptrr, readInput);

                //printf("Data read from the device: %d\n", ptrr );
                printf("Data read from the device: %1s \n", &readInput);
                //printf("Data read from the device: %s \n", readInput);
                free(ptrr);

                while(getchar() != '\n');
                break;
                 
            case 'w': case 'W':
                printf("Enter the data you want to write to the device: \n");
                scanf("%s", buff);
                write(file, buff, strlen(buff));

                //write(file, buff, BUFFER_SIZE);
                
                while(getchar() != '\n');
                break;
            
            case 's': case 'S':
                printf("Enter an offset value: \n");
                scanf("%d", &seekOffset);
                printf("Enter a value for whence(third parameter): \n");
                scanf("%d", &seekWhence);
                llseek(file, seekOffset, seekWhence);
                

                while(getchar() != '\n');
                break; 

            case 'e': case 'E':    
                printf("Peace!!");
                return 0;
            
            default:    
                while(getchar() != '\n');
        }
    }
    close(file);
    return 0;
}
