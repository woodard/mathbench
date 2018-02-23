CXXFLAGS=-g -O2 -fopenmp
LDFLAGS=-lm

time_math2: time_math2.o
	g++ $(CXXFLAGS) time_math2.o -o time_math2 $(LDFLAGS)

time_math2.noomp: time_math2-noomp.o
	g++ -g -O2 time_math2-noomp.o -o time_math2.noomp $(LDFLAGS)

time_math2.mkl: time_math2.o
	g++ $(CXXFLAGS) time_math2.o -o time_math2 -L /usr/tce/packages/intel/intel-18.0.1/lib/intel64_lin/ -Wl,-rpath,/usr/tce/packages/intel/intel-18.0.1/lib/intel64_lin/ -limf -lintlc

time_math2.o: time_math2.C

time_math2-noomp.o: time_math2.C
	g++ -g -O2 -c time_math2.C -o time_math2-noomp.o

clean:
	rm -f *.o time_math2
