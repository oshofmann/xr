CFLAGS=-I../common -g
LDFLAGS=-lssl

LD_CMD = cc $(LDFLAGS) -o $@ $^

TARGETS = c_json tags_json blocker

all: c_json tags_json blocker

c_json: main.o put_vals.o ansic.yy.o
	$(LD_CMD)

tags_json: main.o put_vals.o tags.yy.o
	$(LD_CMD)

blocker: blocker.o
	$(LD_CMD)

%.yy.c: %.l
	flex -o $@ $<

clean:
	rm -f $(TARGETS) *.o *.yy.c
