/****************************************************************************************************
  File name   : writer.c

  Created     : 02-Sep-2022

  Author      : Kevin Tom
                MS-ECEE-ESE
                CU Boulder, CO

  Email       : keto9919@colorado.edu

  Description : This file creates a file and write the string into it. Two command line
                arguements to be given while running the executable file:
                Arg #1 : Full path of a file (including filename)
                Arg #2 : Text string to be written to the Arg #1 file


  Reference   : Linux System Programming (Second Edition)
                Robert Love


  How to compile the script
  -------------------------
  1. Open terminal in the same directory as this file.
  2. Type "make" to compile the file using gcc compiler.
  3. Run "./writer <filepath_with_file_name> <string>" command to run the code.
  4. For crosscompiling do "make clean" to clear the already geenrated executables (if any)
  5. Then run "make CROSS_COMPILE = aarch64-none-linux-gnu-" to generate executable for the target.
******************************************************************************************************/


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





/*
  Name         : usage()
  Descrirption : This function prints what arguments are needed in to run the output file.
  Inputs       : None
  Returns      : None
*/
void usage()
{
    printf("The program needs 2 Arguments \n"
            "---------------------------- \n"
            "Arg #1 : Full path of a file (including filename) \n"
            "Arg #2 : Text string to be written to the Arg #1 file \n");
}


/*
  Name         : void write_to_file()
  Descrirption : This function writes the passed string to the file referenced
                 by the file descriptor passed. If the file already exist it
                 will be overwritten.
  Inputs       : fd     - file descriptor of the opened file.
                 string - pointer to cahr string to be written.
  Returns      : None
*/
void write_to_file(int fd, char * string)
{
    // Variable to store write() return value
    ssize_t w_ret;

    //Getting string length
    size_t len = strlen(string);

    while(len != 0)
    {
        //Writing to the file
        w_ret = write (fd,string,len);
        
        //Checking the return value for error
        if(w_ret == -1)
        {
            //In the case of follwing error try again
            if(errno == EINTR || errno == EAGAIN)
                continue;
            else
            {
                perror("write");
                exit(1);
            }
        }
        else
        {
            //Decrement len varaible according to characters written
            len    -= w_ret;
            
            //Increment string pointer according to characters written
            string += w_ret;
        }
    }
    syslog(LOG_USER,"Wrote the string sucessfully to the file!");
    printf("Wrote the string sucessfully to the file!\n");
}



/*
  Name         : open_file()
  Descrirption : This function opens the file with filename (along with file path) 
                 passed to it. If the file does not exist then it will be created.
  Inputs       : file - filepath along with filename
  Returns      : int - descriptor of the file opened.
*/
int open_file(char *file)
{
    //variable to store the file descriptor.
    int fd;

    //opening the file for write only, truncate to 0 length if file already exist and
    //create a newfile if file does not exist.
    fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IROTH);
    if(fd == -1)
    {
        syslog(LOG_ERR, "Cannot access/create the give file");
        exit(1);
    }

    syslog(LOG_USER,"Sucessfully opened the file!");
    return fd;
}






int main(int argc, char *argv[])
{

    //Checking if 2 arguments are given or not
    if(argc <= 1 || argc > 3)
    {
        syslog(LOG_ERR,"Need 2 arguments!!"
                       "Arg #1 : Full path of a file (Including filename)"
                       "Arg #2 : Text string to be written to the Arg #1 file");
        
        usage();
        exit(1);
    }

    //Saving arguemtns to variables
    char *filepath     = argv[1];
    char *str_to_write = argv[2];
    char *filename     = basename(argv[1]);

    //Printing log data
    syslog(LOG_DEBUG,"Writing %s to %s", str_to_write, filename);
    printf("Writing %s to %s \n", str_to_write, filename);


    //Opening file
    int fd = open_file(filepath);
    
    //Writing string to file
    write_to_file(fd,str_to_write);

    return 0;
}