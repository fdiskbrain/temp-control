CC=gcc
CFLAGS=-I.

hellomake: temp_control.o ssd1306_i2c.o
	$(CC) -o temp_control temp_control.o ssd1306_i2c.o -lwiringPi
clean:
	rm -f ./*.o
