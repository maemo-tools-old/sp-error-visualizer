TARGETS = sp-error-visualizer

all: $(TARGETS)

clean:
	$(RM) *.o *~

distclean: clean
	$(RM) $(TARGETS)

sp-error-visualizer: sp-error-visualizer.c
	gcc $(CFLAGS) -g -Wextra -Wall -std=c99 -o $@ $^ -Wl,--as-needed `pkg-config --libs --cflags dbus-glib-1 glib-2.0`
