#include "utils.h"

#define BACKLOG_SIZE 32

// function protypes
void serialize_server_message(char* buf);
void deserialize_message(char* buf);
bool authUserLogin(int sockfd);
void listUsers(int clientIndex);
void logOutUser(int sockfd, int clientIndex);
void sendServerMessage(int type, int sockfd);
void processCommand(char* buf, int sockfd, int clientIndex);
void sendClientMessage(int clientIndex, char* message);
void fixCommands(int clientIndex);

// session prototypes
void joinSession(int clientIndex, char* sessionID);
void leaveSession(int clientIndex);
void createSession(int clientIndex, char* sessionID);
int doesSessionExist(char* sessionID);


// global variables
struct message m;
struct message response;
struct session sessionList[MAX_SESSIONS];
fd_set master;

//array that keeps track of all clients
//note: login information is included in logininfo.txt in Lab4 folder
struct client clientList[MAX_USERS] = {
    {.id = "client1", .isActive = false, .sockfd = -1, .currentSession = -1},
    {.id = "client2", .isActive = false, .sockfd = -1, .currentSession = -1},
    {.id = "client3", .isActive = false, .sockfd = -1, .currentSession = -1},
    {.id = "client4", .isActive = false, .sockfd = -1, .currentSession = -1},
    {.id = "client5", .isActive = false, .sockfd = -1, .currentSession = -1},
    {.id = "client6", .isActive = false, .sockfd = -1, .currentSession = -1},
    {.id = "client7", .isActive = false, .sockfd = -1, .currentSession = -1},
    {.id = "client8", .isActive = false, .sockfd = -1, .currentSession = -1}
};

int main(int argc, char** argv) {
    // checks if there are enough arguments
    if (argc != 2) {
        printf("Bad arguments\n");
        exit(1);
    }

    //fd_set master;
    fd_set read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    // gets socket fd
    struct addrinfo hints;
    struct addrinfo* port_addr, *p;
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // gets address info of port
    if (getaddrinfo(NULL, argv[1], &hints, &port_addr) != 0) {
        printf("Error getting address info\n");
        exit(1);
    }
    int sockfd;
    int newfd;
    int fd_max;
    int yes = 1;
    char buf[BUF_SIZE];
    char remoteIP[INET6_ADDRSTRLEN];
    int nbytes;

    //Code taken from Beej's Guide to Network Programming https://beej.us/guide/bgnet/html/#cb58-86
    //find socket to bind to
    for (p = port_addr; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) {
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int));

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        printf("Failed to bind\n");
        exit(1);
    }

    FD_SET(sockfd, &master);
    fd_max = sockfd;

    freeaddrinfo(port_addr);

    if (listen(sockfd, BACKLOG_SIZE) == -1) {
        printf("Error listening for request\n");
        exit(1);
    }
    
    
    while(true) {
        read_fds = master; 

        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }

        // run through the existing connections looking for data to read
        for (int i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_fds)) { // found connection
                if (i == sockfd) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(sockfd, (struct sockaddr *) &remoteaddr, &addrlen);
                    if (newfd == -1) {
                        perror("accept");
                        continue;
                    }
                    
                    if (authUserLogin(newfd)) {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fd_max) { // keep track of the max
                            fd_max = newfd;
                        }
                    }

                } else {
                    // loop through all of the users and check if they are active
                    for (int j = 0; j < MAX_USERS; j++) {
                        if (clientList[j].isActive && FD_ISSET(clientList[j].sockfd, &read_fds)) {
                            //receive message from active clients
                            int recv_sz;
                            if ((recv_sz = recv(clientList[j].sockfd, buf, BUF_SIZE, 0)) == -1) {
                                printf("Error receiving message\n");
                                continue;
                            } else if (recv_sz != 0)  {
                                //process the command from the client 
                                processCommand(buf, clientList[j].sockfd, j);
                            }

                        }
                    }
                    
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    }
}

