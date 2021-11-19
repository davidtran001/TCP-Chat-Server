#include "utils.h"

// message
struct message m;
char clientID[MAX_NAME];

// converts a message to a string of format "type:size:source:data"
void serialize_message(char* buf) {
    printf("sent %u:%u:%s:\n", m.type, m.size, m.source);
    snprintf(buf, BUF_SIZE, "%u:%u:%s:", m.type, m.size, m.source);
    memcpy(&(buf[strlen(buf)]), &(m.data), m.size);
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

bool send_and_receive(int sockfd, int ack, int nak) {
    char buf[BUF_SIZE];
    
    strcpy(m.source, clientID);
    serialize_message(buf);
        
    // sends message to the server
    if (send(sockfd, buf, BUF_SIZE, 0) == -1) {
        perror("Error sending login message\n");
        exit(1);
    }

    struct sockaddr_storage src_addr;
    socklen_t addrlen_storage = sizeof(struct sockaddr_storage);

    // receives message from socket
    if (recv(sockfd, buf, BUF_SIZE, 0) == -1) {
        printf("Error receiving confirmation in send and receive\n");
        exit(1);
    }
//        buf[BUF_SIZE -1] = '\0';
    deserialize_message(buf);

    // returns if an ACK is received
    if(m.type == ack)
        return true;
    else if (m.type != nak)
        printf("Received unexpected message type!\n");

    return false;
}

// sends a login message to the specified server
bool login(int* sockfd, char** inputs, fd_set* master) {
    // creates the login message and gets the specified server
    strcpy(clientID, inputs[1]);
    strcpy(m.source, clientID);
    strcpy(m.data, inputs[2]);
    m.type = LOGIN;
    m.size = strlen(m.data) + 1;
    
    // address of server
    struct sockaddr_in serv_addr;
    socklen_t addrlen_in = sizeof(struct sockaddr_in);
    
    // sets the serv_addr to the specified server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(inputs[4]));
    inet_aton(inputs[3], &serv_addr.sin_addr);

    // connects to the specified server
    if (connect(*sockfd, (struct sockaddr*) &serv_addr, addrlen_in) == -1) {
        perror("Error connecting\n");
        exit(1);
    }
    
    if (send_and_receive(*sockfd, LO_ACK, LO_NAK)) {
        // adds the socket to master fd_set
        FD_SET(*sockfd, master);
        return true;
    }
    else {
        printf("Wrong login information!\n");
        close(*sockfd); // close connection to socket
        *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    }
    
    return false;
}

// logs the current user out
void logout(int* sockfd, fd_set* master) {
    strcpy(m.source, clientID);
    m.type = EXIT;
    m.size = 0;
    
    char buf[BUF_SIZE];
    serialize_message(buf);

    // sends message to the server
    if (send(*sockfd, buf, BUF_SIZE, 0) == -1) {
        perror("Error sending logout message\n");
        exit(1);
    }  
    
    // removes the socket to master fd_set
    FD_CLR(*sockfd, master);
    close(*sockfd); // close connection to socket
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
}

// joins the current user to the specified session
void join_session(int sockfd, char** inputs) {
    // creates the join message
    strcpy(m.data, inputs[1]);
    m.type = JOIN;
    m.size = strlen(m.data) + 1;

    send_and_receive(sockfd, JN_ACK, JN_NAK);
}

// removes the current user from their current session
void leave_session(int sockfd) {
    m.type = LEAVE_SESS;
    m.size = 0;
            
    char buf[BUF_SIZE];
    
    strcpy(m.source, clientID);
    serialize_message(buf);

    // sends message to the server
    if (send(sockfd, buf, BUF_SIZE, 0) == -1) {
        perror("Error sending leave message\n");
        exit(1);
    }  
}

// creates a session for the current user
void create_session(int sockfd, char** inputs) {
    //creates the create session message
    strcpy(m.data, inputs[1]);
    m.type = NEW_SESS;
    m.size = strlen(m.data) + 1;

    send_and_receive(sockfd, NS_ACK, NS_ACK);
}

// lists all of the sessions
void list(int sockfd) {
    m.type = QUERY;
    m.size = 0;
            
    char buf[BUF_SIZE];
    
    strcpy(m.source, clientID);
    serialize_message(buf);
        
    // sends message to the server
    if (send(sockfd, buf, BUF_SIZE, 0) == -1) {
        perror("Error sending login message\n");
        exit(1);
    }

    struct sockaddr_storage src_addr;
    socklen_t addrlen_storage = sizeof(struct sockaddr_storage);

    // receives message from socket
    if (recv(sockfd, buf, BUF_SIZE, 0) == -1) {
        printf("Error receiving confirmation in send and receive\n");
        exit(1);
    }

    deserialize_message(buf);

    // returns if an ACK is received
    if(m.type == QU_ACK)
        printf("%s\n", m.data);
    else
        printf("Received unexpected message type!\n");
}

// sends a message to the current session
void send_message(int sockfd, char* data) {
    uint8_t num_messages = (strlen(data) + 1)/DATA_LENGTH;
    uint8_t message_no = 0;
    int data_index = 0;
    
    strcpy(m.source, clientID);
    
    // sends packets to server
    for (int i = 0; i <= num_messages; ++i) {
        // first 2 bytes of data are # of packets and packet number
        m.data[0] = num_messages;
        m.data[1] = message_no;
        message_no = i + 1;
        
        int message_index = 2;
        
        // converts the next bytes of data into a packet
        while (message_index != MAX_DATA && data_index <= strlen(data)) {
            m.data[message_index] = data[data_index];
            ++message_index;
            ++data_index;
        }
        
        m.type = MESSAGE;
        m.size = message_index;
        
        char buf[BUF_SIZE];
        serialize_message(buf);

        // sends message to the server
        if (send(sockfd, buf, BUF_SIZE, 0) == -1) {
            perror("Error sending message\n");
            exit(1);
        }
    }
}

int main(int argc, char** argv) {
    bool logged_in = false;
    
    //gets socket fd
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // creates and zeroes the two fd_sets
    fd_set master, read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    
    // adds the command line to master fd_set
    FD_SET(0, &master);
    
    size_t size = DATA_LENGTH*MAX_MESSAGES;
    char* in = malloc(sizeof(char) * size);
    char* temp = malloc(sizeof(char) * size);
    
    // continuously asks for commands until the user quits
    while(true) {
        read_fds = master;
                
        if (select(sockfd+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("Problem selecting fd\n");
            exit(1);
        }
        
        if (FD_ISSET(0, &read_fds)) {
            char* inputs[8];
            
            getline(&in, &size, stdin);
            in[strlen(in) - 1] = '\0';
            strncpy(temp, in, size);
            
            int i = 0;
            inputs[0] = strtok(in, " ");
            
            while (inputs[i] != NULL) {
                ++i;
                inputs[i] = strtok(NULL, " ");
            }
            
            // determines what to do based on the command
            if (strcmp(inputs[0], "login") == 0) {
                // checks if a user is already logged in
                if (!logged_in)
                    logged_in = login(&sockfd, inputs, &master);
                else
                    printf("Already logged in! Please logout before trying to login again!\n");
            }
            else if (strcmp(inputs[0], "logout") == 0) {
                // checks if a user is currently logged in
                if (logged_in) {
                    logout(&sockfd, &master);
                    logged_in = false;
                }
                else
                    printf("Not logged in!\n");
            }
            else if (strcmp(inputs[0], "joinsession") == 0) {
                // checks if a user is currently logged in
                if (logged_in)
                    join_session(sockfd, inputs);
                else
                    printf("Not logged in!\n");
            }
            else if (strcmp(inputs[0], "leavesession") == 0) {
                // checks if a user is currently logged in
                if (logged_in)
                    leave_session(sockfd);
                else
                    printf("Not logged in!\n");
            }
            else if (strcmp(inputs[0], "createsession") == 0) {
                // checks if a user is currently logged in
                if (logged_in)
                    create_session(sockfd, inputs);
                else
                    printf("Not logged in!\n");
            }
            else if (strcmp(inputs[0], "list") == 0) {
                // checks if a user is currently logged in
                if (logged_in) 
                    list(sockfd);
                else
                    printf("Not logged in!\n");
            }
            else if (strcmp(inputs[0], "quit") == 0) {
                if (logged_in)
                    logout(&sockfd, &master);
                return 0;
            }
            else {
                // checks if a user is currently logged in
                if (logged_in)
                    send_message(sockfd, temp);
                else
                    printf("Not logged in!\n");
            }
        }
        else if (FD_ISSET(sockfd, &read_fds)) {
            char buf[BUF_SIZE];
            int recv_sz;
            
            // receives message from socket
            if ((recv_sz = recv(sockfd, buf, BUF_SIZE, 0)) == -1) {
                printf("Error receiving confirmation\n");
                exit(1);
            }
            
            if (recv_sz != 0) {
                deserialize_message(buf);
                
                // returns if an ACK is received
                if(m.type == MESSAGE) {
                    uint8_t num_messages = m.data[0];

                    for (int i = 0; i <= num_messages; ++i)
                        memcpy(&(buf[DATA_LENGTH*i]), &(m.data[2]), m.size);

                    printf("%s\n", buf);
                }
                else
                    perror("Received unexpected message type!\n");
            }
        }
    }
    
    return 1;
}

