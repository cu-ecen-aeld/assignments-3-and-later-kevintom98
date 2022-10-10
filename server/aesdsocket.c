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
#include <pthread.h> //For threading
#include "queue.h"
#include <time.h>
#include <string.h>
//#include <sys/queue.h>


#define DATASIZE 1024


int socket_fd;
bool signal_received = false;
bool finished_writing = false;
int f_fd = 0;
pthread_mutex_t lock;
char buffer[80];

int time_increased;
int time_handled;

int num_of_threads;


struct connection_data
{
  pthread_t thread_id;
  int accepted_fd;
  //char conn_addr[INET6_ADDRSTRLEN];
  bool thread_active;
  //int file_fd;
};



struct thread_data
{
    struct connection_data *con_data;
    SLIST_ENTRY(thread_data) entries;             /* Singly linked list */
};




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



void write_to_file( char * read_buf, size_t len)
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





void send_packets(int soc_fd)
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
    //free(read_buf);
    //close(f_fd);
    close(socket_fd);
    closelog();
    unlink("/var/tmp/aesdsocketdata");
    exit(0);
  }
  signal_received = true;
}







void *packet_handler(void *conn_param)
{
  bool packet_complete= false;
  int r_ret;
  size_t len, total_len;
  uint64_t realloc_count = 1;
  char *read_buf = NULL;

  //Allocating initial buffer
  read_buf = malloc(DATASIZE);

  struct connection_data *conn_data = (struct connection_data *)conn_param;


  //printf("\nFD Thread: %d", conn_data->accepted_fd);

  while((!packet_complete))
    {

      r_ret = recv(conn_data->accepted_fd, (read_buf+((realloc_count-1)*DATASIZE)), DATASIZE, 0);
      
      if(r_ret == 0)
      {
        //Connection is closed
        // printf("\nClosed connection from %s\n",s);
        // syslog(LOG_DEBUG, "\nClosed connection from %s\n",s);
        close(conn_data->accepted_fd);
        break;
      }
      else if(r_ret < 1)
      {
        perror("recv");
        free(read_buf); //Freeing
        close(conn_data->accepted_fd);
        break;
      }


      len = ((realloc_count-1)*DATASIZE) + r_ret;   

      total_len = is_newline_found(read_buf,len);

      if(total_len > 0)
        packet_complete = true;
        

      if(!packet_complete)
      {
        realloc_count++;
        read_buf = realloc(read_buf, realloc_count*DATASIZE);
      }
        
      if(packet_complete)
      {

        pthread_mutex_lock(&lock);
        write_to_file(read_buf, len);
        printf("\n%s %ld\n",read_buf,conn_data->thread_id);
        send_packets(conn_data->accepted_fd);
        pthread_mutex_unlock(&lock);
        //close(f_fd);
        //Setting the variable to read next packet
        packet_complete = false;
      }
    }
    close(conn_data->accepted_fd);
    //conn_data->thread_active = false;
    return conn_data;
}


void timer_handler()
{

  time_t rawtime;
  struct tm *info;


  time( &rawtime );

  info = localtime( &rawtime );
  char time_val[40];
  info = localtime( &rawtime );
  strftime(time_val,40,"%Y/%m/%d - %H:%M:%S", info);
  sprintf(buffer,"timestamp: %s\n",time_val);
  //printf("Formatted date & time : |%s|\n", buffer );

  time_increased++;

}



