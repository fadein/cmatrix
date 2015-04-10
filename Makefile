LDFLAGS = -lncurses -L. -lccan
CFLAGS  = -I.

cmatrix: cmatrix.o
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

grab_file_test: grab_file_test.c libccan.a
	$(CC) $^ -o $@  $(CFLAGS) $(LDFLAGS)

clean:
	rm -f cmatrix cmatrix.o libccan.a
	find . -name *.o | xargs rm

.PHONY: clean

# Example makefile which makes a "libccan.a" of everything under ccan/.
# For simple projects you could just do:
#	SRCFILES += $(wildcard ccan/*/*.c)

#CCAN_CFLAGS=-g -O3 -Wall -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wwrite-strings -Wundef -DCCAN_STR_DEBUG=1
CCAN_CFLAGS=-g3 -ggdb -Wall -DCCAN_STR_DEBUG=1
CFLAGS = $(CCAN_CFLAGS) -I. $(DEPGEN)

# Modules which are just a header:
MODS_NO_SRC := compiler \
	typesafe_cb

# No external dependencies, with C code:
MODS_WITH_SRC := grab_file \
	noerr \
	talloc

MODS:=$(MODS_WITH_SRC) $(MODS_NO_SRC)

# Automatic dependency generation: makes ccan/*/*.d files.
DEPGEN=-MD
-include ccan/*/*.d

# Anything with C files needs building; dir leaves / on, sort uniquifies
DIRS=$(patsubst %/, %, $(sort $(foreach m, $(filter-out $(MODS_EXCLUDE), $(MODS_WITH_SRC)), $(dir $(wildcard ccan/$m/*.c)))))

# Generate everyone's separate Makefiles.
-include $(foreach dir, $(DIRS), $(dir)-Makefile)

ccan/%-Makefile:
	@echo $@: $(wildcard ccan/$*/*.[ch]) ccan/$*/_info > $@
	@echo ccan/$*.o: $(patsubst %.c, %.o, $(wildcard ccan/$*/*.c)) >> $@

# We compile all the ccan/foo/*.o files together into ccan/foo.o
OBJFILES=$(DIRS:=.o)

# We create all the .o files and link them together.
$(OBJFILES): %.o:
	$(LD) -r -o $@ $^

libccan.a: $(OBJFILES)
	$(AR) r $@ $(OBJFILES)
