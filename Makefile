CXXFLAGS=-g -O2 -fopenmp
LDFLAGS=-lm
INTPATH=/usr/tce/packages/intel/intel-18.0.1/lib/intel64_lin/

time_math2: time_math2.o libmb.o
	g++ $(CXXFLAGS) time_math2.o libmb.o -o time_math2 $(LDFLAGS)

time_math2.o: time_math2.C libmb.h

libmb.o: libmb.C libmb.h

# special cases
time_math2.noomp: time_math2-noomp.o libmb-noomp.o
	g++ -g -O2 time_math2-noomp.o libmb-noomp -o time_math2.noomp $(LDFLAGS)

libmb-noomp.o: libmb.C libmb.h
	g++ -g -O2 -c libmb.C -o libmb-noomp.o

time_math2-noomp.o: time_math2.C libmb.h
	g++ -g -O2 -c time_math2.C -o time_math2-noomp.o

#use intel math libraries
time_math2.mkl: time_math2.o libmb.o
	g++ $(CXXFLAGS) time_math2.o libmb.o -o time_math2 -L$(INTPATH) \
            -Wl,-rpath,$(INTPATH) -limf -lintlc

# utility
clean:
	rm -f *.o time_math2
