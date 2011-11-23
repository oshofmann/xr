CIL=../cil-1.4.0
OCC=ocamlopt
EXT=cmxa
OBJ=cmx

ARCH = x86_64
BITS_x86_64 = 64
BITS_i386 = 32

BITS = ${BITS_$(ARCH)}

SRC=machine.ml tagger.ml
OBJS=$(SRC:.ml=.$(OBJ))

all: tagger

tagger: .depend $(OBJS)
	$(OCC) -ccopt -L$(CIL)/obj/x86_LINUX/ -o $@ \
		nums.$(EXT) unix.$(EXT) str.$(EXT) $(CIL)/obj/x86_LINUX/cil.$(EXT) \
		$(OBJS)

.depend: $(SRC) $(DOONE_SRC)
	ocamldep -I $(CIL)/obj/x86_LINUX $^ > .depend

%.$(OBJ): %.ml
	$(OCC) -c -I $(CIL)/obj/x86_LINUX $<

machine.ml:
	echo let machine_str = \
      \"`$(CIL)/obj/x86_LINUX/machdep-ml$(BITS).exe --env`\" > machine.ml

clean:
	rm -f *.cmi *.$(EXT) *.$(OBJ) *.o .depend $(MACHINE_ML) tagger

include .depend
