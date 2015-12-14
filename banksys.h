#ifndef BANKSYS_H
#define BANKSYS_H

#include <pthread.h>
#include <ctype.h>

    typedef enum command_state_ {
        cs_open,
        cs_start,
        cs_credit,
        cs_debit,
        cs_balance,
        cs_finish,
        cs_exit,
        cs_notexit,
    } command_state;

    typedef struct account_ {
        char name[100];
        float balance;
        int service_flag;              // -1: Not created, 1: In service,  0: Not in service
        pthread_mutex_t lock;
    } account;

    int isamount(char * amount);

    command_state determineCommand(char * command);

    void initAccounts(account ***arr_account);

    // searches bank for account.
    // return < 0 if not found. And index number if found.
    int searchAccount(account ** arr_account, char * name);

    void accOpen(int client_socket_fd, int *numAccount, account ***arr_account, char * name, int *session, pthread_mutex_t * openAuthLock);

    void accStart(int client_socket_fd,  account ***arr_account, char * name, int *session);

    void accCredit(int client_socket_fd, account ***arr_account, char * amount, int *session);

    void accDebit(int client_socket_fd, account ***arr_account, char * amount, int *session);

    void accBalance(int client_socket_fd,  account ***arr_account, int *session);

    void accFinish(int client_socket_fd, account ***arr_account,  int *session);

    //void accExit(int client_socket_fd, int *session);
#endif