int main(int argc, char *argv[])
{

  bool deamon = false;

  //Checking if the user requested to run the program as deamon
  if(argc >= 2)
    if(strcmp(argv[1],"-d")  == 0)
      deamon = true;


  //
  char s[INET6_ADDRSTRLEN];
  struct addrinfo hints, *conn_info;

  //Setting the structure elemetns to 0
  memset(&hints, 0, sizeof hints);
  //A common return variable for system calls
  int  ret;



  //Socket parameters
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  //Filling the structure with address info
  getaddrinfo(NULL, "9000", &hints, &conn_info);

  socket_fd = socket(conn_info->ai_family,conn_info->ai_socktype, conn_info->ai_protocol);
  if(socket_fd == -1)
  {
    printf("\nCannot create socket!");
    return -1;
  }
    

  ret = bind(socket_fd, conn_info->ai_addr, conn_info->ai_addrlen);
  if(ret < 0)
  {
    perror("bind");
    return -1;
  }

  //Freeing the address info after use
  freeaddrinfo(conn_info);

  //Starting the deamon
  if(deamon)
    {
      pid_t pid;

      pid = fork ();
      if (pid == -1)
      {
          //free(read_buf);
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
            //free(read_buf);
            perror("session");
            close(socket_fd);
            return -1;
        }
        if(chdir("/")==-1)
        {
            //free(read_buf);
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


  //File for writing read content
  int  new_fd = 0;
  

  // //Registering signals to use the signal handler
  // if (signal (SIGINT, signal_handler) == SIG_ERR) 
  // {
  //   perror("signal()");
  //   return -1;
  // }
  // if (signal (SIGTERM, signal_handler) == SIG_ERR) 
  // {
  //   perror("signal()");
  //   return -1;
  // }


  //struct sigaction timer_sig;
  //timer_sig.sa_handler = timer_handler;
  //timer_sig.sa_flags = 0;
  //sigset_t empty;
  //sigemptyset(&empty);
  //timer_sig.sa_mask = empty;
  //sigaction(SIGALRM, &timer_sig, NULL);

  struct sigaction sig_handler;
  sig_handler.sa_handler = signal_handler;
  sig_handler.sa_flags = 0;
  sigset_t empty_1;
  sigemptyset(&empty_1);
  sig_handler.sa_mask = empty_1;
  sigaction(SIGINT | SIGTERM, &sig_handler, NULL);


  timer_t timer;

  ret = timer_create (CLOCK_REALTIME,NULL,&timer);
  if (ret)
    perror ("timer_create");

  struct itimerspec ts;
  ts.it_interval.tv_sec = 10;
  ts.it_interval.tv_sec = 0;
  ts.it_value.tv_sec = 10;
  ts.it_value.tv_nsec = 0;

  ret = timer_settime(timer, 0, &ts, NULL);
  if(ret)
    perror("timer_setimer");

  //struct sigaction sigint_act;

  // sigint_act.sa_handler = &signal_handler;
  // sigint_act.sa_mask = ;
  // sigint_act.sa_sigaction = NULL;

  // sigaction (SIGINT, );

  f_fd = open("/var/tmp/aesdsocketdata",O_RDWR | O_CREAT | O_APPEND, 777);
  if(f_fd<0)
    perror("fopen");



  struct connection_data *conn = malloc(sizeof(struct connection_data));


  SLIST_HEAD(slisthead, thread_data)head;
  SLIST_INIT(&head);

  if (pthread_mutex_init(&lock, NULL) != 0)
  {
      perror("\npthread_mutex_init\n");
      return -1;
  }

 

  while(!signal_received)
  {

    //Accepting new connection
    new_fd = accept(socket_fd, (struct sockaddr *)&their_addr, &addr_size);
    if(new_fd < 0)
    {

      if(errno == EINTR)
      {
          //goto join_threads;
      }
      perror("accept");
      continue;
    }


    //printf("\nFD Main: %d", new_fd);


    //Parsing the address to print
    inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
    syslog(LOG_DEBUG, "Accepted connection from %s",s);
    printf("Accepted connection from %s\n",s);

    finished_writing = true;


    struct thread_data *start = NULL;
    struct thread_data *next = NULL;


    //Watching if thread ends
    SLIST_FOREACH_SAFE(start, &head, entries, next)
    {
      if(start->con_data->thread_active == false)
      {
        pthread_join(start->con_data->thread_id,NULL);
        //delete node

        SLIST_REMOVE(&head, start, thread_data, entries);
        free(start);
        num_of_threads--;
      }
      start = NULL;
    }

    if(num_of_threads == 0)
    {
      if(time_increased > time_handled)
      {
        write_to_file(buffer,strlen(buffer));
        time_handled++;
      }
    }

    struct thread_data *thd_pointer;
    thd_pointer = malloc(sizeof(struct thread_data));
    conn->thread_active = true;
    conn->accepted_fd = new_fd;
    ret = pthread_create((&conn->thread_id), NULL, packet_handler, conn);

    if(ret != 0)
    {
        errno = ret;
        perror("pthread_create");
        return -1;
    }
    else
    {
      thd_pointer->con_data = conn;
      SLIST_INSERT_HEAD(&head, thd_pointer, entries);
      num_of_threads++;
    }

    //Put it into linked list and rememebr to free
  }

  //Closing and freeing all the opened f_ds and malloced buffers
  //free(read_buf);
  close(socket_fd);
  close(f_fd);
  pthread_mutex_destroy(&lock);
  closelog();
  unlink("/var/tmp/aesdsocketdata");
  return 0;


}
