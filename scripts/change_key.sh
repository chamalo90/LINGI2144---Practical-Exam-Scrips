#!/bin/bash

in=$(echo -n $1 | sed -e :a -e 's/^.\{1,15\}$/&0/;ta')
in=$(echo "${in:0:8}" | rev)$(echo "${in:8:16}" | rev)

b1=$(echo -n "${in:4:4}"  | rev | xxd -p -u)
b2=$(echo -n "${in:0:4}"  | rev | xxd -p -u)
b3=$(echo -n "${in:12:4}" | rev | xxd -p -u)
b4=$(echo -n "${in:8:4}"  | rev | xxd -p -u)

./write_ultralight.sh UC 2C $b1
./write_ultralight.sh UCN 2D $b2
./write_ultralight.sh UCN 2E $b3
./write_ultralight.sh UCN 2F $b4
