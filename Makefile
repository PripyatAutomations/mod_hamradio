#LOCALC_FLAGS=
MODNAME = mod_hamradio.so
MODOBJS += mod_hamradio.o dict.o config.o radio.o radio_gpio.o radio_rigctl.o tones.o
MODOBJS += conference.o util.o channel.o id.o vad.o core_processing.o endpoint.o
MODCFLAGS = -Wall -Werror
MODLDFLAGS = -lssl -lm -L/usr/local/lib -lgpiod -lhamlib

CC = gcc
CFLAGS = -fPIC -g -ggdb `pkg-config --cflags freeswitch` $(MODCFLAGS) -Wno-unused-variable
LDFLAGS = `pkg-config --libs freeswitch` $(MODLDFLAGS)

.PHONY: all conf-notice
all: $(MODNAME) conf-notice
$(MODNAME): ${MODOBJS}
	@echo "[LD] $@"
	@$(CC) -shared -o $@ $^ $(LDFLAGS)
 
.obj/%.o: %.c
	@echo "[CC] $@"
	@$(CC) $(CFLAGS) -o $@ -c $<
 
.PHONY: clean
clean:
	rm -f $(MODNAME) ${MODOBJS}
 
.PHONY: install
install: $(MODNAME)
	install -d $(DESTDIR)/usr/lib/freeswitch/mod
	install $(MODNAME) $(DESTDIR)/usr/lib/freeswitch/mod

conf-notice:
	@echo "You have succesfully built mod_hamradio! Now install it using 'sudo make install' or place mod_hamradio.so in your freeswitch modules directory."
	@echo ""
	@echo "Don't forget to copy hamradio.conf to /etc/freeswitch and edit it, if you haven't already!"
	@echo ""
	@echo "Thanks for trying mod_hamradio! Please report bugs or contribute improvements via https://github.com/pripyatautomations/mod_hamradio !"

install-config:
	sudo cp conf/hamradio.conf conf/*.xml /etc/freeswitch
