# Make file for mb2http - Modbus to HTTP converter
TARGET=mb2http
CC=gcc
CFLAGS=-I/usr/local/include -L/usr/local/lib -g -std=gnu99

.PHONY: default all clean check cron

default: $(TARGET)
all: default

SRC_C=mb2http.c \
     parseargs.c \
     main.c
	 
HDR=mb2http.h \
    parseargs.h \
    main.h \
    typedefs.h 

LIBS=-lpthread -lmodbus -lcurl

DEPS = $(patsubst %,$(IDIR)/%,$(HDR))
OBJ = $(patsubst %.c,%.o,$(SRC_C))

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	
check:
	@echo '#############################'
	@echo ' SRC_C  = $(SRC_C)           '
	@echo ' OBJ    = $(OBJ)             '
	@echo ' HDR    = $(HDR)             '
	@echo ' DEPS   = $(DEPS)            '
	@echo '#############################'
	
cronjobstart:
	crontab -u ${USER} cronjob.txt

cronjobstop:
	crontab -u ${USER} -r

clean:
	rm -f *.o $(TARGET) 
