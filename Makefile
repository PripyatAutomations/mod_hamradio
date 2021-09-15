#LOCALC_FLAGS=
MODNAME = mod_hamradio.so
MODOBJS += mod_hamradio.o radio.o radio_gpio.o #radio_rigctl.o
MODCFLAGS = -Wall -Werror
MODLDFLAGS = -lssl -lm -L/usr/local/lib -lgpiod
 
CC = gcc
CFLAGS = -fPIC -g -ggdb `pkg-config --cflags freeswitch` $(MODCFLAGS) -Wno-unused-variable
LDFLAGS = `pkg-config --libs freeswitch` $(MODLDFLAGS)
 
.PHONY: all
all: $(MODNAME)
 
$(MODNAME): $(MODOBJS)
	@echo "[LD] $@"
	@$(CC) -shared -o $@ $(MODOBJS) $(LDFLAGS)
 
.c.o: $<
	@echo "[CC] $@"
	@$(CC) $(CFLAGS) -o $@ -c $<
 
.PHONY: clean
clean:
	rm -f $(MODNAME) $(MODOBJS)
 
.PHONY: install
install: $(MODNAME)
	install -d $(DESTDIR)/usr/lib/freeswitch/mod
	install $(MODNAME) $(DESTDIR)/usr/lib/freeswitch/mod