//luiz.navarro@mail.utoronto.ca

void serialize_server_message(char* buf) {
    printf("sent %u:%u:%s:\n", response.type, response.size, response.source);
    snprintf(buf, BUF_SIZE, "%u:%u:%s:", response.type, response.size, response.source);
    memcpy(&(buf[strlen(buf)]), &(response.data), response.size);
}


// generates a message from a string buf
void deserialize_message(char* buf) {
    int buf_index = 0;
    char out[BUF_SIZE];
    int out_index = 0;
    
    // extract type from buf into out and then into m
    while (buf[buf_index] != ':') {
        out[out_index] = buf[buf_index];
        ++out_index;
        ++buf_index;
    }
    m.type = atoi(out);
    memset(out, 0, sizeof(out));
    out_index = 0;
    ++buf_index;
    
    // extract size from buf into out and then into m
    while (buf[buf_index] != ':') {
        out[out_index] = buf[buf_index];
        ++out_index;
        ++buf_index;
    }
    m.size = atoi(out);
    memset(out, 0, sizeof(out));
    out_index = 0;
    ++buf_index;
    
    // extract source from buf into out and then into m
    while (buf[buf_index] != ':') {
        out[out_index] = buf[buf_index];
        ++out_index;
        ++buf_index;
    }
    out[out_index] = '\0';
    strcpy(m.source, out);
    out_index = 0;
    ++buf_index;
    
    
    // extract data from buf into out and then into m
    while (out_index < m.size) {
        m.data[out_index] = buf[buf_index];
        ++out_index;
        ++buf_index;
    }
    
    printf("received %u:%u:%s\n", m.type, m.size, m.source);
}

// authenticate user login information
bool authUserLogin(int sockfd) {
    char buf[BUF_SIZE];
    if (recv(sockfd, buf, BUF_SIZE, 0) == -1) {
        printf("Error receiving message\n");
    }
    FILE *authPtr;
    char str[MAX_DATA];
    char temp[MAX_DATA];
    
    deserialize_message(buf);
    snprintf(str, MAX_DATA, "%s/%s", m.source, m.data);        
    if ((authPtr = fopen("logininfo.txt", "r")) == NULL) {
	printf("Error opening file.");
	exit(1);
    }
    
    // check if user is already active
    int i;
    for (i = 0; i < MAX_USERS; i++) { 
        if (strcmp(clientList[i].id, m.source) == 0) {
            if (clientList[i].isActive) { // if user is already active, return false to deny the login request
                fclose(authPtr);
                sendServerMessage(LO_NAK, sockfd);
                return false;
            } else if (!clientList[i].isActive) {
                break;
            }
        }
    }

    
    // check if user's password is correct by checking checking logininfo.txt in Lab4 folder
    while (fgets(temp, 512, authPtr) != NULL) {
	if ((strstr(temp, str)) != NULL) {

            // edit clientList members
            clientList[i].isActive = true;
            clientList[i].sockfd = sockfd;
            sendServerMessage(LO_ACK, sockfd);
            fclose(authPtr);
            return true;
	}
    }
    sendServerMessage(LO_NAK, sockfd);
    return false;
    
}

