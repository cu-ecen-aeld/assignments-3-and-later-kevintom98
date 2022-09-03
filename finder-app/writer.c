/**************************************************************************************
  File name   : writer.c

  Created     : 02-Sep-2022

  Author      : Kevin Tom
                MS-ECEE-ESE
                CU Boulder, CO

  Email       : keto9919@colorado.edu

  Description : This file creates a file and write the string into it.

  Reference   : Linux System Programming (Second Edition)
                Robert Love
****************************************************************************************/


#include <syslog.h> // For syslogging
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>  //For read/write/open file operations
#include <stdlib.h>  //For exit() function
#include <stdbool.h> //For using bool type




#include <stdio.h> // CAREFULLLLLLLLL DELETE THISSSSSS





void write_to_file(int fd, char * string)
{
    ssize_t w_ret;
    

    printf("\n\nSize:%d",sizeof(string));
    //while(w_ret = write(fd,string,))

}



int open_file(char *file)
{
    int fd;
    fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0700);
    if(fd == -1)
    {
        syslog(LOG_ERR, "Cannot access/create the give file");
        exit(1);
    }
    return fd;
}




int main(int argc, char *argv[])
{

    if(argc <= 1 || argc > 3)
    {
        syslog(LOG_ERR,"Need 2 arguments!! \n"
                       "Arg #1 : Full path of a file (Including filename) \n"
                       "Arg #2 : Text string to be written to the Arg #1 file \n");

        printf("No 2 arguments\n\n");
        exit(1);
    }

    char *filepath     = argv[1];
    char *str_to_write = argv[2];

    int fd = open_file(filepath);
    write_to_file(fd,str_to_write);


    return 0;
}