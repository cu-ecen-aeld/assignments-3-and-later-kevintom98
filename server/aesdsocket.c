/****************************************************************************************************
  File name   : aesdsocket.c

  Created     : 28-Sep-2022

  Author      : Kevin Tom
                MS-ECEE-ESE
                CU Boulder, CO

  Email       : keto9919@colorado.edu

  Description : 

  Reference   : * Beej's Guide to Network Programming
                  https://beej.us/guide/bgnet/html/
******************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <syslog.h>
#include <fcntl.h> //For file operations
#include <string.h>
#include <error.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h> //for write function
#include <arpa/inet.h> // for  inet_ntop
#include <signal.h> //For handling signals



int socket_fd;

bool signal_received = false;
bool finished_writing = false;
bool deamon = false;
char *read_buf;
int f_fd = 0;



/* Adopted from * Beej's Guide to Network Programming
                  https://beej.us/guide/bgnet/html/

*/
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int is_newline_found(char *buf, size_t len)
{
  for(int i=0;i<len;i++)
    if(buf[i]=='\n')
      return i;
      
  return 0;
}



void write_to_file(int f_fd, char * read_buf, size_t len)
{
  int ret;
  while (len != 0 && (ret = write (f_fd, read_buf, len)) != 0) 
  {
    if (ret == -1) 
    {
      if (errno == EINTR)
          continue;
      perror ("write");
      break;
    }
    len -= ret;
    read_buf += ret;
  }
}





void send_packets(int f_fd, int soc_fd)
{

  int read_len = 1024;
  char read_buf[read_len];
  int ret =1, s_ret = 0;
  
  

  //Seeking to the beginning of file
  ret = lseek(f_fd,0,SEEK_SET);
  if(ret < 0)
  {
    perror("lseek");
    return;
  }

  //Reading from file
  do
  {
    ret = read (f_fd, read_buf, read_len);
    if (ret == -1) 
    {
      if (errno == EINTR)
      continue;
      perror ("read");
      break;
    }
    s_ret = send(soc_fd, read_buf, ret, 0);
    if(s_ret < 0)
    {
      perror("1- s_ret");
      break;
    }
      
  }while(ret != 0);

}


static void signal_handler(int signo)
{
  printf("\nCaught signal\n");
  syslog(LOG_DEBUG, "Caught signal, exiting");
  
  if(!finished_writing)
  {
    free(read_buf);
    close(f_fd);
    close(socket_fd);
    closelog();
    unlink("/var/tmp/aesdsocketdata");
    exit(0);
  }
  
  signal_received = true;
}





int main(int argc, char *argv[])
{

  if(argc >= 2)
    if(strcmp(argv[1],"-d")  == 0)
      deamon = true;



  char s[INET6_ADDRSTRLEN];
  struct addrinfo hints, *conn_info;
  memset(&hints, 0, sizeof hints);
  int  ret;

  read_buf = malloc(1024);

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, "9000", &hints, &conn_info);

  socket_fd = socket(conn_info->ai_family,conn_info->ai_socktype, conn_info->ai_protocol);
  if(socket_fd == -1)
    return -1;

  ret = bind(socket_fd, conn_info->ai_addr, conn_info->ai_addrlen);
  if(ret < 0)
  {
    perror("bind");
    return -1;
  }

  freeaddrinfo(conn_info);

  //Starting the deamon
  if(deamon)
    {
      pid_t pid;

      pid = fork ();
      if (pid == -1)
      {
          free(read_buf);
          close(socket_fd);
          perror("fork");
          return -1;
      }
      else if (pid != 0)
      {
          exit (EXIT_SUCCESS);
      }

      else //Valid pid
      {
        if(setsid()==-1)
        {
            free(read_buf);
            perror("session");
            close(socket_fd);
            return -1;
        }
        if(chdir("/")==-1)
        {
            free(read_buf);
            perror("chdir");
            close(socket_fd);
            return -1;
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        open ("/dev/null", O_RDWR);
        dup (0); 
        dup (0); 
      }
    }


  //Listening on the port
  ret = listen(socket_fd, 5);  
  if(ret < 0)
  {
    perror("listen");
    return -1;
  }

  struct sockaddr_storage their_addr;
  socklen_t addr_size = sizeof their_addr;


  int  new_fd = 0;
  f_fd = open("/var/tmp/aesdsocketdata",O_RDWR | O_CREAT | O_TRUNC | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP | S_IROTH | S_IWOTH); //666 equivalent value


  

  size_t len, total_len;
  int r_ret = 0;
  bool packet_complete = false;
  uint64_t realloc_count = 1;


  if (signal (SIGINT, signal_handler) == SIG_ERR) 
  {
    perror("signal()");
    return -1;
  }
  if (signal (SIGTERM, signal_handler) == SIG_ERR) 
  {
    perror("signal()");
    return -1;
  }



  while(!signal_received)
  {
    new_fd = accept(socket_fd, (struct sockaddr *)&their_addr, &addr_size);
    if(new_fd< 0)
    {
      perror("accept");
      return -1;
    }

    inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
    syslog(LOG_DEBUG, "Accepted connection from %s",s);
    printf("Accepted connection from %s",s);

    finished_writing = true;

    while((!packet_complete))
    {
      
      r_ret = recv(new_fd, (read_buf+((realloc_count-1)*1024)), 1024, 0);
      
      if(r_ret == 0)
      {
        //Connection is closed
        printf("Accepted connection from %s",s);
        syslog(LOG_DEBUG, "Closed connection from %s",s);
        break;
      }
      else if(r_ret < 1)
      {
        perror("recv");
        free(read_buf); //Freeing
        return -1;
      }

      //len = strlen(read_buf);
      len = ((realloc_count-1)*1024) + r_ret;   

      total_len = is_newline_found(read_buf,len);

      if(total_len > 0)
        packet_complete = true;

      if(!packet_complete)
      {
        realloc_count++;
        read_buf = realloc(read_buf, realloc_count*1024);
      }
        
      if(packet_complete)
      {
        write_to_file(f_fd, read_buf, len);
        send_packets(f_fd, new_fd);

        //Setting the variable to read next packet
        packet_complete = false;
      }
    }

    close(new_fd);
    finished_writing = false;
  }


  //Closing and freeing all the opened f_ds andmalloced buffers
  free(read_buf);
  close(socket_fd);
  close(f_fd);
  closelog();
  unlink("/var/tmp/aesdsocketdata");
  return 0;
}