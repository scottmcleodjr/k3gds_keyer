FILE = k3gds_keyer.ino
PORT = /dev/ttyUSB0
FQBN = arduino:avr:nano

.PHONY: all load compile

all: load

load:
	@arduino-cli upload --verbose -p $(PORT) --fqbn $(FQBN) $(FILE)

compile:
	@arduino-cli compile --warnings all --fqbn $(FQBN) $(FILE)
	
