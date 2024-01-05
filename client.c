#include <stdio.h>
#include <string.h>
#include <stdlib.h>  
#include "client.h"

void printClient(struct Client client) {
    printf("structure client: {\n");
    printf("           socket=%d\n", client.socket);
    printf("             port=%d\n", client.port);
    printf("               ip=%s\n", client.ip);
    printf("        user_name=%s\n", client.user_name);
    printf("         password=%s\n", client.password);
    printf("             flag=%d\n", client.flag);
    printf("}\n");   
}