CC=avr-gcc
MCU=atmega328p
LINKER=avr-objcopy
FLAGS=-mmcu=$(MCU) -std=c11 -Os
OBJS=main.o
MAIN=main.bin
# PROGRAMMER=usbasp
DEVICE=/dev/ttyUSB0
PROGRAMMER=arduino -P $(DEVICE)

.PHONY: clean

all: $(OBJS) bin hex size

bin: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(MAIN)

hex: $(MAIN)
	$(LINKER) -R eeprom -O ihex $(MAIN) main.hex

%.o: %.c
	$(CC) $(FLAGS) -c $^ -o $@

program: all
	 avrdude -c $(PROGRAMMER) -p $(MCU) -U flash:w:main.hex:i 


size: $(MAIN)
	avr-size -C --mcu=$(MCU) $(MAIN)

clean:
	rm -rf *.hex *.bin *.o
