CC ?= cc
MINGW_CC ?= x86_64-w64-mingw32-gcc
CFLAGS ?= -O2 -Wall -Wextra

all: uinput_producer xinput_probe.exe regression_producer regression_probe.exe

uinput_producer: uinput_producer.c
	$(CC) $(CFLAGS) -o $@ $< -lm

xinput_probe.exe: xinput_probe.c
	$(MINGW_CC) $(CFLAGS) -o $@ $< -lxinput9_1_0

regression_producer: regression_producer.c
	$(CC) $(CFLAGS) -o $@ $< -lm

regression_probe.exe: regression_probe.c
	$(MINGW_CC) $(CFLAGS) -o $@ $< -lxinput9_1_0

clean:
	rm -f uinput_producer xinput_probe.exe regression_producer regression_probe.exe

.PHONY: all clean
