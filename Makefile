CC      ?= gcc
CFLAGS  ?= -std=c11 -lc -Wall -Wextra -O2 -Iinclude
CFLAGS  += $(shell mysql_config --cflags)

LDLIBS  := -lexpat $(shell mysql_config --libs) -lcurl -lcrypto

TARGET  := mon1mm
OBJS    := src/main.o src/config.o src/log.o src/udp.o src/xmlparse.o src/db.o src/lotw.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# header dependencies
main.o:     main.c config.h log.h udp.h xmlparse.h db.h
config.o:   config.c config.h
log.o:      log.c log.h
udp.o:      udp.c udp.h log.h
xmlparse.o: xmlparse.c xmlparse.h log.h
db.o:       db.c db.h config.h xmlparse.h log.h
lotw.o:		lotw.c lotw.h xmlparse.h log.h

clean:
	rm -f $(TARGET) $(OBJS)
