int callback();
sqlite3* open_database();
int insert_user(int port, int socket, char* ip, char* username, char* password);
struct Client find_client_by_username(char* user_name);
