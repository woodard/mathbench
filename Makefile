CXXFLAGS=-g -O2
LDFLAGS=-lm

time_math2: time_math2.o
	g++ $(CXXFLAGS) time_math2.o -o time_math2 $(LDFLAGS)

time_math2.o: time_math2.C
