#!/bin/bash
rm config.o
rm parce.o
rm db.o
rm structs.o
rm server.o
rm json.o
rm task.o
rm dictionary.o
rm string_element.o
rm logs.o
rm valid.o

gcc -c logs.c
gcc -c parce.c
gcc -c structs.c
gcc -c db.c
gcc -c config.c
gcc -c server.c
gcc -c json.c
gcc -c task.c
gcc -c dictionary.c
gcc -c string_element.c
gcc -c valid.c

gcc controller.c -lpthread -lmicrohttpd valid.o dictionary.o logs.o string_element.o config.o task.o json.o server.o parce.o db.o structs.o -L/usr/lib64/mysql/ -lmysqlclient -o controller
