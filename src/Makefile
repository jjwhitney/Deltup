DESTDIR=
PREFIX=%%PREFIX%%
BINDIR=$(PREFIX)/bin

CC = g++

objfiles=bpatch.o archfunc.o file.o system.o tmpstore.o deltup.o 
libs=-lstdc++ -lz -lbz2 -lcrypto #-lssl

#edelta: edelta.cpp deltup
#	gcc edelta.cpp ${CXXFLAGS} -o edelta
%.o : %.cpp
	${CC} -c $< -o $@ ${CXXFLAGS} #-g #-Wall -pedantic

all: deltup

depend:
	gcc -MM *.cpp > .depend
	
deltup: $(objfiles)
	${CC} $(objfiles) ${CXXFLAGS} -o deltup $(libs)

install: deltup
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 deltup $(DESTDIR)$(BINDIR)

clean:
	-rm -f *.o deltup
