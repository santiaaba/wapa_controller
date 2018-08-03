#!/bin/bash

gcc controller.c config.o parce.o db.o structs.o -L/usr/lib64/mysql/ -lmysqlclient -o controller
