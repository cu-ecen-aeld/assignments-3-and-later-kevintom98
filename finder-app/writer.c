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


#include <syslog.h>     // For syslogging
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>     //For read/write/open file operations
#include <stdlib.h>     //For exit() function
#include <stdbool.h>    //For using bool type
#include <errno.h>      //For checking error 
#include <stdio.h>      //For using printf()
#include <string.h>     //For using string functions such as strlen()
#include <libgen.h>     //For using basename() function





void write_to_file(int fd, char * string)
{
    ssize_t w_ret;
    size_t len = strlen(string);

    while(len != 0)
    {
        w_ret = write (fd,string,len);
        if(w_ret == -1)
        {
            if(errno == EINTR || errno == EAGAIN)
                continue;
            else
            {
                perror("write");
                break;
            }
        }
        else
        {
            len    -= w_ret;
            string += w_ret;
        }
    }
    printf("Wrote the string sucessfully to the file!\n");
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
        syslog(LOG_ERR,"Need 2 arguments!!"
                       "Arg #1 : Full path of a file (Including filename)"
                       "Arg #2 : Text string to be written to the Arg #1 file");

        printf("The program needs 2 Arguments \n"
               "---------------------------- \n"
               "Arg #1 : Full path of a file (including filename) \n"
               "Arg #2 : Text string to be written to the Arg #1 file \n");
        exit(1);
    }

    char *filepath     = argv[1];
    char *str_to_write = argv[2];
    char *filename     = basename(argv[1]);

    syslog(LOG_DEBUG,"Writing %s to %s", str_to_write, filename);
    printf("Writing %s to %s \n", str_to_write, filename);

    int fd = open_file(filepath);
    write_to_file(fd,str_to_write);

    return 0;
}