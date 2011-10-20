# A minimal makefile for compiling the error visualizer
# (as libosso is the only build-dep, autotools would be overkill).

all:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean

distclean:
	$(MAKE) -C src distclean

install:
	install -d $(DESTDIR)/usr/bin/
	install src/sp-error-visualizer $(DESTDIR)/usr/bin/
	install -d $(DESTDIR)/usr/share/man/man1/
	cp man/* $(DESTDIR)/usr/share/man/man1/
	install -d $(DESTDIR)/usr/share/sp-error-visualizer/data/
	install -m 664 data/* $(DESTDIR)/usr/share/sp-error-visualizer/data/
	install -d $(DESTDIR)/usr/share/meegotouch/notifications/eventtypes/
	install -m 664 events/* $(DESTDIR)/usr/share/meegotouch/notifications/eventtypes/