// Send ACK or NAK to client
void sendServerMessage(int type, int sockfd) {
    char buf[BUF_SIZE];
    
    if (type == LO_ACK) {
        response.type = LO_ACK;
        strcpy(response.source, "server");
        strcpy(response.data, "");
        response.size = strlen(response.data);
    } else if (type == LO_NAK) {
        response.type = LO_NAK;
        strcpy(response.source, "server");
        strcpy(response.data, "User is already active!");
        response.size = strlen(response.data);
    } else if (type == JN_ACK) {
        response.type = JN_ACK;
        strcpy(response.source, "server");
        strcpy(response.data, m.data);
        response.size = strlen(response.data);
    } else if (type == JN_NAK) {
        response.type = JN_NAK;
        strcpy(response.source, "server");
        snprintf(response.data, MAX_DATA, "%s, %s", m.data, "error joining session!");
        response.size = strlen(response.data);
    } else if (type == NS_ACK) {
        response.type = NS_ACK;
        strcpy(response.source, "server");
        strcpy(response.data, "");
        response.size = strlen(response.data);
    } else if (type == QU_ACK) {
        response.type = QU_ACK;
        strcpy(response.source, "server");
        char temp[MAX_NAME] = "List of active users:\n";
        
        //loop through all users and check if they are active
        //if user is active then add them to the message that list all active users
        for (int i = 0; i < MAX_USERS; i++) {
            if (clientList[i].isActive) {
                strcat(temp, clientList[i].id);
                strcat(temp, "\n");
            }
        }
        
        strcat(temp, "List of active sessions:");
        
        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (sessionList[i].numUsers != 0) {
                strcat(temp, "\n");
                strcat(temp, sessionList[i].sessionID);
            }
        }
        
        strcpy(response.data, temp);
        response.size = strlen(response.data)+1;
    }
    serialize_server_message(buf);
    //send message to client
    if (send(sockfd, buf, BUF_SIZE,  0) == -1) {
        perror("send");
    }
    
}

void logOutUser(int sockfd, int clientIndex) {
    printf("%s has logged out\n", clientList[clientIndex].id);
    leaveSession(clientIndex); //should leave any sessions that user is connected to first
    
    FD_CLR(sockfd, &master); // remove from master set 
    close(sockfd); // close connection to socket
    
    //change client members back to default values
    clientList[clientIndex].isActive = false;
    clientList[clientIndex].sockfd = -1;
}


// create new session
// REFER TO session STRUCT IN utils.h
void createSession(int clientIndex, char* sessionID) {
    printf("%s is creating session named %s\n", clientList[clientIndex].id, sessionID);
    if (doesSessionExist(sessionID) != -1) {
        sendServerMessage(JN_NAK, clientList[clientIndex].sockfd);
        printf("cannot create session because it already exist!");
        return;
    }  
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        //printf("i: %d\n", i);
        if (strcmp(sessionList[i].sessionID, "") == 0) {
            printf("found empty session: %d\n", i);
            // Add session to sessionList and add client to it
            strcpy(sessionList[i].sessionID, sessionID);
            sessionList[i].clientsInSession[0] = &clientList[clientIndex];
            sessionList[i].numUsers = 1;
            clientList[clientIndex].currentSession = i;
            sendServerMessage(NS_ACK, clientList[clientIndex].sockfd);
            return;
        }
    }
}

// add client to requested session
// REFER TO session STRUCT IN utils.h
void joinSession(int clientIndex, char* sessionID) {
    int sessionIndex = doesSessionExist(sessionID);
    
    if (clientList[clientIndex].currentSession != -1) { //client is already in session
        sendServerMessage(JN_NAK, clientList[clientIndex].sockfd);
        return;
    }
    if (sessionIndex != -1) {
        for (int j = 0; j < MAX_USERS; j++) {
            if (sessionList[sessionIndex].clientsInSession[j] == NULL) {
                //each element insession array will point to an client in the client array
                sessionList[sessionIndex].clientsInSession[j] = &clientList[clientIndex]; 
                ++sessionList[sessionIndex].numUsers;
                clientList[clientIndex].currentSession = sessionIndex;
                sendServerMessage(JN_ACK, clientList[clientIndex].sockfd);
                printf("%s is joining session %s\n", clientList[clientIndex].id, sessionID);
                return;
            }
        }

    } else if (sessionIndex == -1) {
        sendServerMessage(JN_NAK, clientList[clientIndex].sockfd);
        return; //SESSION NOT FOUND
    }
    
        
}

