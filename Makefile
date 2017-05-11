NAME=wav2flash
CFLAGS=-O0
SOURCES=$(NAME).cpp
OBJECTS=$(SOURCES:.cpp=.o)
CC=c++
all: $(SOURCES) $(NAME)

$(NAME):$(OBJECTS)
	$(CC) $(OBJECTS) -o bin/$@

install: all
	cp bin/$(NAME) /usr/local/sbin
	
flash:
	
	flashrom --chip SST25VF020B --programmer buspirate_spi:dev=/dev/tty.usbmodem00000001,spispeed=1M -w ./roms/flashChip.rom --output logs/output.log -n -V -V

dump:
	flashrom --chip SST25VF020B --programmer buspirate_spi:dev=/dev/tty.usbmodem00000001,spispeed=1M -r ./roms/flashDump.rom --output logs/output.log -n -V -V

clean:
	rm *.o
	rm bin/*