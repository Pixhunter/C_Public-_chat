
#include <stdio.h>
#include <string.h>
#include <stdlib.h>  
#include <errno.h>
#include "sqlite3.h"

#include "client.h"

// name of database - you can use any because by default it will be created
#define DATABASE_NAME "userss.db"
#define CREATE_TABLE_USERS "CREATE TABLE IF NOT EXISTS USERS ("  \
                                "PORT           INT," \
                                "SOCKET         INT," \
                                "IP             CHAR(17)," \
                                "USER_NAME      CHAR(32) NOT NULL," \
                                "PASSWORD       CHAR(32)," \
                                "PRIMARY KEY    (USER_NAME));"
 
// callback function 
int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

// function to close database
void close_database(sqlite3* db) {
    int rc = sqlite3_close(db);
    if( rc != SQLITE_OK ){
        printf("Close database SQL error\n");  
    }
}

// function to open database
sqlite3* open_database() {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc = sqlite3_open(DATABASE_NAME, &db);
    
    // try to open database
    if(rc) {
        printf("Can't open database: %s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);   
    } 

    // try to create tabele if not exists
    char* create_table_sql;
    rc = sqlite3_exec(db, CREATE_TABLE_USERS, callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        printf("Open database SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(EXIT_FAILURE);   
    }

    printf("Table created successfully or already exists\n");
    return db; 
}

// function to insert database
int insert_user(int port, int socket, char* ip, char* username, char* password) {
    printf("Try to insert into table USERS data:\n     port=%d socket=%d ip=%s username=%s\n", port, socket, ip, username);
    // create arrays without pointers and copy data
    char *zErrMsg = 0;
    char ip_arr[15];
    strcpy(ip_arr, ip);
    char username_arr[32];
    strcpy(username_arr, username); 
    char password_arr[32];
    strcpy(password_arr, password);

    // try to open database
    sqlite3* db = open_database();

    // query to insert data
    char* query = sqlite3_mprintf("INSERT OR IGNORE INTO USERS (PORT, SOCKET, IP, USER_NAME, PASSWORD) VALUES (%d, %d, '%q', '%q', '%q');", port, socket, ip_arr, username_arr, password_arr);
    // try to execute insert
    int rc = sqlite3_exec(db, query, callback, 0, &zErrMsg);
   
    if( rc != SQLITE_OK ){
        printf("SQL insert error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        printf("Insert successfully\n");
    }
    
    // close database after insert
    close_database(db);
    return 1;
}

// function to get user by username
struct Client find_client_by_username(char* user_name) {
    struct Client client;
    // set default values to client
    client.socket = 0;
    client.port = 0;
    client.ip = NULL;
    client.user_name = NULL;
    client.password = NULL;
    
    // prepare sql statement
    printf("SELECT * FROM USERS WHERE USER_NAME='%s';\n", user_name);
    sqlite3_stmt *res;
    char* sql = "SELECT * FROM USERS WHERE USER_NAME=?;";
    char* zErrMsg = 0;
    // try to open database
    sqlite3* db = open_database();

    // prepare sql
    int rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    if (rc == SQLITE_OK) {
        //int idx = sqlite3_bind_parameter_index(res, "@user_name");
        sqlite3_bind_text(res, 1, user_name, strlen(user_name), SQLITE_STATIC);
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }
    
    // step like columns of table
    int step = sqlite3_step(res);
    
    // save data to client if select was successful
    printf("step=%d SQLITE_ROW=%d\n", step, SQLITE_ROW);
    if (step == SQLITE_ROW) {
        // print for debug
        printf("column[0] PORT=%d\n",  atoi(sqlite3_column_text(res, 0)));
        printf("column[1] SOCKET=%d\n",  atoi(sqlite3_column_text(res, 1)));
        printf("column[2] IP=%s\n", sqlite3_column_text(res, 2));
        printf("column[3] USER_NAME=%s\n", sqlite3_column_text(res, 3));
        printf("column[4] PASSWORD=%s\n", sqlite3_column_text(res, 4));

        // allocate memory
        client.user_name = (char*)malloc(sizeof(char) * strlen(sqlite3_column_text(res, 3)));
        client.password = (char*)malloc(sizeof(char) * 32);
        client.ip = (char*)malloc(sizeof(char) * 15);

        // copy data
        client.socket =  atoi(sqlite3_column_text(res, 0));
        client.port =  atoi(sqlite3_column_text(res, 1));
        client.ip = strdup(sqlite3_column_text(res, 2));
        client.user_name =  strdup(sqlite3_column_text(res, 3));
        client.password =  strdup(sqlite3_column_text(res, 4));
        printClient(client);
    } 
     
    //destroy the object to avoid resource leaks
    sqlite3_finalize(res);
    // close database after insert
    close_database(db);
    return client;
}