#!/bin/bash
rm config.o
rm parce.o
rm db.o
rm structs.o
gcc -c parce.c
gcc -c structs.c
gcc -c db.c
gcc -c config.c
gcc controller.c config.o parce.o db.o structs.o -L/usr/lib64/mysql/ -lmysqlclient -o controller
