#!/bin/bash

url="apinetffice.fibercorp.com.ar"

if [ $# -ne 2 ]; then
	echo "Debe utilizar $0 <archivo de cuentas> <plan>"
	exit 1;
fi

for i in `cat $1` ; do
	echo "curl -X POST \"https://$url/os/mobile/fbcChangeUser?plan=$2&email=$i\" -H 'Content-Type: application/x-www-form-urlencoded' -H 'fibercorp-sid: 6f27c339-47ca-435a-8677-b9d6db2d299a'";
	curl -X POST "https://$url/os/mobile/fbcChangeUser?plan=$2&email=$i" -H 'Content-Type: application/x-www-form-urlencoded' -H 'fibercorp-sid: 6f27c339-47ca-435a-8677-b9d6db2d299a';
done
