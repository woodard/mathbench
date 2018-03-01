CXXFLAGS=-g -O2 -fopenmp -D_GNU_SOURCE
LDFLAGS=-lm -ldl
INTPATH=/usr/tce/packages/intel/intel-18.0.1/lib/intel64_lin/

all: time_math2 compare_vals

time_math2: time_math2.o libmb.o
	g++ $(CXXFLAGS) time_math2.o libmb.o -o time_math2 $(LDFLAGS)

compare_vals: compare_vals.o libmb.o
	g++ $(CXXFLAGS) compare_vals.o libmb.o -o compare_vals $(LDFLAGS)

time_math2.o: time_math2.C libmb.h

compare_vals.o: compare_vals.C libmb.h

libmb.o: libmb.C libmb.h

# special cases
time_math2.noomp: time_math2-noomp.o libmb-noomp.o
	g++ -g -O2 -D_GNU_SOURCE time_math2-noomp.o libmb-noomp \
	    -o time_math2.noomp $(LDFLAGS)

libmb-noomp.o: libmb.C libmb.h
	g++ -g -O2 -D_GNU_SOURCE -c libmb.C -o libmb-noomp.o

time_math2-noomp.o: time_math2.C libmb.h
	g++ -g -O2 -D_GNU_SOURCE -c time_math2.C -o time_math2-noomp.o

#use intel math libraries
time_math2.mkl: time_math2.o libmb.o
	g++ $(CXXFLAGS) time_math2.o libmb.o -o time_math2 -L$(INTPATH) \
            -Wl,-rpath,$(INTPATH) -limf -lintlc

# utility
clean:
	rm -f *.o time_math2 time_math2.mkl time_math2.noomp \
	    compare_vals *~
