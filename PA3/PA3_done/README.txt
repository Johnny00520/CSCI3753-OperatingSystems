CSCI 3753 - Programming Assignment 3: DNS Name Resolution Engine 
Student Name: Chen Hao Cheng

Student Email: chch6597@colorado.edu

Student ID: 104666885

Files:
multi-lookup.c: Program that creates requester threads for each input text file. These threads write the domain names to a queue. Multiple resolver threads pull the domain names from the queue and resolve the IP address and write them to an output text file.

multi-lookup.h: A header file that contains prototypes for the functions addRequestToQueue and resolve_DNS.

Makefile: Builds the multi-lookup program as the default target. Also contains a 'clean' target that will remove any files generated during the course building and runnning the program.

performance.txt: Run the program in 6 scenarios over 5 input files provided in the input directory.
1) 1 requester thread, 1 resolver threads
2) 1 requester thread, 3 resolver threads
3) 3 requester thread, 1 resolver thread
4) 3 requester thread, 3 resolver thread
5) 5 requester thread, 5 resolver thread
6) 8 requester thread, 5 resolver thread

How to build:
From the command line:
1) make clean
2) make (To create result.txt serviced.txt)

./multi-lookup num_requester-threads num_resolver-threads result.txt serviced.txt names1.txt names2.txt names3.txt names4.txt names5.txt

To evaluate memory management:
valgrind ./multi-lookup requester-threads resolver-threads result.txt serviced.txt names1.txt names2.txt names3.txt names4.txt names5.txt

Compile the multi-lookup:
gcc -pthread -o multi-lookup multi-lookup.c
