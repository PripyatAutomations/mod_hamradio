#LOCALC_FLAGS=
MODNAME = mod_hamradio.so
MODOBJS += mod_hamradio.o dict.o config.o radio.o radio_gpio.o radio_rigctl.o tones.o
MODCFLAGS = -Wall -Werror
MODLDFLAGS = -lssl -lm -L/usr/local/lib -lgpiod
real_objs := $(foreach x,${MODOBJS},.obj/${x})

CC = gcc
CFLAGS = -fPIC -g -ggdb `pkg-config --cflags freeswitch` $(MODCFLAGS) -Wno-unused-variable
LDFLAGS = `pkg-config --libs freeswitch` $(MODLDFLAGS)

printsrc_objs += printable_source.txt printable_source.pdf
 
.PHONY: all
all: $(MODNAME)
 
$(MODNAME): ${real_objs}
	@echo "[LD] $@"
	$(CC) -shared -o $@ $^ $(LDFLAGS)
 
.obj/%.o: %.c
	@echo "[CC] $@"
	$(CC) $(CFLAGS) -o $@ -c $<
 
.PHONY: clean
clean:
	rm -f $(MODNAME) ${real_objs} ${printsrc_objs}
 
.PHONY: install
install: $(MODNAME)
	install -d $(DESTDIR)/usr/lib/freeswitch/mod
	install $(MODNAME) $(DESTDIR)/usr/lib/freeswitch/mod

print:
	./tools/printsrc *.[ch] > printable_source.txt
