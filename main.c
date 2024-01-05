#include <stdio.h>  
#include <string.h>    
#include <stdlib.h>  
// Include for errors
#include <errno.h>  

#include <unistd.h>  
#include <arpa/inet.h>    
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
//FD_SET, FD_ISSET, FD_ZERO macros  
#include <sys/time.h> 

// Include SQL 
#include <sqlite3.h> 

// Custom files to deligate functions
#include "sql_data.h"
#include "client.h"
     
#define FALSE  0  
#define PORT 7030
#define CLIENT_SIZE 5
#define BUFFER_SIZE 1024

// wellcome message from sever to user 
#define WELLCOME_MESSAGE "Minecraft Server: Wellcome to Minecraft Server!\nMinecraft Server: Any message you write will be your login, be carefull, you have only 1 try!\nMinecraft Server: Login must have max 32 letters and must contains letters or numbers only!\nMinecraft Server: One mistake and you will be disconected!\n"  

// fuction to clear buffer - to predict collisions from socket input data
void clearBuffer(char* buffer, int size) {
    printf("Clear buffer=%s\n", buffer);
    size--;
    while (size >= 0) {
        buffer[size--] = '\0';
    }
    printf("Clear finisshed buffer=%s\n", buffer);
}

// function to check user input login
// expect only letters and digits
int check_login(char* login, int read_size) {
    printf("Check user login=%s\n", login);
    int i = 0;
    while (read_size - 1 > i && i < 32) { 
        printf("Check char login[%d]=%c\n", i, login[i]);
        if (login[i] >= 'a' && login[i] <= 'z') {
             ++i;
             continue;
        } 
        if (login[i] >= 'A' && login[i] <= 'Z') {
             ++i;
             continue;
        } 
        if (login[i] >= '0' && login[i] <= '9') {
             ++i;
             continue;
        } 
        return 1;
    } 
    login[i] = '\0';
    return 0;
}

// function to kill ALL processes running on expected server port
void kill_all_processes_on_server_port() {
    printf("Try to kill port=%d\n", PORT);
    char command_to_run[21];
    //strcpy(command_to_run, "npx kill-port "); 
    sprintf(command_to_run, "fuser -n tcp -k %d", PORT);
    system(command_to_run);
    printf("port=%d successfully killed\n", PORT);
}

// check if someone online with same username
int check_user_online(struct Client* client, char* user_name) {
    printf("check if user=%s online\n", user_name);
    for (int i = 0; i < CLIENT_SIZE; i++) {
        printf("check new user=%s ? old user=%s i=%d\n", user_name, client[i].user_name, i);
        if ( client[i].user_name == NULL) {
            continue;
        } 
        if (strlen(user_name) != strlen(client[i].user_name)) {
            printf("check size new user=%ld old user=%ld\n", strlen(user_name), strlen(client[i].user_name));
            continue;
        }

        int j = 0;
        while (client[i].user_name[j] == user_name[j]) {
            if (client[i].user_name[j] == '\0') return 1;
            j++;
        }
        return 0;
    }
}

// function to set values of one client to default
void clean_client(struct Client* client) {
        client->ip = NULL;
        client->port = 0;
        client->socket = 0;
        client->user_name = NULL;
        client->password = NULL;
        client->checked_pass = 0;
        client->flag = 3;
}

// function to disconnect user
void attempt_over(int socket, struct sockaddr_in socket_address, int address_len, int sd, struct Client* client) {
    // prepare message to user
    char attempts_message[36] = "Minecraft Server: attempts are over\n";
    // send message to user
    send(socket , attempts_message, 36, 0);

    getpeername(sd, (struct sockaddr*) &socket_address, (socklen_t*) &address_len);   

    printf("Host disconnected , ip=%s , port=%d \n", inet_ntoa(socket_address.sin_addr) , ntohs(socket_address.sin_port));   

    // set client to default          
    clean_client(client);
    // close socket and free space in list
    close(sd); 
}

// function to check user wants to disconnect from server
int check_exit(char* buffer) {
    printf("check buffer=%s if user wants to disconnect\n", buffer);
    char q[4] = "quit";
    // check length
    if (strlen(buffer) != 4) return 0;
    
    // check each letter
    int i = 0;
    while (i < 4 && buffer[i] == q[i]) i++;

    // check that each char was checked
    if (i != 4) return 0;
    return 1;
}

