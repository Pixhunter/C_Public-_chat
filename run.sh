#!/bin/bash
gcc main.c sql_data.c client.c -lsqlite3 -o app 
./app 