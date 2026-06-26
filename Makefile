confdir := /opt/freeswitch/config/freeswitch

MODNAME = mod_hamradio.so
MODOBJS += dict.o
MODOBJS += mod_hamradio.o
MODOBJS += radio.o
MODOBJS += radio_cfg.o
MODOBJS += radio_channel.o
MODOBJS += radio_conf.o
MODOBJS += radio_core.o
MODOBJS += radio_endpoint.o
MODOBJS += radio_events.o 
MODOBJS += radio_gpio.o
MODOBJS += radio_hamlib.o
MODOBJS += radio_id.o
MODOBJS += radio_tones.o

MODCFLAGS = -Wall -Werror
MODLDFLAGS = -lssl -lm -L/usr/local/lib -lgpiod -lhamlib -lbsd

# uncomment these to disable features ;(
#MODCFLAGS += -DNO_HAMLIB
#MODCFLAGS += -DNO_LIBGPIOD

CC = gcc
CFLAGS = -fPIC -g -ggdb `pkg-config --cflags freeswitch` $(MODCFLAGS) -Wno-unused-variable
LDFLAGS = `pkg-config --libs freeswitch` $(MODLDFLAGS)

.PHONY: all conf-notice
all: world

world: $(MODNAME) conf-notice
$(MODNAME): ${MODOBJS}
	@echo "[LD] $@"
	@$(CC) -shared -o $@ $^ $(LDFLAGS)
 
%.o: %.c $(wildcard *.h)
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
	@echo "Don't forget to copy hamradio.conf to \$${confdir} and edit it, if you haven't already!"
	@echo ""
	@echo "Thanks for trying mod_hamradio! Please report bugs or contribute improvements via https://github.com/pripyatautomations/mod_hamradio !"

install-config: hostconf-load

hostconf-save:
	-diff -uNr conf/ ${confdir}/ > config.save.$$(date +"%Y%m%d_%H%M%S").diff
	sudo rsync -avx ${confdir}/ conf/

hostconf-load:
	-diff -uNr ${confdir}/ conf/ > config.save.$$(date +"%Y%m%d_%H%M%S").diff
	sudo rsync -avx conf/ ${confdir}/

distclean: clean
	${RM} *.diff
