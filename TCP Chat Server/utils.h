#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>

#define BUF_SIZE 1024
    
#define MAX_NAME 256
#define MAX_DATA 256
#define DATA_LENGTH 254
#define MAX_MESSAGES 256
#define MAX_USERS 8
#define MAX_SESSION_ID 256
#define MAX_SESSIONS 8
#define NO_SESSION "no_session"
    
struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

struct client {
    char id[MAX_NAME];
    bool isActive;
    int sockfd;
    int currentSession;
};

struct session {
    char sessionID[MAX_SESSION_ID];
    // each session has an array that keeps track of each client in the session
    // each element in this array is a pointer to a client in the global client array in server.c
    struct client* clientsInSession[MAX_USERS]; 
    int numUsers;
};

#define LOGIN 0
#define LO_ACK 1
#define LO_NAK 2
#define EXIT 3
#define JOIN 4
#define JN_ACK 5
#define JN_NAK 6
#define LEAVE_SESS 7
#define NEW_SESS 8
#define NS_ACK 9
#define MESSAGE 10
#define QUERY 11
#define QU_ACK 12

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */

