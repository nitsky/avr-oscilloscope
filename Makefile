DEVICE     = atmega328p
CLOCK      = 16000000
FUSES      = -U hfuse:w:0xDE:m -U lfuse:w:0xFF:m
PROGRAMMER = -c usbasp -P usb

CFLAGS=-mmcu=$(DEVICE) -Wall -Os -DF_CPU=$(CLOCK)

main.hex: main.out
	avr-objcopy -O ihex main.out main.c.hex;\
	avr-size --mcu=$(DEVICE) --format=avr main.out

main.out: main.c
	avr-gcc $(CFLAGS) -I./ -o main.out main.c

flash: main.hex
	avrdude -p $(DEVICE) $(PROGRAMMER) -U flash:w:main.c.hex
