CC=gcc
CFLAGS=-I.
LFLAGS= -L../lib
LIBS= -lwiringPi
INCLUDES=-I../include -I ./ 
ODIR=obj
TARGET = temp_control
MAIN= temp_control
prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
export prefix
export exec_prefix
export bindir
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
	rm -f ./*.o *~ $(MAIN)
install: depend all
	install -d $(bindir)
	install -m 0755 $(TARGET)  $(bindir)
uninstall:
	rm -f $(bindir)/$(TARGET)
