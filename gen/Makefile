CC = gcc-4.5
CFLAGS = -g

LEX = flex

PLUGIN_DIR = $(shell $(CC) -print-file-name=plugin)
tagger.so: OBJS = tagger.o collapse_name.tagger.o
tagger.so: LDFLAGS += -shared -lz
tagger.so: CFLAGS += -I$(PLUGIN_DIR)/include -fPIC

blocker: OBJS = blocker.o
blocker: LDFLAGS +=-lcrypto

c_json: OBJS = main.o put_vals.o ansic.yy.o
tags_json: OBJS = main.o collapse_name.tags_json.o put_vals.o tags.yy.o
asm_json: OBJS = main.o put_vals.o asm.yy.o

TARGETS = c_json tags_json asm_json blocker tagger.so

all: $(TARGETS)

.SECONDEXPANSION:

$(TARGETS): $$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) 

%.tagger.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.tags_json.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.yy.c: %.l
	$(LEX) -o $@ $<

clean:
	rm -f $(TARGETS) *.o *.yy.c
