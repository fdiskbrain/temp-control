CC=gcc
CFLAGS=-I.
LFLAGS= -L../lib
LIBS= -lwiringPi
INCLUDES=-I../include -I ./ 
ODIR=obj
TARGET = temp_control
MAIN= bin/temp_control
#SRCS = $(ls *.c)
SRCS = ssd1306_i2c.c  temp_control.c
#SRCS = fan.c  fan_temp.c  oled.c  rgb.c  rgb_effect.c  rgb_temp.c  ssd1306_i2c.c  temp_control.c
OBJS= $(SRCS:.c=.o)
.PHONY: depend clean
all: $(MAIN)
	@echo $(SRCS)
#%.o: %.c $(LIBS)
#	$(CC) -c -o $@ $< $(CFLAGS)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@
#temp_control: temp_control.o ssd1306_i2c.o
#	$(CC) -o temp_control temp_control.o ssd1306_i2c.o $(LIBS)
clean:
	rm -f ./*.o *~
