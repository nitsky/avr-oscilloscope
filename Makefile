DEVICE     = atmega328p
CLOCK      = 16000000
FUSES      = -U hfuse:w:0xDE:m -U lfuse:w:0xFF:m
PROGRAMMER = -c usbasp -P usb
OBJECTS    = main.o

AVRDUDE    = avrdude $(PROGRAMMER) -p $(DEVICE)
COMPILE    = avr-gcc -std=gnu99 -g -Wall -Winline -O3 -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

LINK_FLAGS = -lc -lm

%.o: %.c
	$(COMPILE) -c $< -o $@

%.o: %.s
	$(COMPILE) -S $< -o $@

cpp:
	$(COMPILE) -E main.c

flash:	all
	$(AVRDUDE) -U flash:w:main.hex:i

fuse:
	$(AVRDUDE) $(FUSES)

clean:
	rm -f main.hex main.elf $(OBJECTS)

main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS) $(LINK_FLAGS)

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex

disasm:	main.elf
	avr-objdump -d main.elf

%.lst: %.c
	{ echo '.psize 0' ; $(COMPILE) -S -g -o - $< ; } | avr-as -alhd -mmcu=$(DEVICE) -o /dev/null - > $@
