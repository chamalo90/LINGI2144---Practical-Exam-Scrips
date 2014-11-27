CC=gcc
CFLAGS=

LIBS=-lnfc -lcrypto
ALL=hello_desfire reset_desfire select_app create_app read_file change_key

all: hello_desfire reset_desfire select_app create_app read_file change_key

change_key: change_key.c
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

hello_desfire: hello_desfire.c
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

reset_desfire: reset_desfire.c
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

select_app: select_app.c
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

create_app: create_app.c
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

read_file: read_file.c
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ALL)

