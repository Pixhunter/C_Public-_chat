struct Client {
    int socket;
    int port;
    char* ip;
    char* user_name;
    char* password;
    int checked_pass;
    int flag;
};

void printClient(struct Client client);