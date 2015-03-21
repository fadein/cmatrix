LDFLAGS = -lncurses

cmatrix: cmatrix.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f cmatrix cmatrix.o

.PHONY: clean
