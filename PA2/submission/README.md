##CSCI 3753 - Programming Assignment 2: Coding a Simple Character Device Driver

##Student Name: Chen Hao Cheng

##Student Email: chch6597@colorado.edu

##Student ID: 104666885

Below are files
#simple_char_driver.c:

This file declares init and exit functions for module. The character driver is registered in the init function using register_chrdev(). The register_chrdev() function is passed the the major number for the device we are reistering and the file operations structure for the driver to execute. The character driver is unregisterd in the exit function using unregister_chrdev(). Functions for file operations such as open, close, read, and write are included in this file. The write function first increments the offset to ensure we are writing to the end of the device buffer. It then ensures we are not writing beyond the allocated space for the buffer. This can happen if the offset is placed beyond the end of the buffer or the data we are trying to write exceeds the last index of the buffer. Following those checks, we write to the device buffer from the userspace using copy_from_user. The read function also performs some initial checks in order to catch attempts to read beyond the end of the device buffer or read more bytes than are contained in the buffer. The read function transfers data from the device buffer to the userspace through the copy_to_user() function. 

#test.c:

Enables the user to write and read into the character device buffer with interactive
options such as read, write, and exit.

#Makefile:

This file is used to build the binary program from simple_char_driver source code.

##Creating Device File for a Device Driver

What instructions said to do:

sudo mknod –m 777 /dev/simple_character_device c 240 0

**What worked for me:**

1. Find available major numbers in /home/kernel/linux-4.4.0/Documentation/devices.txt or by cat /proc/devices. 234 was available on my machine.

2. sudo mknod simple_character_device c 234 0 

3. sudo chmod a+r+w simple_character_device


##How to run:

#Load Module:

**Compile Module**: make –C /lib/modules/$(uname -r)/build M=$PWD modules (What it looked like on my machine: make –C /lib/modules/$(uname -r)/build M=$PWD modules)

**Insert module into kernel:** sudo insmod helloModule.ko

**Remove module from kernel:** sudo rmmod helloModule

**Compiling and running test app**:

Make sure the module is loaded into the kernel (Very important!).

Compile: In the command line type gcc -o test test.c
Run: In the command line type ./test

**Checking the system log file**
The loadable module will print information about its operation to the system log file. To view its contents type dmesg.

