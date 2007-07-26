TARGETS = sp-error-visualizer

all: $(TARGETS)

clean:
	$(RM) *.o *~

distclean: clean
	$(RM) $(TARGETS)

sp-error-visualizer: sp-error-visualizer.c
	gcc $(CFLAGS) -g `pkg-config --libs --cflags libosso` -o $@ $^