int main(int argc , char *argv[]) {   
    // kill processes on PORT if buisy
    kill_all_processes_on_server_port();
    // open or create database 
    sqlite3* data_base = open_database();

    int opt = 1;   
    int max_clients = CLIENT_SIZE;
    char* user_ip;
    int master_socket, address_len, new_socket, activity, i, read_size, sd, max_sd;  

    struct sockaddr_in socket_address;   
    struct Client client[CLIENT_SIZE];

    // init all clients with default values
    for (int i = 0; i < CLIENT_SIZE; ++i) {
        client[i].ip = NULL;
        client[i].port = 0;
        client[i].socket = 0;
        client[i].user_name = NULL;
        client[i].password = NULL;
        client[i].checked_pass = 0;
        client[i].flag = 3;
    }

    // init buffer      
    char buffer[BUFFER_SIZE];
         
    // set of socket descriptors  
    fd_set readfds;   
     
    // initialise all client[i].socket to 0 so not checked  
    for (i = 0; i < max_clients; i++) {   
        client[i].socket = 0;   
    }   
         
    // create a master socket  
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {   
        perror("socket creation failed");   
        exit(EXIT_FAILURE);   
    }   
     
    // set master socket to allow multiple connections
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char*) &opt, sizeof(opt)) < 0 ) {   
        perror("set socket opt");   
        exit(EXIT_FAILURE);   
    }   
     
    // create type of socket
    socket_address.sin_family = AF_INET;   
    socket_address.sin_addr.s_addr = INADDR_ANY;   
    socket_address.sin_port = htons(PORT);   
         
    // bind the socket to port
    if (bind(master_socket, (struct sockaddr*) &socket_address, sizeof(socket_address)) < 0) {   
        perror("bind socket failed");   
        exit(EXIT_FAILURE);   
    }   
    printf("Listener on port=%d\n", PORT);   
         
    // try to specify maximum of CLIENT_SIZE pending connections for the master socket  
    if (listen(master_socket, CLIENT_SIZE) < 0) {   
        perror("listen socket error");   
        exit(EXIT_FAILURE);   
    }   
         
    // save size of socket address  
    address_len = sizeof(socket_address);   
    printf("Waiting for any connections ...");   
         
    while(1) {   
        // clear the socket   
        FD_ZERO(&readfds);   
        // add master socket   
        FD_SET(master_socket, &readfds); 
        // set max sd  
        max_sd = master_socket;   
             
        // add child sockets 
        for ( i = 0 ; i < max_clients ; i++) {   
            // socket descriptor  
            sd = client[i].socket;   
            // add socket to read list  
            if(sd > 0) FD_SET(sd , &readfds);   
            // highest file descriptor number  
            if(sd > max_sd) max_sd = sd;   
        }   
     
        // wait for any activity on one of the sockets and set timeout is NULL
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {   
            printf("select error");   
        }   
             
        // process socket incoming connection 
        if (FD_ISSET(master_socket, &readfds)) {   
            if ((new_socket = accept(master_socket, (struct sockaddr*)&socket_address, (socklen_t*)&address_len)) < 0) {   
                perror("error process incoming connection");   
                exit(EXIT_FAILURE);   
            }   
            user_ip = inet_ntoa(socket_address.sin_addr); 
            printf("New connection , i=%d socket=%d, ip=%s, port=%d\n", i, new_socket , user_ip, ntohs(socket_address.sin_port));   
           
            // send hello message to new connection 
            if( send(new_socket, WELLCOME_MESSAGE, strlen(WELLCOME_MESSAGE), 0) != strlen(WELLCOME_MESSAGE) ) {   
                perror("send hello message error");   
            }   
                 
            printf("Welcome message sent successfully to user=%d\n", new_socket);   
                 
            // add new socket to array if empty position exists
            for (i = 0; i < max_clients; i++) {   
                if( client[i].socket == 0 ) {   
                    client[i].socket = new_socket;   
                    printf("Adding to list of sockets as i=%d new_socket=%d\n" , i, client[i].socket); 
                    break;   
                }   
            }   
        }   
        
        printf("Start IO operation if exist\n");
        // IO operations on other sockets if exist
        for (i = 0; i < max_clients; i++) {   
            sd = client[i].socket;   
            printf("Start IO operation for socket=%d and user_name=%s\n", client[i].socket, client[i].user_name); 
            if (FD_ISSET( sd , &readfds)) {   
                // check if socket was closing
                if ((read_size = read(sd , buffer, BUFFER_SIZE)) == 0) {   
                    // user disconnected
                    getpeername(sd , (struct sockaddr*) &socket_address, (socklen_t*) &address_len);   
                    printf("Host disconnected , ip=%s , port=%d \n", inet_ntoa(socket_address.sin_addr) , ntohs(socket_address.sin_port));   
                    
                    // close socket and free space in list
                    close(sd);   
                
                // if cliend exist and send message
                } else { 
                    // replace \n in message from client
                    buffer[read_size - 1] = '\0';

                    // if user want to disconnect
                    if (check_exit(buffer)) {
                        printf("user_name=%s wants to disconnect\n", client[i].user_name);
                        attempt_over(client[i].socket, socket_address, address_len, sd, &client[i]);
                        continue;
                    }
                    

                    printf("Echo message from socket=%d user_name=%s buffer_size=%d buffer=%s\n", client[i].socket, client[i].user_name, read_size, buffer);

                    if (client[i].user_name == NULL) {
                        printf("user_name is null for socket=%d flag=%d\n", client[i].socket, client[i].flag); 
                        // check if user_name is correct
                        if (check_login(buffer, read_size)) {
                            printf("user_name is not correct for socket=%d\n", client[i].socket); 
                            clearBuffer(buffer, read_size);

                            // send error message to client
                            printf("retries=%d for socket=%d\n", client[i].flag, client[i].socket);
                            char login_err__message[68] = "Minecraft Server: login is not correct, try again (times to try=";
                            sprintf(login_err__message, "%s%d", login_err__message, client[i].flag);
                            strcat(login_err__message, ")\n");
                            send(client[i].socket, login_err__message, 67, 0);

                            // dec the counter
                            client[i].flag--;
                            // if counter is 0 - disconnect user
                            if (client[i].flag < 1) {
                                attempt_over(client[i].socket, socket_address, address_len, sd, &client[i]);
                            }
                            continue;
                        } else {
                            client[i].flag = 3;
                        }

                        printf("user_name=%s is correct for i=%d socket=%d buffer=%s user_ip=%s\n", client[i].user_name, i, client[i].socket, buffer, user_ip); 

                        // check if user with same name is online
                        if (check_user_online(client, buffer)) {
                            printf("same user_name=%s online - disconnect\n", buffer);
                            attempt_over(client[i].socket, socket_address, address_len, sd, &client[i]);
                            continue; 
                        }

                        // save user settings to structure
                        client[i].ip = user_ip;
                        client[i].port = ntohs(socket_address.sin_port); 
                        client[i].user_name = (char*)malloc(sizeof(char) * read_size);
                        strcpy(client[i].user_name, buffer); 

                        // try to find user in database
                        struct Client existing_client = find_client_by_username(buffer);
                        printClient(existing_client);
                        printf("SELECT RESULT: ip=%s password=%s user_name=%s\n", existing_client.ip, existing_client.password, existing_client.user_name);

                        // user with same name already exists
                        if (existing_client.user_name != NULL) {
                            printf("user_name=%s is already exist\n", client[i].user_name);
                            char password_exist_message[34] = "Minecraft Server: Enter password:\n";
                            send(client[i].socket , password_exist_message , 34 , 0);
                            client[i].password = existing_client.password;

                        // new user
                        } else {
                            printf("user=%s doesn't exist\n", client[i].user_name);
                            char _message[49] = "Minecraft Server: Enter new password to set it:\n";
                            send(client[i].socket, _message , strlen(_message) , 0); 
                            client[i].checked_pass = 1;
                        }

                        printf("user_name=%s for socket=%d ip=%s\n", client[i].user_name, client[i].socket, client[i].ip); 
                        clearBuffer(buffer, read_size);
                        continue;

                    // if no password present    
                    } else if (client[i].password == NULL) {
                        printf("Save new password=%s for user=%s\n", client[i].password, client[i].user_name);
                        client[i].password = (char*)malloc(sizeof(char) * read_size);
                        client[i].password = buffer;
                        client[i].checked_pass = 1; 

                        // insert new user in database 
                        insert_user(client[i].port, client[i].socket, client[i].ip, client[i].user_name, client[i].password); 
                        // select just for log to check all data in debug
                        find_client_by_username(client[i].user_name);

                        // send to user message
                        char password_message[85] = "Minecraft Server: Password saved..\nMinecraft Server: Connecting to Minecraft server\n";
                        send(client[i].socket, password_message , strlen(password_message), 0); 
                        continue;
                    // state for existing user 
                    } else if (client[i].checked_pass == 0) {
                        printf("Check Password\n");
                        printf("password=%s ? buffer=%s\n", client[i].password, buffer);
                        int j = 0;

                        // check same sizes
                        if (read_size - 1 != strlen(client[i].password)) {
                            printf("Password size is not correct, try again exp=%d act=%ld \n", read_size,  strlen(client[i].password));
                            // send error message to client
                            printf("retries=%d for socket=%d\n", client[i].flag, client[i].socket);
                            char login_err__message[71] = "Minecraft Server: password is not correct, try again (times to try=";
                            sprintf(login_err__message, "%s%d", login_err__message, client[i].flag);
                            strcat(login_err__message, ")\n");
                            send(client[i].socket, login_err__message, 71, 0);

                            // dec the counter
                            client[i].flag--;
                            // if counter is 0 - disconnect user
                            if (client[i].flag < 1) {
                                attempt_over(client[i].socket, socket_address, address_len, sd, &client[i]);
                            }
                            continue;
                        }

                        // check each char
                        while (j < read_size && client[i].password[j] == buffer[j] ) {
                            j++;
                        }
                        
                        // check that each char have checked
                        if (j < read_size) {
                            printf("Password is not correct, try again\n");
                            // send error message to client
                            printf("retries=%d for socket=%d\n", client[i].flag, client[i].socket);
                            char login_err__message[71] = "Minecraft Server: password is not correct, try again (times to try=";
                            sprintf(login_err__message, "%s%d", login_err__message, client[i].flag);
                            strcat(login_err__message, ")\n");
                            send(client[i].socket, login_err__message, 71, 0);

                            // dec the counter
                            client[i].flag--;
                            // if counter is 0 - disconnect user
                            if (client[i].flag < 1) {
                                attempt_over(client[i].socket, socket_address, address_len, sd, &client[i]);
                            }
                            continue;
                        } 

                        // password is ok
                        printf("Password correct, user=%s in chat\n", client[i].user_name);
                        // send ok message to user
                        client[i].checked_pass = 1; 
                        clearBuffer(buffer, read_size);
                        char password_message[91] = "Minecraft Server: Password is correct..\nMinecraft Server: Connecting to Minecraft server\n";
                        send(client[i].socket, password_message , strlen(password_message), 0); 

                        // send new user message to all online users
                        char hello_user[44 + strlen(client[i].user_name)];
                        strcpy(hello_user, "Minecraft Server: user=");
                        strcat(hello_user, client[i].user_name);
                        strcat(hello_user, " connected to server\n");
                        for (j = 0; j < max_clients; j++) { 
                            // do not send message to not full connected user
                            if (client[i].password == NULL) continue;

                            sd = client[j].socket;   
                            send(sd , hello_user , 44 + strlen(client[i].user_name), 0);   
                        }
                        continue;

                    // all other good cases
                    } else {
                        printf("Username is present i=%d user_name=%s for socket=%d\n", i, client[i].user_name, client[i].socket); 
                    }

                    // prepare user message
                    printf("Echo message from user_name=%s\n", client[i].user_name);
                    char str[read_size + strlen(client[i].user_name)];
                    strcpy(str, client[i].user_name);
                    strcat(str, ": ");
                    strcat(str, buffer);
                    strcat(str, "\n");

                    // send message to all users with author of message
                    printf("Echo message from buffer=%s\n", buffer); 
                    for (i = 0; i < max_clients; i++) { 
                        // do not send message to not full connected user
                        if (client[i].password == NULL) continue;

                        sd = client[i].socket;   
                        send(sd , str , strlen(str) , 0);   
                    }
                }  
                // anyway clear buffer
                clearBuffer(buffer, read_size);
            }   
        }  
    }   

    // close base at the end
    sqlite3_close(data_base);     
    return 0;   
}   
