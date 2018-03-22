CXXFLAGS=-g -O2 -fopenmp -D_GNU_SOURCE
LDFLAGS=-ldl
INTPATH=/usr/tce/packages/intel/intel-18.0.1/lib/intel64_lin/

all: time_math2 compare_vals

time_math2: time_math2.o timeable.o ranges.o parameters.o
	g++ $(CXXFLAGS) time_math2.o timeable.o ranges.o parameters.o \
	  -o time_math2 $(LDFLAGS)

compare_vals: compare_vals.o parameters.o timeable.o
	g++ $(CXXFLAGS) compare_vals.o parameters.o timeable.o \
	  -o compare_vals $(LDFLAGS)

time_math2.o: time_math2.C timeable.h parameters.h ranges.h

compare_vals.o: compare_vals.C parameters.h timeable.h

timeable.o: timeable.C timeable.h parameters.h

ranges.o: ranges.h ranges.C timeable.h parameters.h

parameters.o: parameters.h parameters.C

# special cases
time_math2.noomp: time_math2-noomp.o libmb-noomp.o
	g++ -g -O2 -D_GNU_SOURCE time_math2-noomp.o libmb-noomp.o \
	    -o time_math2.noomp $(LDFLAGS)

libmb-noomp.o: timeable.C timeable.h
	g++ -g -O2 -D_GNU_SOURCE -c timeable.C -o libmb-noomp.o

time_math2-noomp.o: time_math2.C timeable.h
	g++ -g -O2 -D_GNU_SOURCE -c time_math2.C -o time_math2-noomp.o

# utility
clean:
	rm -f *.o time_math2 time_math2.mkl time_math2.noomp \
	    compare_vals *~
