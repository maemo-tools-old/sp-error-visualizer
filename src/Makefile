TARGETS = sp-error-visualizer

all: $(TARGETS)

clean:
	$(RM) *.o *~

distclean: clean
	$(RM) $(TARGETS)

sp-error-visualizer: sp-error-visualizer.c
	gcc $(CFLAGS) -g -pedantic-errors -Werror -Wall -Wunreachable-code -std=c99 \
	`pkg-config --libs --cflags libosso` -o $@ $^

