CC = gcc
LDFLAGS = -lncurses -L. -lccan
#CCAN_CFLAGS=-g -O3 -Wall -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wwrite-strings -Wundef -DCCAN_STR_DEBUG=1
CCAN_CFLAGS=-g3 -ggdb -Wall -D_DEFAULT_SOURCE=1 -DCCAN_STR_DEBUG=1
CFLAGS = -std=c99 $(CCAN_CFLAGS) -I. $(DEPGEN)
PREFIX = /usr/local/bin

# Modules which are just a header:
MODS_NO_SRC := compiler \
	typesafe_cb

# No external dependencies, with C code:
MODS_WITH_SRC := grab_file \
	noerr \
	talloc

MODS:=$(MODS_WITH_SRC) $(MODS_NO_SRC)




cmatrix: cmatrix.o libccan.a
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)



# Automatic dependency generation: makes ccan/*/*.d files.
DEPGEN=-MD
-include ccan/*/*.d

# Anything with C files needs building; dir leaves / on, sort uniquifies
DIRS=$(patsubst %/, %, $(sort $(foreach m, $(filter-out $(MODS_EXCLUDE), $(MODS_WITH_SRC)), $(dir $(wildcard ccan/$m/*.c)))))

# Generate everyone's separate Makefiles.
-include $(foreach dir, $(DIRS), $(dir)-Makefile)

# We compile all the ccan/foo/*.o files together into ccan/foo.o
OBJFILES=$(DIRS:=.o)

# We create all the .o files and link them together.
$(OBJFILES): %.o:
	$(LD) -r -o $@ $^

ccan/%-Makefile:
	@echo $@: $(wildcard ccan/$*/*.[ch]) ccan/$*/_info > $@
	@echo ccan/$*.o: $(patsubst %.c, %.o, $(wildcard ccan/$*/*.c)) >> $@

libccan.a: $(OBJFILES)
	$(AR) r $@ $(OBJFILES)

grab_file_test: grab_file_test.c libccan.a
	$(CC) $^ -o $@  $(CFLAGS) $(LDFLAGS)

clean:
	rm -f cmatrix cmatrix.o libccan.a
	find . -name *.o | xargs rm -f

install: cmatrix
	install -m 755 -o root $^ $(PREFIX)

.PHONY: clean install
