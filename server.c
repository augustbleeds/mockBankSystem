
 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <string.h>
 #include <errno.h>
 #include <ctype.h>

 #include <sys/types.h>
 #include <sys/socket.h>

 #include <netinet/in.h>
 #include <netdb.h>

 #include <pthread.h>

 #include "banksys.h"

 account **arr_account;
 int numAccount;
 pthread_mutex_t openAuthLock;


 // responsible for freeing data
 void * clientServiceFunction(void * arg){
      printf("Server has just accepted a client connection.\n");
      int client_socket_fd = *(int *) arg;
      // declare buffers
      char command[10];
      char name[100];
      char ibuffer[256];
      char obuffer[256];
      // clear buffers
      memset(ibuffer, 0, sizeof(ibuffer));
      memset(obuffer, 0, sizeof(obuffer));
      memset(command, 0, sizeof(command));
      memset(name, 0, sizeof(name));
      // command state
      command_state cs;
      // session state < 0 if not in session,
      // session state >=0 if in session (specifying location of account)
      int session = -1;
      // give command options
      char c_options[100] = "COMMANDS:\n\topen\n\tstart\n\tcredit\n\tdebit\n\tbalance\n\tfinish\n\texit";
      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "%s\n",c_options) ) < 0 ){
          printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
          exit(1);
        }
      memset(obuffer, 0, sizeof(obuffer));
      // read
      while( read(client_socket_fd, ibuffer, 255) > 0){
          sscanf(ibuffer, "%s %s",command, name);

          cs = determineCommand(command);

          switch(cs){
              case cs_open:
                  accOpen(client_socket_fd,&numAccount,&arr_account,name,&session,&openAuthLock);
                  break;
              case cs_start:
                  accStart(client_socket_fd,&arr_account,name,&session);
                  break;
              case cs_credit:
                  accCredit(client_socket_fd,&arr_account,name,&session);
                  break;
              case cs_debit:
                  accDebit(client_socket_fd,&arr_account,name,&session);
                  break;
              case cs_balance:
                  accBalance(client_socket_fd,&arr_account,&session);
                  break;
              case cs_finish:
                  accFinish(client_socket_fd,&arr_account,&session);
                  break;
              case cs_exit:     // exiting the session
                  if(session < 0){
                      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "- - - EXITED - - -\n- - -Thank you. Please come again! - - -.\n") ) < 0 ){
                          printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
                          exit(1);
                        }
                      printf("Server has closed a client connection\n");
                      close(client_socket_fd);
                      free(arg);
                      return 0;
                    }
                    // implicit finish of session to exit
                    arr_account[session]->service_flag = 0;
                    pthread_mutex_unlock(&(arr_account[session]->lock));
                    session = -1;
                      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "- - - EXITED - - -\n- - -Thank you. Please come again! - - -.\n") ) < 0 ){
                          printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
                          exit(1);
                        }
                    printf("Server has closed a client connection\n");
                    close(client_socket_fd);
                    free(arg);
                  return 0;
              default:
                  if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Invalid argument: %s\n",command) ) < 0 ){
                      printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
                      exit(1);
                  }
                  break;

          }
          if ( write(client_socket_fd, obuffer, sprintf(obuffer, "%s\n",c_options) ) < 0 ){
              printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
              exit(1);
            }
          // clear buffers
          memset(ibuffer, 0, sizeof(ibuffer));
          memset(obuffer, 0, sizeof(obuffer));
          memset(command, 0, sizeof(command));
          memset(name, 0, sizeof(name));
      }

      free(arg);
      return 0;
 }

 void *print_every_20(void * args){

      while(1){
          printf("BANK INFORMATION: \n");
          if(numAccount == 0){
              printf("\tNo Account have been opened yet\n");
          }else{
            //  printf("%15s %10s %13s", "ACCOUNT NAME", "BALANCE", "IN SERVICE");
              int i;
              for(i = 0 ; i < 20 ; i++){
                  account * ca = arr_account[i];
                  if(ca->service_flag >= 0){
                      if(ca->service_flag == 0){
                          printf("Account name: %s, Balance: %f, In Service: %s\n",ca->name,ca->balance,"NO");
                      }else{
                          printf("Account name: %s, Balance: %f, In Service: %s\n",ca->name,ca->balance,"YES");
                      }
                  }
              }
          }
          sleep(20);
      }
 }

 int main() {
    // server socket file descriptor
    int server_socket_fd;
    // client socket file descriptor (that server receives)
    int client_socket_fd;
    // request structs we need for socket communication
    struct addrinfo request;
    request.ai_flags = AI_PASSIVE;
    request.ai_family = AF_INET;
    request.ai_socktype = SOCK_STREAM;
    request.ai_protocol = 0;
    request.ai_addrlen = 0;
    request.ai_canonname = NULL;
    request.ai_next = NULL;
    // will point to results
    struct addrinfo *result;

    // retrieve structures
    getaddrinfo(0, "7007", &request, &result );

    // create socket
    if ( (server_socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol) ) < 0 ){
        printf("ERROR: SERVER COULD NOT BE CREATED: %s\n", strerror(errno));
        exit(1);
    }

    // bind the socket to our specified IP and port
    if( bind(server_socket_fd, result->ai_addr, result->ai_addrlen) < 0){
        printf("ERROR: SERVER COULD NOT BE CREATED: %s\n", strerror(errno));
        exit(1);
    }
    int optval = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR , &optval, sizeof(int));

    // ----------------------------------------------------------------------------
    // Create 20 accounts and initialize them
    // ----------------------------------------------------------------------------
    numAccount = 0;
    initAccounts(&arr_account);
    pthread_mutex_init(&openAuthLock, NULL);

    // thread that prints out accounts every 20 seconds

    pthread_t get_account_info;
    pthread_create(&get_account_info,NULL,&print_every_20,NULL);

    listen(server_socket_fd,5); // five connections can be queued
    void * client_socket_arg;   // socket argument for every client thread created
    pthread_t client;
    while(1){

        if( (client_socket_fd = accept(server_socket_fd, NULL, NULL)) < 0){
            printf("ERROR: FAILED TO ACCEPT: %s\n", strerror(errno));
            exit(1);
        }

        client_socket_arg = malloc(sizeof(int));
        memcpy(client_socket_arg, &client_socket_fd, sizeof(int));

        // create thread for client - service
        if (pthread_create(&client, NULL, &clientServiceFunction, client_socket_arg ) != 0){
            printf("ERROR: Can't create user server thread: %s\n", strerror(errno));
            exit(1);
        }

        if (pthread_detach(client) != 0){
            printf("ERROR: Could not detach client thread: %s\n", strerror(errno));
            exit(1);
        }



    }
 }