// leave A session that the client is in
// REFER TO session STRUCT IN utils.h
void leaveSession(int clientIndex) {
    int size;
    char* sessionID = sessionList[clientList[clientIndex].currentSession].sessionID;
    int sessionIndex = doesSessionExist(sessionID);
    if (sessionIndex != -1) {
        for (int j = 0; j < MAX_USERS; j++) {
            if (sessionList[sessionIndex].clientsInSession[j] != NULL && strcmp(sessionList[sessionIndex].clientsInSession[j]->id, clientList[clientIndex].id) == 0) {
                printf("leaving session...\n");
                sessionList[sessionIndex].clientsInSession[j] = NULL;
                --sessionList[sessionIndex].numUsers;
                clientList[clientIndex].currentSession = -1;
                printf("numUsers: %d\n", sessionList[sessionIndex].numUsers);
                if (sessionList[sessionIndex].numUsers <= 0) {
                    printf("sessionID before: %s\n", sessionList[sessionIndex].sessionID);
                    strcpy(sessionList[sessionIndex].sessionID, "");
                    printf("sessionID: %s\n", sessionList[sessionIndex].sessionID);
                }
                return;
            }
        }


    } else { //session not found
        printf("Cannot leave session that does not exist!\n");
        return;
        
    }
        
}

//check if session with sessionID already exist, if so return the sessionList index of that session
// REFER TO session STRUCT IN utils.h
int doesSessionExist(char* sessionID) {
    printf("checking if session named %s exist\n", sessionID);
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (strcmp(sessionList[i].sessionID, sessionID) == 0) {
            return i;
        }
    }
    // NOT FOUND
    return -1;
  
}

// sends message from client to all of the clients in the same session
void sendClientMessage(int clientIndex, char* message) {
    int temp_sockfd;
    char buf[BUF_SIZE];
    response.type = MESSAGE;
    
    // prepare message data
    int message_size = sizeof m.data;
    memcpy(response.data, m.data, message_size);
    response.size = m.size;
    strcpy(response.source, "server");
    serialize_server_message(buf);
    
    char sessionID[MAX_SESSION_ID];
    int sessionListIndex = doesSessionExist(sessionList[clientList[clientIndex].currentSession].sessionID); // check if session exist and if it does then save it's sessionIndex
    strcpy(sessionID, sessionList[sessionListIndex].sessionID); // save the sessionID from the user that sent the message
    
    if (sessionListIndex != -1) {
        for (int j = 0; j < MAX_USERS; j++) { // loop through each user in the session and send them the message.
            // do not send if the client ID matches the user that originally sent the message 
            if (clientList[j].currentSession == sessionListIndex) {
                temp_sockfd = clientList[j].sockfd;
                if (strcmp(clientList[j].id, clientList[clientIndex].id) != 0) {
                    if (send(temp_sockfd, buf, BUF_SIZE, 0) == -1) 
                        printf("Error sending client message");
                } else printf("Sent to %s, sockfd: %d\n", clientList[j].id, temp_sockfd);

            }

        }
    } else {
        printf("Client is not in a session!");
        return;
    }
}

void listUsers(int clientIndex) {
    sendServerMessage(QU_ACK, clientList[clientIndex].sockfd);
}



void processCommand(char* buf, int sockfd, int clientIndex) { 
    deserialize_message(buf);
    if (m.type > 12) m.type = m.type/10;
    printf("processing command: %d\n", m.type);
    
    if (m.type == EXIT) {
        logOutUser(sockfd, clientIndex);
    } else if (m.type == JOIN) {
        joinSession(clientIndex, m.data);
    } else if (m.type == LEAVE_SESS) {
        leaveSession(clientIndex);
    } else if (m.type == NEW_SESS) {
        createSession(clientIndex, m.data);
    } else if (m.type == QUERY) {
        listUsers(clientIndex);
    } else if (m.type == MESSAGE) {
        sendClientMessage(clientIndex, m.data);
    }
    else
        sendServerMessage(m.type, sockfd);
    
}
    